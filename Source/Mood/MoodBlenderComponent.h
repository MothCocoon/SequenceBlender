#pragma once

#include "CoreMinimal.h"
#include "MoodBlenderComponent.generated.h"

class ULevelSequence;
class UMovieScene;
class UMaterialParameterCollection;
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

	TWeakObjectPtr<AActor> Actor;
	TWeakObjectPtr<USceneComponent> Component;
	TArray<TWeakObjectPtr<UMovieScenePropertyTrack>> Tracks;

	FCachedPropertyTrack()
	{
		Actor = nullptr;
		Component = nullptr;
	}

	FCachedPropertyTrack(AActor* InActor, USceneComponent* InComponent, TArray<TWeakObjectPtr<UMovieScenePropertyTrack>> InTracks)
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

	bool bValid;
	bool bNewTransform;
	FTransform Transform;

	TMap<FFloatProperty*, float> Floats;
	TMap<FStructProperty*, FLinearColor> Colors;
	TMap<FStructProperty*, FLinearColor> LinearColors;

	FObjectMood() 
	{
		bValid = false;
		bNewTransform = false;
	}
};

/**
* Container for mood state of material param collection
*/
USTRUCT()
struct FCollectionMood
{
	GENERATED_BODY()

	bool bValid;
	TMap<FName, float> Scalars;
	TMap<FName, FLinearColor> Colors;

	FCollectionMood() 
	{
		bValid = false;
	}
};

UCLASS(Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent), CollapseCategories, HideCategories = (Activation, Collision, Cooking, Tags))
class UMoodBlenderComponent final : public UActorComponent
{
	GENERATED_UCLASS_BODY()

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

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = Mood, AdvancedDisplay, Transient)
	bool bBlending;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = Mood, AdvancedDisplay, Transient)
	float CurrentBlendTime;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = Mood, AdvancedDisplay, Transient)
	float BlendAlpha;

private:
	TWeakObjectPtr<UMovieScene> MoodMovie;

	TSet<TWeakObjectPtr<UMovieSceneMaterialParameterCollectionTrack>> CollectionTracks;
	TMap<TWeakObjectPtr<UMaterialParameterCollection>, FCollectionMood> OldCollectionStates;
	TMap<TWeakObjectPtr<UMaterialParameterCollection>, FCollectionMood> NewCollectionStates;

	TMap<TWeakObjectPtr<UObject>, FCachedPropertyTrack> ObjectTracks;
	TMap<TWeakObjectPtr<UObject>, FObjectMood> OldObjectStates;
	TMap<TWeakObjectPtr<UObject>, FObjectMood> NewObjectStates;

	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<USkyLightComponent> SkyLightComponent;

public:
	virtual void OnRegister() override;
	void CacheTracks();
	void CacheObjectTrack(UObject* Object, const FGuid& ObjectGuid);
	void GetPropertyTracks(const TWeakObjectPtr<UMovieScene>& MovieScene, const FGuid& ObjectGuid, TArray<TWeakObjectPtr<UMovieScenePropertyTrack>>& OutTracks) const;

	UFUNCTION(BlueprintPure, Category = Mood)
	USceneComponent* GetComponentFromSequence(const TSubclassOf<USceneComponent> Class);

	UFUNCTION(BlueprintCallable, Category = Mood)
	void SetMood(const int32 NewTime, const bool bForce);

private:
	void CacheCollection(UMovieSceneMaterialParameterCollectionTrack* Track);
	void CacheObject(UObject* Object, const FCachedPropertyTrack& Cache);

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void UpdateMood();
	void UpdateCollection(UMaterialParameterCollection* Collection, const FCollectionMood& NewState);
	void UpdateObject(UObject* Object, const FObjectMood& NewState, const FCachedPropertyTrack& CachedTrack);
};
