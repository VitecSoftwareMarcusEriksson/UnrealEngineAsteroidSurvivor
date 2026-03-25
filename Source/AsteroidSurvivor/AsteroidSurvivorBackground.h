// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AsteroidSurvivorBackground.generated.h"

class UInstancedStaticMeshComponent;

/**
 * Parallax scrolling star-field background.
 * Three layers of stars at different depths scroll at different speeds
 * relative to the player ship, providing visual feedback of movement.
 */
UCLASS()
class ASTEROIDSURVIVOR_API AAsteroidSurvivorBackground : public AActor
{
	GENERATED_BODY()

public:
	AAsteroidSurvivorBackground();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	/** Size of the star field area per layer (units) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Background")
	float FieldSize = 6000.0f;

	/** Parallax factor for the far star layer (small = barely moves) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Background")
	float FarParallax = 0.05f;

	/** Parallax factor for the mid star layer */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Background")
	float MidParallax = 0.15f;

	/** Parallax factor for the near star layer */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Background")
	float NearParallax = 0.35f;

private:
	UPROPERTY()
	UInstancedStaticMeshComponent* FarStarsISM = nullptr;

	UPROPERTY()
	UInstancedStaticMeshComponent* MidStarsISM = nullptr;

	UPROPERTY()
	UInstancedStaticMeshComponent* NearStarsISM = nullptr;

	void CreateStarLayer(UInstancedStaticMeshComponent* ISM, int32 NumStars, float MinScale, float MaxScale);
	void UpdateLayerPosition(UInstancedStaticMeshComponent* ISM, float ParallaxFactor, const FVector& ShipPos, float ZOffset);
};
