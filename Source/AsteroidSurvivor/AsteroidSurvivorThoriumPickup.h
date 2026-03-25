// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AsteroidSurvivorThoriumPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPointLightComponent;

/**
 * A collectible Thorium Energy pickup that spawns from destroyed asteroids.
 *
 * Behavior:
 * - Drifts outward from the destroyed asteroid's location.
 * - When the player ship enters the pull radius, the pickup accelerates
 *   toward the ship and is collected on overlap.
 * - Auto-destroys after a timeout if not collected.
 */
UCLASS()
class ASTEROIDSURVIVOR_API AAsteroidSurvivorThoriumPickup : public AActor
{
	GENERATED_BODY()

public:
	AAsteroidSurvivorThoriumPickup();

	virtual void Tick(float DeltaTime) override;

	/** Set the amount of Thorium energy this pickup grants and its initial drift velocity. */
	void InitPickup(int32 InThoriumAmount, const FVector& InDriftVelocity);

	/** Returns the Thorium amount this pickup grants on collection. */
	int32 GetThoriumAmount() const { return ThoriumAmount; }

protected:
	virtual void BeginPlay() override;

	// ── Components ──────────────────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionSphere = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* PickupMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* OuterGlowMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPointLightComponent* GlowLight = nullptr;

	// ── Tuning ──────────────────────────────────────────────────────────────
	/** Distance at which the pickup starts being pulled toward the ship (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Thorium")
	float PullRadius = 400.0f;

	/** Acceleration toward the ship when being pulled (cm/s²). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Thorium")
	float PullAcceleration = 2000.0f;

	/** Maximum speed when being pulled toward the ship (cm/s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Thorium")
	float MaxPullSpeed = 1200.0f;

	/** How long the pickup lingers before auto-destroying (seconds). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Thorium")
	float Lifetime = 10.0f;

	/** Rate at which the initial drift velocity decays (per second). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Thorium")
	float DriftDrag = 3.0f;

private:
	int32 ThoriumAmount = 5;
	FVector Velocity = FVector::ZeroVector;
	float LifeTimer = 0.0f;
	bool bBeingPulled = false;

	UFUNCTION()
	void OnPickupOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                          UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                          bool bFromSweep, const FHitResult& SweepResult);
};
