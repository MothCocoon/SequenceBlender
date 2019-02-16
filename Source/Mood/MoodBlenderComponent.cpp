#include "MoodBlenderComponent.h"

#include "Components/SkyLightComponent.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "MovieScene3DTransformSection.h"
#include "MovieScene3DTransformTrack.h"
#include "MovieSceneChannelProxy.h"
#include "MovieSceneColorSection.h"
#include "MovieSceneColorTrack.h"
#include "MovieSceneCommonHelpers.h"
#include "MovieSceneFloatSection.h"
#include "MovieSceneFloatTrack.h"
#include "MovieSceneMaterialParameterCollectionTrack.h"
#include "MovieSceneParameterSection.h"
#include "MovieScenePropertyTrack.h"
#include "TimerManager.h"

UMoodBlenderComponent::UMoodBlenderComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.SetTickFunctionEnable(false);
}

void UMoodBlenderComponent::OnRegister()
{
	Super::OnRegister();

	if (MoodSequence == nullptr) return;

	MoodMovie = MoodSequence->GetMovieScene();
	CacheTracks();

	if (bResetTime)
	{
		SetMood(0, true);
		bResetTime = false;
	}
	else if (ForceTime > 0)
	{
		SetMood(ForceTime, true);
		ForceTime = 0;
	}
}

void UMoodBlenderComponent::CacheTracks()
{
	CollectionTracks.Empty();
	ObjectTracks.Empty();

	// get material collection tracks
	for (UMovieSceneTrack* Track : MoodMovie.Get()->GetMasterTracks())
	{
		UMovieSceneMaterialParameterCollectionTrack* CollectionTrack = Cast<UMovieSceneMaterialParameterCollectionTrack>(Track);
		if (CollectionTrack)
		{
			CollectionTracks.AddUnique(CollectionTrack);
		}
	}

	// sequence player is needed to obtain object references
	ALevelSequenceActor* SequenceActor;
	ULevelSequencePlayer* SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(this, MoodSequence, FMovieSceneSequencePlaybackSettings(), SequenceActor);
	if (SequencePlayer == nullptr) return;

	// get object pointers
	for (int i = 0; i < MoodMovie.Get()->GetPossessableCount(); i++)
	{
		const FGuid& ObjectGuid = MoodMovie.Get()->GetPossessable(i).GetGuid();
		check(ObjectGuid.IsValid());

		const TArrayView<TWeakObjectPtr<>> FoundObjects = SequencePlayer->FindBoundObjects(ObjectGuid, MovieSceneSequenceID::Root);
		for (const TWeakObjectPtr<> WeakObj : FoundObjects)
		{
			if (UObject* Object = WeakObj.Get())
			{
				for (const FMovieSceneBinding& Binding : MoodMovie.Get()->GetBindings())
				{
					if (Binding.GetObjectGuid() != ObjectGuid) continue;

					TArray<TWeakObjectPtr<UMovieScenePropertyTrack>> PropertyTracks;
					for (UMovieSceneTrack* Track : Binding.GetTracks())
					{
						TWeakObjectPtr<UMovieScenePropertyTrack> PropertyTrack = Cast<UMovieScenePropertyTrack>(Track);
						if (PropertyTrack.IsValid())
						{
							PropertyTracks.Add(PropertyTrack);
						}
					}

					ObjectTracks.Add(Object, FCachedPropertyTrack(Cast<AActor>(Object), Cast<USceneComponent>(Object), PropertyTracks));
				}

				break;
			}
		}
	}

	USceneComponent* Component = GetComponentFromSequence(USkyLightComponent::StaticClass());
	if (Component)
	{
		SkyLightComponent = Cast<USkyLightComponent>(Component);
	}
}

USceneComponent* UMoodBlenderComponent::GetComponentFromSequence(const TSubclassOf<USceneComponent> Class)
{
	for (const TPair<TWeakObjectPtr<UObject>, FCachedPropertyTrack>& Object : ObjectTracks)
	{
		if (Object.Key->IsA(Class))
		{
			return Cast<USceneComponent>(Object.Key);
		}
	}

	return nullptr;
}

void UMoodBlenderComponent::Init()
{
	if (FirstRecaptureDelay > 0.0f)
	{
		FTimerHandle FirstRecaptureTimer;
		GetOwner()->GetWorldTimerManager().SetTimer(FirstRecaptureTimer, this, &UMoodBlenderComponent::RecaptureSky, FirstRecaptureDelay, false, 0.0f);
	}
}

void UMoodBlenderComponent::RecaptureSky()
{
	if (SkyLightComponent.IsValid())
	{
		SkyLightComponent.Get()->RecaptureSky();
	}
}

void UMoodBlenderComponent::SetMood(const int32 NewTime, const bool bForce)
{
	if (bBlending || MoodSequence == nullptr || !MoodMovie.IsValid()) return;
	if (CurrentFrame == NewTime && !bForce) return;

	const FFrameNumber NewFrameNumber = MoodMovie.Get()->GetTickResolution().AsFrameNumber(NewTime);
	if (!MoodMovie.Get()->GetPlaybackRange().Contains(NewFrameNumber)) return;

	CurrentFrame = NewTime;
	CurrentFrameNumber = NewFrameNumber;
	CurrentFrameTime = FFrameTime(NewFrameNumber, 0.0f);

	World = GetWorld();

	// cache all collections
	OldCollectionStates.Empty();
	NewCollectionStates.Empty();
	for (TWeakObjectPtr<UMovieSceneMaterialParameterCollectionTrack>& Track : CollectionTracks)
	{
		if (Track.IsValid())
		{
			CacheCollection(Track.Get());
		}
	}

	// cache all objects
	OldObjectStates.Empty();
	NewObjectStates.Empty();
	for (TPair<TWeakObjectPtr<UObject>, FCachedPropertyTrack>& Object : ObjectTracks)
	{
		if (Object.Key.IsValid())
		{
			CacheObject(Object.Key.Get(), Object.Value.Tracks);
		}
	}

	// call update
	if (NewObjectStates.Num() > 0 || NewCollectionStates.Num() > 0)
	{
		if (bForce)
		{
			BlendAlpha = 1.0f;
			UpdateMood();
		}
		else if (World.Get()->WorldType == EWorldType::Game || World.Get()->WorldType == EWorldType::PIE)
		{
			BlendAlpha = 0.0f;
			bBlending = true;
			PrimaryComponentTick.SetTickFunctionEnable(true);
		}
	}
}

void UMoodBlenderComponent::CacheCollection(UMovieSceneMaterialParameterCollectionTrack* Track)
{
	UMaterialParameterCollection* MPC = Track->MPC;

	FCollectionMood& OldState = OldCollectionStates.FindOrAdd(MPC);
	FCollectionMood& NewState = NewCollectionStates.FindOrAdd(MPC);

	const UMovieSceneParameterSection* Section = Cast<UMovieSceneParameterSection>(MovieSceneHelpers::FindSectionAtTime(Track->GetAllSections(), CurrentFrameNumber));
	if (Section)
	{
		for (const FScalarParameterNameAndCurve ScalarCurve : Section->GetScalarParameterNamesAndCurves())
		{
			OldState.Scalars.Add(ScalarCurve.ParameterName, UKismetMaterialLibrary::GetScalarParameterValue(World.Get(), MPC, ScalarCurve.ParameterName));

			float NewFloat;
			ScalarCurve.ParameterCurve.Evaluate(CurrentFrameTime, NewFloat);

			NewState.Scalars.Add(ScalarCurve.ParameterName, NewFloat);
			NewState.bValid = true;
		}

		for (const FColorParameterNameAndCurves ColorCurve : Section->GetColorParameterNamesAndCurves())
		{
			OldState.Colors.Add(ColorCurve.ParameterName, UKismetMaterialLibrary::GetVectorParameterValue(World.Get(), MPC, ColorCurve.ParameterName));

			FLinearColor Color;
			ColorCurve.RedCurve.Evaluate(CurrentFrameTime, Color.R);
			ColorCurve.GreenCurve.Evaluate(CurrentFrameTime, Color.G);
			ColorCurve.BlueCurve.Evaluate(CurrentFrameTime, Color.B);
			ColorCurve.AlphaCurve.Evaluate(CurrentFrameTime, Color.A);

			NewState.Colors.Add(ColorCurve.ParameterName, Color);
			NewState.bValid = true;
		}
	}
}

void UMoodBlenderComponent::CacheObject(UObject* Object, TArray<TWeakObjectPtr<UMovieScenePropertyTrack>>& Tracks)
{
	FObjectMood& OldState = OldObjectStates.FindOrAdd(Object);
	FObjectMood& NewState = NewObjectStates.FindOrAdd(Object);

	for (const TWeakObjectPtr<UMovieScenePropertyTrack>& TrackPtr : Tracks)
	{
		if (!TrackPtr.IsValid()) { continue; }

		const UMovieScenePropertyTrack* Track = TrackPtr.Get();
		if (Track->GetClass() == UMovieScene3DTransformTrack::StaticClass())
		{
			if (const USceneComponent* Component = Cast<USceneComponent>(Object))
			{
				OldState.Transform = Component->GetRelativeTransform();
			}
			if (const AActor* Actor = Cast<AActor>(Object))
			{
				OldState.Transform = Actor->GetActorTransform();
			}

			const UMovieScene3DTransformSection* Section = Cast<UMovieScene3DTransformSection>(MovieSceneHelpers::FindSectionAtTime(Track->GetAllSections(), CurrentFrameNumber));
			if (Section)
			{
				TArrayView<FMovieSceneFloatChannel*> FloatChannels = Section->GetChannelProxy().GetChannels<FMovieSceneFloatChannel>();

				FVector Translation;
				FloatChannels[0]->Evaluate(CurrentFrameTime, Translation.X);
				FloatChannels[1]->Evaluate(CurrentFrameTime, Translation.Y);
				FloatChannels[2]->Evaluate(CurrentFrameTime, Translation.Z);

				FRotator Rotation;
				FloatChannels[3]->Evaluate(CurrentFrameTime, Rotation.Pitch);
				FloatChannels[4]->Evaluate(CurrentFrameTime, Rotation.Yaw);
				FloatChannels[5]->Evaluate(CurrentFrameTime, Rotation.Roll);

				FVector Scale;
				FloatChannels[6]->Evaluate(CurrentFrameTime, Scale.X);
				FloatChannels[7]->Evaluate(CurrentFrameTime, Scale.Y);
				FloatChannels[8]->Evaluate(CurrentFrameTime, Scale.Z);

				NewState.Transform = FTransform(Rotation, Translation, Scale);
				NewState.bNewTransform = true;
				NewState.bValid = true;
			}
		}

		if (Track->GetClass() == UMovieSceneFloatTrack::StaticClass())
		{
			UFloatProperty* Property = FindField<UFloatProperty>(Object->GetClass(), Track->GetPropertyName());
			const UMovieSceneFloatSection* Section = Cast<UMovieSceneFloatSection>(MovieSceneHelpers::FindSectionAtTime(Track->GetAllSections(), CurrentFrameNumber));
			if (Property && Section)
			{
				OldState.Floats.Add(Property, Property->GetPropertyValue_InContainer(Object));

				float NewFloat;
				Section->GetChannel().Evaluate(CurrentFrameTime, NewFloat);

				NewState.Floats.Add(Property, NewFloat);
				NewState.bValid = true;
			}
		}

		if (Track->GetClass() == UMovieSceneColorTrack::StaticClass())
		{
			UStructProperty* Property = FindField<UStructProperty>(Object->GetClass(), Track->GetPropertyName());
			const UMovieSceneColorSection* Section = Cast<UMovieSceneColorSection>(MovieSceneHelpers::FindSectionAtTime(Track->GetAllSections(), CurrentFrameNumber));
			if (Property && Section)
			{
				FLinearColor Color;
				Section->GetRedChannel().Evaluate(CurrentFrameTime, Color.R);
				Section->GetGreenChannel().Evaluate(CurrentFrameTime, Color.G);
				Section->GetBlueChannel().Evaluate(CurrentFrameTime, Color.B);
				Section->GetAlphaChannel().Evaluate(CurrentFrameTime, Color.A);

				const UScriptStruct* ScriptStruct = Property->Struct;
				if (ScriptStruct == TBaseStructure<FColor>::Get())
				{
					const FColor Value = *Property->ContainerPtrToValuePtr<FColor>(Object);
					OldState.Colors.Add(Property, FLinearColor::FromSRGBColor(Value));

					NewState.Colors.Add(Property, Color);
					NewState.bValid = true;
				}
				else if (ScriptStruct == TBaseStructure<FLinearColor>::Get())
				{
					const FLinearColor Value = FLinearColor
					(
						Cast<UFloatProperty>(ScriptStruct->FindPropertyByName("R"))->GetDefaultPropertyValue(),
						Cast<UFloatProperty>(ScriptStruct->FindPropertyByName("G"))->GetDefaultPropertyValue(),
						Cast<UFloatProperty>(ScriptStruct->FindPropertyByName("B"))->GetDefaultPropertyValue(),
						Cast<UFloatProperty>(ScriptStruct->FindPropertyByName("A"))->GetDefaultPropertyValue()
					);
					OldState.LinearColors.Add(Property, Value);

					NewState.LinearColors.Add(Property, Color);
					NewState.bValid = true;
				}
			}
		}
	}
}

void UMoodBlenderComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateMood();
}

void UMoodBlenderComponent::UpdateMood()
{
	if (bBlending)
	{
		CurrentBlendTime = FMath::Min(CurrentBlendTime + World.Get()->GetDeltaSeconds(), BlendTime);
		BlendAlpha = FMath::Min(CurrentBlendTime / BlendTime, 1.0f);
	}

	// update properties
	for (TPair<TWeakObjectPtr<UMaterialParameterCollection>, FCollectionMood>& Pair : NewCollectionStates)
	{
		if (Pair.Key.IsValid() && Pair.Value.bValid)
		{
			UpdateCollection(Pair.Key.Get(), Pair.Value);
		}
	}
	for (TPair<TWeakObjectPtr<UObject>, FObjectMood>& Pair : NewObjectStates)
	{
		if (Pair.Key.IsValid() && Pair.Value.bValid)
		{
			UpdateObject(Pair.Key.Get(), Pair.Value, ObjectTracks[Pair.Key]);
		}
	}

	// apply changes
	for (const TWeakObjectPtr<UMovieSceneMaterialParameterCollectionTrack>& Track : CollectionTracks)
	{
		if (Track.IsValid())
		{
			World.Get()->GetParameterCollectionInstance(Track.Get()->MPC)->UpdateRenderState();
		}
	}
	for (const TPair<TWeakObjectPtr<UObject>, FCachedPropertyTrack>& Object : ObjectTracks)
	{
		if (Object.Value.Component.IsValid())
		{
			Object.Value.Component.Get()->MarkRenderStateDirty();
		}
		else if (Object.Value.Actor.IsValid())
		{
			Object.Value.Actor.Get()->MarkComponentsRenderStateDirty();
		}
	}

	if (bRecaptureSkyEveryFrame || BlendAlpha == 1.0f)
	{
		RecaptureSky();
	}

	if (BlendAlpha == 1.0f)
	{
		CurrentBlendTime = 0.0f;
		BlendAlpha = 0.0f;
		bBlending = false;
		PrimaryComponentTick.SetTickFunctionEnable(false);
	}
}

void UMoodBlenderComponent::UpdateCollection(UMaterialParameterCollection* Collection, const FCollectionMood& NewState)
{
	FCollectionMood& OldState = OldCollectionStates.FindOrAdd(Collection);

	for (const TPair<FName, float>& Pair : NewState.Scalars)
	{
		UKismetMaterialLibrary::SetScalarParameterValue(World.Get(), Collection, Pair.Key, FMath::Lerp(OldState.Scalars[Pair.Key], Pair.Value, BlendAlpha));
	}

	for (const TPair<FName, FLinearColor>& Pair : NewState.Colors)
	{
		UKismetMaterialLibrary::SetVectorParameterValue(World.Get(), Collection, Pair.Key, UKismetMathLibrary::LinearColorLerp(OldState.Colors[Pair.Key], Pair.Value, BlendAlpha));
	}
}

void UMoodBlenderComponent::UpdateObject(UObject* Object, const FObjectMood& NewState, const FCachedPropertyTrack& CachedTrack)
{
	FObjectMood& OldState = OldObjectStates.FindOrAdd(Object);

	if (NewState.bNewTransform)
	{
		const FTransform CurrentTransform = UKismetMathLibrary::TLerp(OldState.Transform, NewState.Transform, BlendAlpha);

		if (CachedTrack.Component.IsValid())
		{
			CachedTrack.Component.Get()->SetRelativeTransform(CurrentTransform);
		}
		else if (CachedTrack.Actor.IsValid())
		{
			CachedTrack.Actor.Get()->SetActorTransform(CurrentTransform);
		}
	}

	for (const TPair<UFloatProperty*, float>& Pair : NewState.Floats)
	{
		Pair.Key->SetPropertyValue_InContainer(Object, FMath::Lerp(OldState.Floats[Pair.Key], Pair.Value, BlendAlpha));
	}

	for (const TPair<UStructProperty*, FLinearColor>& Pair : NewState.Colors)
	{
		const FLinearColor CurrentLinearColor = UKismetMathLibrary::LinearColorLerp(OldState.Colors[Pair.Key], Pair.Value, BlendAlpha);
		*Pair.Key->ContainerPtrToValuePtr<FColor>(Object) = CurrentLinearColor.ToFColor(true);
	}

	for (const TPair<UStructProperty*, FLinearColor>& Pair : NewState.LinearColors)
	{
		const FLinearColor CurrentLinearColor = UKismetMathLibrary::LinearColorLerp(OldState.LinearColors[Pair.Key], Pair.Value, BlendAlpha);
		*Pair.Key->ContainerPtrToValuePtr<FLinearColor>(Object) = CurrentLinearColor;
	}
}
