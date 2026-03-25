// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPointLightComponent;

/**
 * Projectile fired by enemy ships (shooting ships, bosses).
 *
 * Travels in a straight line toward the player's last-known position.
 * Deals damage to the player ship on overlap, then self-destructs.
 * Ignores asteroids and other enemies.
 */
UCLASS()
class ASTEROIDSURVIVOR_API AEnemyProjectile : public AActor
{
	GENERATED_BODY()

public:
	AEnemyProjectile();

	virtual void Tick(float DeltaTime) override;

	/** Returns the damage this projectile deals to the player. */
	float GetDamage() const { return Damage; }

protected:
	virtual void BeginPlay() override;

	// ── Components ──────────────────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionSphere = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ProjectileMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPointLightComponent* GlowLight = nullptr;

	// ── Configuration ───────────────────────────────────────────────────────
	/** Projectile travel speed (cm/s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	float Speed = 1200.0f;

	/** Auto-destroy lifetime (seconds). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	float Lifetime = 3.0f;

	/** Damage dealt to the player ship on hit. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	float Damage = 15.0f;

private:
	float LifeTimer = 0.0f;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                    bool bFromSweep, const FHitResult& SweepResult);
};
