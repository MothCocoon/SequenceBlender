#pragma once

#include "CoreMinimal.h"
#include "SequenceBlenderComponent.generated.h"

class ULevelSequence;
class UMovieScene;
class UMaterialParameterCollection;
class UMovieSceneMaterialParameterCollectionTrack;
class UMovieScenePropertyTrack;

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
* Container for the blend state of objects
*/
USTRUCT()
struct FObjectBlendState
{
	GENERATED_BODY()

	bool bValid;
	bool bNewTransform;
	FTransform Transform;

	TMap<FFloatProperty*, float> Floats;
	TMap<FStructProperty*, FLinearColor> Colors;
	TMap<FStructProperty*, FLinearColor> LinearColors;

	FObjectBlendState() 
	{
		bValid = false;
		bNewTransform = false;
	}
};

/**
* Container for the blend state of material param collection
*/
USTRUCT()
struct FShaderBlendState
{
	GENERATED_BODY()

	bool bValid;
	TMap<FName, float> Scalars;
	TMap<FName, FLinearColor> Colors;

	FShaderBlendState() 
	{
		bValid = false;
	}
};

UCLASS(Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent), CollapseCategories, HideCategories = (Activation, Collision, Cooking, Tags))
class USequenceBlenderComponent final : public UActorComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Sequence Blend", meta = (ClampMin = 0))
	int32 ForceTime;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Sequence Blend")
	bool bResetTime;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sequence Blend")
	int32 CurrentFrame;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sequence Blend")
	FFrameTime CurrentFrameTime;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sequence Blend")
	FFrameNumber CurrentFrameNumber;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Sequence Blend", meta = (ClampMin = 0.0f))
	float BlendTime;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Sequence Blend")
	ULevelSequence* Sequence;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sequence Blend", AdvancedDisplay, Transient)
	bool bBlending;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sequence Blend", AdvancedDisplay, Transient)
	float CurrentBlendTime;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sequence Blend", AdvancedDisplay, Transient)
	float BlendAlpha;

private:
	TWeakObjectPtr<UMovieScene> MovieScene;

	TSet<TWeakObjectPtr<UMovieSceneMaterialParameterCollectionTrack>> CollectionTracks;
	TMap<TWeakObjectPtr<UMaterialParameterCollection>, FShaderBlendState> OldCollectionStates;
	TMap<TWeakObjectPtr<UMaterialParameterCollection>, FShaderBlendState> NewCollectionStates;

	TMap<TWeakObjectPtr<UObject>, FCachedPropertyTrack> ObjectTracks;
	TMap<TWeakObjectPtr<UObject>, FObjectBlendState> OldObjectStates;
	TMap<TWeakObjectPtr<UObject>, FObjectBlendState> NewObjectStates;

	TWeakObjectPtr<UWorld> World;

public:
	virtual void OnRegister() override;
	void CacheTracks();
	void CacheObjectTrack(UObject* Object, const FGuid& ObjectGuid);
	void GetPropertyTracks(const FGuid& ObjectGuid, TArray<TWeakObjectPtr<UMovieScenePropertyTrack>>& OutTracks) const;

	UFUNCTION(BlueprintCallable, Category = "Sequence Blend")
	void SetFrame(const int32 NewTime, const bool bForce);

private:
	void CacheCollection(const UMovieSceneMaterialParameterCollectionTrack* Track);
	void CacheObject(UObject* Object, const FCachedPropertyTrack& Cache);

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void UpdateBlendState();
	void UpdateCollection(UMaterialParameterCollection* Collection, const FShaderBlendState& NewState);
	void UpdateObject(UObject* Object, const FObjectBlendState& NewState, const FCachedPropertyTrack& CachedTrack);

	void OnBlendCompleted();
};
