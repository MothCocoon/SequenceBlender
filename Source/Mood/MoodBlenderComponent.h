#pragma once

#include "CoreMinimal.h"
#include "MoodBlenderComponent.generated.h"

class ULevelSequence;
class UMovieScene;
class UMovieSceneMaterialParameterCollectionTrack;
class UMovieScenePropertyTrack;
class USkyLightComponent;

/**
* Cached list of objects and tracks
*/
USTRUCT()
struct FCachedPropertyTrack
{
	GENERATED_BODY()

public:
	UPROPERTY()
		AActor* Actor;

	UPROPERTY()
		USceneComponent* Component;

	UPROPERTY()
		TArray<UMovieScenePropertyTrack*> Tracks;

	FCachedPropertyTrack() {}

	FCachedPropertyTrack(AActor* InActor, USceneComponent* InComponent, const TArray<UMovieScenePropertyTrack*> InTracks)
	{
		Actor = InActor;
		Component = InComponent;
		Tracks = InTracks;
	}
};

/**
* Container for mood state of object
*/
USTRUCT()
struct FObjectMood
{
	GENERATED_BODY()

public:
	UPROPERTY()
		bool bValid;

	UPROPERTY()
		bool bNewTransform;

	UPROPERTY()
		FTransform Transform;

	UPROPERTY()
		TMap<FName, float> Floats;

	UPROPERTY()
		TMap<FName, FLinearColor> Colors;

	FObjectMood() {}
};

/**
* Container for mood state of material param collection
*/
USTRUCT()
struct FCollectionMood
{
	GENERATED_BODY()

public:
	UPROPERTY()
		bool bValid;

	UPROPERTY()
		TMap<FName, float> Scalars;

	UPROPERTY()
		TMap<FName, FLinearColor> Colors;

	FCollectionMood() {}
};

UCLASS(Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent), CollapseCategories, HideCategories = (Activation, Collision, Cooking, Tags))
class UMoodBlenderComponent final : public UActorComponent
{
	GENERATED_UCLASS_BODY()

public:
	virtual void OnRegister() override;
	void CacheTracks();
	
	UFUNCTION(BlueprintPure, Category = Mood)
		USceneComponent* GetMoodComponent(const TSubclassOf<USceneComponent> Class);

	void Init();

	UFUNCTION(BlueprintCallable, Category = Mood)
		void RecaptureSky();

	UFUNCTION(BlueprintCallable, Category = Mood)
		void SetMood(const int32 NewTime, const bool bForce);

private:
	void CacheSequencerCollection(const UMovieSceneMaterialParameterCollectionTrack* Track);
	void CacheCurrentCollection(UMaterialParameterCollection* Collection);

	void CacheSequencerObject(UObject* Object, const TArray<UMovieScenePropertyTrack*> Tracks);
	void CacheCurrentObject(UObject * Object);

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void UpdateBlend();
	void UpdateCollection(UMaterialParameterCollection * Collection, FCollectionMood& NewState);
	void UpdateObject(UObject* Object, FObjectMood& NewState);

public:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = Mood, meta = (ClampMin = 0))
		int32 ForceTime;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = Mood)
		bool bResetTime;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = Mood)
		int32 CurrentFrame;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = Mood)
		FFrameTime CurrentFrameTime;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = Mood)
		FFrameNumber CurrentFrameNumber;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = Mood, meta = (ClampMin = 0.0f))
		float BlendTime = 1.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = Mood)
		ULevelSequence* MoodSequence;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = Mood)
		bool bRecaptureSkyEveryFrame = true;

	// if value > 0.0f, use it to trigger recapture after initializing the game
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = Mood)
		float FirstRecaptureDelay = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = Mood, Transient)
		UMovieScene* MoodMovie;

	UPROPERTY(Transient)
		TArray<UMovieSceneMaterialParameterCollectionTrack*> CollectionTracks;

	UPROPERTY(Transient)
		TMap<UObject*, FCachedPropertyTrack> ObjectTracks;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = Mood, AdvancedDisplay, Transient)
		USkyLightComponent* SkyLightComponent;

public:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = Mood, AdvancedDisplay, Transient)
		bool bBlending;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = Mood, AdvancedDisplay, Transient)
		float CurrentBlendTime;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = Mood, AdvancedDisplay, Transient)
		float BlendAlpha;

private:
	TMap<UObject*, FObjectMood> OriginalObjectStates;
	TMap<UObject*, FObjectMood> OldObjectStates;
	TMap<UObject*, FObjectMood> NewObjectStates;

	TMap<UMaterialParameterCollection*, FCollectionMood> OriginalCollectionStates;
	TMap<UMaterialParameterCollection*, FCollectionMood> OldCollectionStates;
	TMap<UMaterialParameterCollection*, FCollectionMood> NewCollectionStates;
};
