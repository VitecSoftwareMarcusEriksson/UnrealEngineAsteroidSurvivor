// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AsteroidSurvivorBackground.generated.h"

class UInstancedStaticMeshComponent;

/**
 * Parallax scrolling star-field background with space dust.
 * Multiple layers of stars at different depths scroll at different speeds
 * relative to the player ship, providing visual feedback of movement.
 * A space dust layer adds atmospheric depth and vibrancy.
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

	/** Parallax factor for the space dust layer */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Background")
	float DustParallax = 0.08f;

	/** Parallax factor for the nebula layer */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Background")
	float NebulaParallax = 0.02f;

	/** Speed of the space dust layer's subtle drift animation */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Background|SpaceDust")
	float DustDriftSpeed = 15.0f;

	/** Horizontal oscillation frequency of the space dust drift */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Background|SpaceDust")
	float DustDriftFrequencyX = 0.3f;

	/** Vertical oscillation frequency of the space dust drift */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Background|SpaceDust")
	float DustDriftFrequencyY = 0.2f;

private:
	UPROPERTY()
	UInstancedStaticMeshComponent* FarStarsISM = nullptr;

	UPROPERTY()
	UInstancedStaticMeshComponent* MidStarsISM = nullptr;

	UPROPERTY()
	UInstancedStaticMeshComponent* NearStarsISM = nullptr;

	UPROPERTY()
	UInstancedStaticMeshComponent* SpaceDustISM = nullptr;

	UPROPERTY()
	UInstancedStaticMeshComponent* NebulaISM = nullptr;

	/** Cumulative drift offset for animating the space dust layer */
	float DustDriftTime = 0.0f;

	void CreateStarLayer(UInstancedStaticMeshComponent* ISM, int32 NumStars, float MinScale, float MaxScale);
	void CreateSpaceDustLayer(UInstancedStaticMeshComponent* ISM, int32 NumParticles);
	void CreateNebulaLayer(UInstancedStaticMeshComponent* ISM, int32 NumClouds);
	void ApplyLayerColor(UInstancedStaticMeshComponent* ISM, const FLinearColor& Color);
	void UpdateLayerPosition(UInstancedStaticMeshComponent* ISM, float ParallaxFactor, const FVector& ShipPos, float ZOffset);
};
