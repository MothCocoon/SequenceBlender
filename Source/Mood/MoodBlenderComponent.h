#pragma once

#include "Atmosphere/AtmosphericFog.h"
#include "CoreMinimal.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "LevelSequence.h"
#include "MovieSceneMaterialParameterCollectionTrack.h"
#include "MovieScenePropertyTrack.h"
#include "MoodBlenderComponent.generated.h"

/**
* Container for mood state of object
*/
USTRUCT()
struct FObjectMood
{
	GENERATED_BODY()

public:
	UPROPERTY()
		bool IsValid;

	UPROPERTY()
		bool IsNewTransform;

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
		bool IsValid;

	UPROPERTY()
		TMap<FName, float> Scalars;

	UPROPERTY()
		TMap<FName, FLinearColor> Colors;

	FCollectionMood() {}
};

UCLASS(ClassGroup = (Mood), Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent), CollapseCategories, HideCategories = (Activation, Collision, Cooking, Tags))
class UMoodBlenderComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	UMoodBlenderComponent();

	virtual void OnRegister() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = Mood)
		void SetMood(const int32 NewTime, const bool bForce);

private:
	void GetAllCollections(UMovieScene* MovieScene);
	void CacheCollection(const UMovieSceneMaterialParameterCollectionTrack * Track);
	void CacheSequencerCollection(const UMovieSceneMaterialParameterCollectionTrack* Track);
	void CacheCurrentCollection(UMaterialParameterCollection* Collection);

	void GetAllObjects(UMovieScene* MovieScene);
	void CacheObjects(const TArray<UMovieScenePropertyTrack*> Tracks, UObject* Object);
	void CacheSequencerObject(const TArray<UMovieScenePropertyTrack*> Tracks, UObject* Object);
	void CacheCurrentObject(UObject * Object);

	void UpdateBlend();
	void UpdateCollections(UMaterialParameterCollection * Collection, FCollectionMood& NewState);
	void UpdateObjects(UObject* Object, FObjectMood& NewState);

public:
	UFUNCTION(BlueprintCallable, Category = Mood)
		void RecaptureSky();

	void OnGameInitialized();

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

	//UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = Mood)
	//	APostProcessVolume* PostProcess;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = Mood)
		AAtmosphericFog* AtmosphericFog;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = Mood)
		AExponentialHeightFog* ExponentialFog;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = Mood)
		ADirectionalLight* DirectionalLight;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = Mood)
		ASkyLight* SkyLight;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = Mood)
		bool bRecaptureSkyEveryFrame = true;

	// if value > 0.0f, use it to trigger recapture after initializing the game
	// OnGameInitialized() must be called from custom game logic
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = Mood)
		float FirstRecaptureDelay = 0.0f;

public:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = Mood, AdvancedDisplay, Transient)
		bool bBlending;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = Mood, AdvancedDisplay, Transient)
		float CurrentBlendTime;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = Mood, AdvancedDisplay, Transient)
		float BlendAlpha;

private:
	TArray<UObject*> Objects;
	TMap<UObject*, FObjectMood> OriginalObjectStates;
	TMap<UObject*, FObjectMood> OldObjectStates;
	TMap<UObject*, FObjectMood> NewObjectStates;

	TArray<UMaterialParameterCollection*> Collections;
	TMap<UMaterialParameterCollection*, FCollectionMood> OriginalCollectionStates;
	TMap<UMaterialParameterCollection*, FCollectionMood> OldCollectionStates;
	TMap<UMaterialParameterCollection*, FCollectionMood> NewCollectionStates;
};
