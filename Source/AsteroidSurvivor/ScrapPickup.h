// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ScrapPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPointLightComponent;

/**
 * Collectible scrap item dropped by destroyed enemy ships.
 *
 * Behaviour:
 * - Scatters outward from the destroyed enemy with an initial drift velocity.
 * - Drift velocity decays over time (drag).
 * - When the player ship enters the pull radius, the scrap accelerates
 *   toward the ship and is collected on overlap.
 * - Collected scrap is added to the player's scrap counter in the GameMode.
 * - Auto-destroys after a timeout if not collected.
 *
 * Visually distinguished from Thorium pickups by an orange/gold colour.
 */
UCLASS()
class ASTEROIDSURVIVOR_API AScrapPickup : public AActor
{
	GENERATED_BODY()

public:
	AScrapPickup();

	virtual void Tick(float DeltaTime) override;

	/** Initialise the pickup with scrap amount and scatter velocity. */
	void InitPickup(int32 InScrapAmount, const FVector& InDriftVelocity);

	/** Returns the scrap value this pickup grants on collection. */
	int32 GetScrapAmount() const { return ScrapAmount; }

protected:
	virtual void BeginPlay() override;

	// ── Components ──────────────────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionSphere = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* PickupMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPointLightComponent* GlowLight = nullptr;

	// ── Configuration ───────────────────────────────────────────────────────
	/** Distance at which the pickup starts being pulled toward the ship (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scrap")
	float PullRadius = 350.0f;

	/** Acceleration toward the ship when being pulled (cm/s²). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scrap")
	float PullAcceleration = 1800.0f;

	/** Maximum speed when being pulled toward the ship (cm/s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scrap")
	float MaxPullSpeed = 1000.0f;

	/** How long the pickup lingers before auto-destroying (seconds). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scrap")
	float Lifetime = 12.0f;

	/** Rate at which initial drift velocity decays (per second). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scrap")
	float DriftDrag = 3.0f;

private:
	int32 ScrapAmount = 1;
	FVector Velocity = FVector::ZeroVector;
	float LifeTimer = 0.0f;
	bool bBeingPulled = false;

	UFUNCTION()
	void OnPickupOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                          UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                          bool bFromSweep, const FHitResult& SweepResult);
};
