// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AsteroidSurvivorProjectile.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class UPointLightComponent;
class AAsteroidSurvivorTrailParticle;

/**
 * Projectile fired by the player's ship.
 * Moves in a straight line and destroys itself on hit or after a lifetime.
 */
UCLASS()
class ASTEROIDSURVIVOR_API AAsteroidSurvivorProjectile : public AActor
{
	GENERATED_BODY()

public:
	AAsteroidSurvivorProjectile();

	virtual void Tick(float DeltaTime) override;

	/**
	 * Mark this projectile as a homing missile.
	 * Scales the mesh up, changes colors to orange-red, and enables smoke trail.
	 */
	void SetHomingMissile(bool bHoming);

	/** Override the damage this projectile deals (for crits, damage boost). */
	void SetDamage(int32 NewDamage);

	/** Override the projectile's travel speed in cm/s (e.g. 0 for a stationary mine). */
	void SetSpeed(float NewSpeed);

	/** Scale the projectile mesh relative to its default size. */
	void SetScaleMultiplier(float Mult);

	/** Mark this projectile as explosive. On impact it deals splash damage within InRadius. */
	void SetExplosiveRounds(bool bHasExplosion, float InRadius);

protected:
	virtual void BeginPlay() override;

	/** Collision sphere */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionSphere = nullptr;

	/** Visual representation of the projectile */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ProjectileMesh = nullptr;

	/** Point light for projectile glow effect */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPointLightComponent* GlowLight = nullptr;

	/** Speed of the projectile in cm/s */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	float Speed = 1800.0f;

	/** Seconds before the projectile auto-destroys */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	float Lifetime = 2.0f;

	/** Amount of damage dealt on hit */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	int32 Damage = 25;

public:
	/** Returns the damage this projectile deals. */
	int32 GetDamage() const { return Damage; }

private:
	float LifeTimer = 0.0f;

	// Homing missile state
	bool bIsHomingMissile = false;
	float HomingMissileTrailTimer = 0.0f;
	static constexpr float HomingMissileTrailInterval = 0.05f;
	static constexpr float HomingMissileScale = 0.4f;
	static constexpr float HomingMissileTrailOffset = -30.0f;

	// Explosive rounds state
	bool bIsExplosive = false;
	float ExplosionRadius = 0.0f;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                    bool bFromSweep, const FHitResult& SweepResult);
};
