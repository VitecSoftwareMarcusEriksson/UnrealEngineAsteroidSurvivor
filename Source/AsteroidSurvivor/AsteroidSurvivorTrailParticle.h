// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AsteroidSurvivorTrailParticle.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;

/**
 * A lightweight trail particle spawned behind the player ship's engines.
 * Shrinks and fades over its short lifetime, then self-destructs.
 */
UCLASS()
class ASTEROIDSURVIVOR_API AAsteroidSurvivorTrailParticle : public AActor
{
	GENERATED_BODY()

public:
	AAsteroidSurvivorTrailParticle();

	virtual void Tick(float DeltaTime) override;

	/** Override the particle color (e.g. gray smoke for homing missiles). */
	void SetSmokeColor(const FLinearColor& BaseColor);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ParticleMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPointLightComponent* TrailLight = nullptr;

	/** How long this particle lives (seconds). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trail")
	float Lifetime = 0.4f;

	/** Initial scale of the particle mesh. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trail")
	float InitialScale = 0.2f;

	/** Initial light intensity. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trail")
	float InitialLightIntensity = 10000.0f;

private:
	float LifeTimer = 0.0f;
};
