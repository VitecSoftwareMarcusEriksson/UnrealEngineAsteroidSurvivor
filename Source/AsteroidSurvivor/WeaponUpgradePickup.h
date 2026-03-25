// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponUpgradePickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPointLightComponent;

/**
 * Weapon types the player can acquire and upgrade.
 *
 * - SpreadShot:    Fires 3 (upgradeable to 5) projectiles in a fan pattern.
 * - RapidBlaster:  Significantly increases fire rate.
 * - RearTurret:    Fires a projectile backward in addition to the forward shot.
 * - HomingMissile: Fires a slow-tracking projectile toward the nearest enemy.
 */
UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	SpreadShot,
	RapidBlaster,
	RearTurret,
	HomingMissile
};

/**
 * A rare pickup dropped by destroyed enemy ships.
 *
 * When collected by the player:
 * - If the player does not have the weapon, it is added to their arsenal.
 * - If the player already has the weapon, its level is increased (stacking).
 *
 * Visually distinguished by a bright white/blue pulsing glow.
 */
UCLASS()
class ASTEROIDSURVIVOR_API AWeaponUpgradePickup : public AActor
{
	GENERATED_BODY()

public:
	AWeaponUpgradePickup();

	virtual void Tick(float DeltaTime) override;

	/** Initialise the pickup. Randomly selects a weapon type. */
	void InitPickup();

	/** Returns the weapon type this pickup grants. */
	EWeaponType GetWeaponType() const { return WeaponType; }

	/** Returns the display name for the contained weapon type. */
	static FString GetWeaponDisplayName(EWeaponType Type);

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
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponPickup")
	float PullRadius = 300.0f;

	/** Acceleration toward the ship when being pulled (cm/s²). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponPickup")
	float PullAcceleration = 1500.0f;

	/** Maximum speed when being pulled toward the ship (cm/s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponPickup")
	float MaxPullSpeed = 900.0f;

	/** How long the pickup lingers before auto-destroying (seconds). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponPickup")
	float Lifetime = 15.0f;

private:
	EWeaponType WeaponType = EWeaponType::SpreadShot;
	FVector Velocity = FVector::ZeroVector;
	float LifeTimer = 0.0f;
	bool bBeingPulled = false;

	UFUNCTION()
	void OnPickupOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                          UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                          bool bFromSweep, const FHitResult& SweepResult);
};
