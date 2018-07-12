#include "MoodBlenderComponent.h"
#include "Atmosphere/AtmosphericFogComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "MovieScene.h"
#include "MovieScene3DTransformSection.h"
#include "MovieScene3DTransformTrack.h"
#include "MovieSceneColorSection.h"
#include "MovieSceneColorTrack.h"
#include "MovieSceneCommonHelpers.h"
#include "MovieSceneFloatSection.h"
#include "MovieSceneFloatTrack.h"
#include "MovieSceneSubTrack.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "MovieSceneMaterialTrack.h"
#include "MovieSceneParameterSection.h"
#include "MovieScenePossessable.h"
#include "Sections/MovieSceneSubSection.h"
#include "TimerManager.h"

UMoodBlenderComponent::UMoodBlenderComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UMoodBlenderComponent::OnRegister()
{
	Super::OnRegister();

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

void UMoodBlenderComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bBlending)
	{
		UpdateBlend();
	}
}

void UMoodBlenderComponent::SetMood(const int32 NewTime, const bool bForce)
{
	if (MoodSequence == nullptr || bBlending) return;
	if (CurrentFrame == NewTime && !bForce) return;

	UMovieScene* MovieScene = MoodSequence->GetMovieScene();
	if (MovieScene == nullptr) return;

	CurrentFrame = NewTime;

	Collections.Empty();
	OldCollectionStates.Empty();
	NewCollectionStates.Empty();
	GetAllCollections(MovieScene);

	Objects.Empty();
	OldObjectStates.Empty();
	NewObjectStates.Empty();
	GetAllObjects(MovieScene);

	if (NewObjectStates.Num() > 0 || NewCollectionStates.Num() > 0)
	{
		CurrentBlendTime = 0.0f;

		if (bForce)
		{
			BlendAlpha = 1.0f;
			UpdateBlend();
		}
		else
		{
			BlendAlpha = 0.0f;
			bBlending = true;
		}
	}
}

void UMoodBlenderComponent::GetAllCollections(UMovieScene* MovieScene)
{
	for (UMovieSceneTrack* Track : MovieScene->GetMasterTracks())
	{
		if (Track->IsA(UMovieSceneMaterialParameterCollectionTrack::StaticClass()))
		{
			CacheCollection((UMovieSceneMaterialParameterCollectionTrack*)Track);
		}

		if (Track->IsA(UMovieSceneSubTrack::StaticClass()))
		{
			UMovieSceneSubTrack* SubTrack = CastChecked<UMovieSceneSubTrack>(Track);

			const TArray<UMovieSceneSection*>& Sections = SubTrack->GetAllSections();
			for (const auto& Section : Sections)
			{
				const auto SubSection = CastChecked<UMovieSceneSubSection>(Section);

				// is the section referencing the sequence?
				const UMovieSceneSequence* SubSequence = SubSection->GetSequence();
				if (SubSequence == nullptr)
				{
					continue;
				}

				// does the section have sub-tracks referencing the sequence?
				UMovieScene* SubMovieScene = SubSequence->GetMovieScene();
				if (SubMovieScene != nullptr)
				{
					GetAllCollections(SubMovieScene);
				}
			}
		}
	}
}

void UMoodBlenderComponent::CacheCollection(const UMovieSceneMaterialParameterCollectionTrack* Track)
{
	if (Track->MPC == nullptr) return;

	Collections.AddUnique(Track->MPC);
	CacheSequencerCollection(Track);
	CacheCurrentCollection(Track->MPC);
}

void UMoodBlenderComponent::CacheSequencerCollection(const UMovieSceneMaterialParameterCollectionTrack* Track)
{
	FCollectionMood& NewState = NewCollectionStates.FindOrAdd(Track->MPC);

	const UMovieSceneParameterSection* Section = Cast<UMovieSceneParameterSection>(MovieSceneHelpers::FindSectionAtTime(Track->GetAllSections(), CurrentFrame));
	if (Section)
	{
		for (const FScalarParameterNameAndCurve ScalarCurve : Section->GetScalarParameterNamesAndCurves())
		{
			NewState.Scalars.Add(ScalarCurve.ParameterName, ScalarCurve.ParameterCurve.Eval(CurrentFrame));
			NewState.IsValid = true;
		}

		for (const FColorParameterNameAndCurves ColorCurve : Section->GetColorParameterNamesAndCurves())
		{
			FLinearColor Color = FLinearColor
			(
				ColorCurve.RedCurve.Eval(CurrentFrame),
				ColorCurve.GreenCurve.Eval(CurrentFrame),
				ColorCurve.BlueCurve.Eval(CurrentFrame),
				ColorCurve.AlphaCurve.Eval(CurrentFrame)
			);
			NewState.Colors.Add(ColorCurve.ParameterName, Color);
			NewState.IsValid = true;
		}
	}
}

void UMoodBlenderComponent::CacheCurrentCollection(UMaterialParameterCollection* Collection)
{
	FCollectionMood& NewState = NewCollectionStates.FindOrAdd(Collection);
	FCollectionMood& OldState = OldCollectionStates.FindOrAdd(Collection);

	for (TPair<FName, float> Pair : NewState.Scalars)
	{
		float Value = UKismetMaterialLibrary::GetScalarParameterValue(GetWorld(), Collection, Pair.Key);
		OldState.Scalars.Add(Pair.Key, Value);
	}

	for (TPair<FName, FLinearColor> Pair : NewState.Colors)
	{
		FLinearColor Value = UKismetMaterialLibrary::GetVectorParameterValue(GetWorld(), Collection, Pair.Key);
		OldState.Colors.Add(Pair.Key, Value);
	}
}

void UMoodBlenderComponent::GetAllObjects(UMovieScene* MovieScene)
{
	for (int i = 0; i < MovieScene->GetPossessableCount(); i++)
	{
		const FGuid& ObjectGuid = MovieScene->GetPossessable(i).GetGuid();
		check(ObjectGuid.IsValid());

		const FMovieScenePossessable* Possessable = MovieScene->FindPossessable(ObjectGuid);
		const UClass* PossessedClass = Possessable->GetPossessedObjectClass();
		TArray<UMovieScenePropertyTrack*> PropertyTracks;

		for (const FMovieSceneBinding& Binding : MovieScene->GetBindings())
		{
			if (Binding.GetObjectGuid() != ObjectGuid)
			{
				continue;
			}

			for (const UMovieSceneTrack* Track : Binding.GetTracks())
			{
				if (Track->IsA(UMovieScenePropertyTrack::StaticClass()))
				{
					PropertyTracks.Add((UMovieScenePropertyTrack*)Track);
				}
			}
		}

		if (PropertyTracks.Num() > 0)
		{
			if (PossessedClass == UAtmosphericFogComponent::StaticClass())
			{
				if (AtmosphericFog != nullptr)
				{
					CacheObjects(PropertyTracks, AtmosphericFog->GetAtmosphericFogComponent());
				}
			}
			else if (PossessedClass == UExponentialHeightFogComponent::StaticClass())
			{
				if (ExponentialFog != nullptr)
				{
					CacheObjects(PropertyTracks, ExponentialFog->GetComponent());
				}
			}
			else if (PossessedClass == ADirectionalLight::StaticClass())
			{
				if (DirectionalLight != nullptr)
				{
					CacheObjects(PropertyTracks, DirectionalLight);
				}
			}
			else if (PossessedClass == UDirectionalLightComponent::StaticClass())
			{
				if (DirectionalLight != nullptr)
				{
					CacheObjects(PropertyTracks, DirectionalLight->GetLightComponent());
				}
			}
			else if (PossessedClass == ASkyLight::StaticClass())
			{
				if (SkyLight != nullptr)
				{
					CacheObjects(PropertyTracks, SkyLight);
				}
			}
			else if (PossessedClass == USkyLightComponent::StaticClass())
			{
				if (SkyLight != nullptr)
				{
					CacheObjects(PropertyTracks, SkyLight->GetLightComponent());
				}
			}
		}
	}
}

void UMoodBlenderComponent::CacheObjects(const TArray<UMovieScenePropertyTrack*> Tracks, UObject* Object)
{
	if (Object == nullptr) return;

	Objects.AddUnique(Object);
	CacheSequencerObject(Tracks, Object);
	CacheCurrentObject(Object);

}

void UMoodBlenderComponent::CacheSequencerObject(const TArray<UMovieScenePropertyTrack*> Tracks, UObject* Object)
{
	FObjectMood& NewState = NewObjectStates.FindOrAdd(Object);
	NewState.IsNewTransform = false;

	for (const UMovieScenePropertyTrack* Track : Tracks)
	{
		if (Track->GetClass() == UMovieScene3DTransformTrack::StaticClass())
		{
			const UMovieScene3DTransformSection* Section = Cast<UMovieScene3DTransformSection>(MovieSceneHelpers::FindSectionAtTime(Track->GetAllSections(), CurrentFrame));
			if (Section)
			{
				FRotator Rotation = FRotator
				(
					Section->GetRotationCurve(EAxis::Y).Eval(CurrentFrame),
					Section->GetRotationCurve(EAxis::Z).Eval(CurrentFrame),
					Section->GetRotationCurve(EAxis::X).Eval(CurrentFrame)
				);
				FVector Translation = FVector
				(
					Section->GetTranslationCurve(EAxis::X).Eval(CurrentFrame),
					Section->GetTranslationCurve(EAxis::Y).Eval(CurrentFrame),
					Section->GetTranslationCurve(EAxis::Z).Eval(CurrentFrame)
				);
				FVector Scale = FVector
				(
					Section->GetScaleCurve(EAxis::X).Eval(CurrentFrame),
					Section->GetScaleCurve(EAxis::Y).Eval(CurrentFrame),
					Section->GetScaleCurve(EAxis::Z).Eval(CurrentFrame)
				);

				NewState.Transform = FTransform(Rotation, Translation, Scale);
				NewState.IsNewTransform = true;
				NewState.IsValid = true;
			}
		}

		if (Track->GetClass() == UMovieSceneFloatTrack::StaticClass())
		{
			const UMovieSceneFloatSection* Section = Cast<UMovieSceneFloatSection>(MovieSceneHelpers::FindSectionAtTime(Track->GetAllSections(), CurrentFrame));
			if (Section)
			{
				NewState.Floats.Add(Track->GetPropertyName(), Section->GetFloatCurve().Eval(CurrentFrame));
				NewState.IsValid = true;
			}
		}

		if (Track->GetClass() == UMovieSceneColorTrack::StaticClass())
		{
			const UMovieSceneColorSection* Section = Cast<UMovieSceneColorSection>(MovieSceneHelpers::FindSectionAtTime(Track->GetAllSections(), CurrentFrame));
			if (Section)
			{
				FLinearColor Color = FLinearColor
				(
					Section->GetRedCurve().Eval(CurrentFrame),
					Section->GetGreenCurve().Eval(CurrentFrame),
					Section->GetBlueCurve().Eval(CurrentFrame),
					Section->GetAlphaCurve().Eval(CurrentFrame)
				);
				NewState.Colors.Add(Track->GetPropertyName(), Color);
				NewState.IsValid = true;
			}
		}
	}
}

void UMoodBlenderComponent::CacheCurrentObject(UObject* Object)
{
	FObjectMood& NewState = NewObjectStates.FindOrAdd(Object);
	FObjectMood& OldState = OldObjectStates.FindOrAdd(Object);

	const AActor* Actor = Cast<AActor>(Object);
	if (Actor != nullptr)
	{
		OldState.Transform = Actor->GetActorTransform();
	}
	else
	{
		const USceneComponent* Component = Cast<USceneComponent>(Object);
		if (Component != nullptr)
		{
			OldState.Transform = Component->GetRelativeTransform();
		}
	}

	for (TPair<FName, float> Pair : NewState.Floats)
	{
		UFloatProperty* Property = FindField<UFloatProperty>(Object->GetClass(), Pair.Key);
		if (Property != nullptr)
		{
			float Value = Property->GetPropertyValue_InContainer(Object);
			OldState.Floats.Add(Pair.Key, Value);
		}
	}

	for (TPair<FName, FLinearColor> Pair : NewState.Colors)
	{
		UStructProperty* Property = FindField<UStructProperty>(Object->GetClass(), Pair.Key);
		if (Property != nullptr)
		{
			UScriptStruct* ScriptStruct = Property->Struct;
			FLinearColor Value = FLinearColor
			(
				Cast<UFloatProperty>(ScriptStruct->FindPropertyByName("R"))->GetDefaultPropertyValue(),
				Cast<UFloatProperty>(ScriptStruct->FindPropertyByName("G"))->GetDefaultPropertyValue(),
				Cast<UFloatProperty>(ScriptStruct->FindPropertyByName("B"))->GetDefaultPropertyValue(),
				Cast<UFloatProperty>(ScriptStruct->FindPropertyByName("A"))->GetDefaultPropertyValue()
			);
			OldState.Colors.Add(Pair.Key, Value);
		}
	}
}

void UMoodBlenderComponent::UpdateBlend()
{
	// update blend alpha
	if (BlendAlpha < 1.0f)
	{
		CurrentBlendTime = FMath::Clamp(CurrentBlendTime + UGameplayStatics::GetWorldDeltaSeconds(this), 0.0f, BlendTime);
		BlendAlpha = FMath::Clamp(CurrentBlendTime / BlendTime, 0.0f, 1.0f);
	}

	// update properties
	for (TPair<UMaterialParameterCollection*, FCollectionMood> Pair : NewCollectionStates)
	{
		UpdateCollections(Pair.Key, Pair.Value);
	}
	for (TPair<UObject*, FObjectMood> Pair : NewObjectStates)
	{
		UpdateObjects(Pair.Key, Pair.Value);
	}

	// apply changes
	for (UMaterialParameterCollection* Collection : Collections)
	{
		GetWorld()->GetParameterCollectionInstance(Collection)->UpdateRenderState();
	}
	for (UObject* Object : Objects)
	{
		if (Object->GetClass()->IsChildOf(AActor::StaticClass()))
		{
			(Cast<AActor>(Object))->MarkComponentsRenderStateDirty();
		}
		else
		{
			Cast<USceneComponent>(Object)->MarkRenderStateDirty();
		}
	}

	if (SkyLight != nullptr)
	{
		if ((BlendAlpha < 1.0f && bRecaptureSkyEveryFrame) || BlendAlpha == 1.0f)
		{
			RecaptureSky();
		}
	}

	if (BlendAlpha == 1.0f)
	{
		bBlending = false;
	}
}

void UMoodBlenderComponent::UpdateCollections(UMaterialParameterCollection* Collection, FCollectionMood& NewState)
{
	if (NewState.IsValid)
	{
		FCollectionMood& OriginalState = OriginalCollectionStates.FindOrAdd(Collection);
		FCollectionMood& OldState = OldCollectionStates.FindOrAdd(Collection);
		if (!OriginalState.IsValid)
		{
			OriginalState = OldState;
			OriginalState.IsValid = true;
		}

		float AlphaCopy = BlendAlpha;
		if (NewState.Scalars.Num() > 0 && OldState.Scalars.Num() > 0)
		{
			for (TPair<FName, float> Pair : NewState.Scalars)
			{
				UKismetMaterialLibrary::SetScalarParameterValue(GetWorld(), Collection, Pair.Key, FMath::Lerp(OldState.Scalars[Pair.Key], Pair.Value, AlphaCopy));
			}
		}

		AlphaCopy = BlendAlpha;
		if (NewState.Colors.Num() > 0 && OldState.Colors.Num() > 0)
		{
			for (TPair<FName, FLinearColor> Pair : NewState.Colors)
			{
				UKismetMaterialLibrary::SetVectorParameterValue(GetWorld(), Collection, Pair.Key, UKismetMathLibrary::LinearColorLerp(OldState.Colors[Pair.Key], Pair.Value, AlphaCopy));
			}
		}
	}
}

void UMoodBlenderComponent::UpdateObjects(UObject* Object, FObjectMood& NewState)
{
	if (NewState.IsValid)
	{
		FObjectMood& OriginalState = OriginalObjectStates.FindOrAdd(Object);
		FObjectMood& OldState = OldObjectStates.FindOrAdd(Object);
		if (!OriginalState.IsValid)
		{
			OriginalState = OldState;
			OriginalState.IsValid = true;
		}

		float AlphaCopy = BlendAlpha;
		if (NewState.IsNewTransform)
		{
			FTransform TransformValue = UKismetMathLibrary::TLerp(OldState.Transform, NewState.Transform, AlphaCopy);

			if (Object->GetClass()->IsChildOf(AActor::StaticClass()))
			{
				Cast<AActor>(Object)->SetActorTransform(TransformValue);
			}
			else
			{
				Cast<USceneComponent>(Object)->SetRelativeTransform(TransformValue);
			}
		}

		AlphaCopy = BlendAlpha;
		if (NewState.Floats.Num() > 0 && OldState.Floats.Num() > 0)
		{
			for (TPair<FName, float> Pair : NewState.Floats)
			{
				UFloatProperty* Property = FindField<UFloatProperty>(Object->GetClass(), Pair.Key);
				Property->SetPropertyValue_InContainer(Object, FMath::Lerp(OldState.Floats[Pair.Key], Pair.Value, AlphaCopy));
			}
		}

		AlphaCopy = BlendAlpha;
		if (NewState.Colors.Num() > 0 && OldState.Colors.Num() > 0)
		{
			for (TPair<FName, FLinearColor> Pair : NewState.Colors)
			{
				UStructProperty* Property = FindField<UStructProperty>(Object->GetClass(), Pair.Key);
				if (Property != nullptr)
				{
					FLinearColor CurrentLinearColor = UKismetMathLibrary::LinearColorLerp(OldState.Colors[Pair.Key], Pair.Value, AlphaCopy);

					if (Property->Struct == TBaseStructure<FLinearColor>::Get())
					{
						*Property->ContainerPtrToValuePtr<FLinearColor>(Object) = CurrentLinearColor;
					}
					else if (Property->Struct == TBaseStructure<FColor>::Get())
					{
						*Property->ContainerPtrToValuePtr<FColor>(Object) = CurrentLinearColor.ToFColor(true);
					}
				}
			}
		}
	}
}

void UMoodBlenderComponent::RecaptureSky()
{
	if (SkyLight != nullptr)
	{
		SkyLight->GetLightComponent()->RecaptureSky();
	}
}

void UMoodBlenderComponent::OnGameInitialized()
{
	SetMood(0, true);

	if (FirstRecaptureDelay > 0.0f)
	{
		FTimerHandle FirstRecaptureTimer;
		GetOwner()->GetWorldTimerManager().SetTimer(FirstRecaptureTimer, this, &UMoodBlenderComponent::RecaptureSky, FirstRecaptureDelay, false, 0.0f);
	}
}
