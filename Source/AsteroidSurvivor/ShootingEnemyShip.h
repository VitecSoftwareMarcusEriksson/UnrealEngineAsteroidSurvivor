// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnemyShipBase.h"
#include "ShootingEnemyShip.generated.h"

class AEnemyProjectile;

/**
 * Shooting enemy ship – moves toward the player AND fires projectiles.
 *
 * Behaviour:
 * - Approaches the player at moderate speed.
 * - Maintains a preferred engagement distance; slows down when close.
 * - Fires red/orange projectiles at the player at a configurable interval.
 * - More health than standard or zigzag ships.
 *
 * The ship leads its shots slightly based on the player's velocity
 * for more challenging gameplay.
 */
UCLASS()
class ASTEROIDSURVIVOR_API AShootingEnemyShip : public AEnemyShipBase
{
	GENERATED_BODY()

public:
	AShootingEnemyShip();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void UpdateMovement(float DeltaTime, AAsteroidSurvivorShip* PlayerShip) override;

	/** Seconds between shots. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Weapon")
	float FireInterval = 2.0f;

	/** Preferred distance to the player before slowing down (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Movement")
	float PreferredDistance = 600.0f;

	/** Turn rate for tracking the player (degrees/s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Movement")
	float TurnRate = 50.0f;

	/** Projectile speed for enemy bullets (cm/s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Weapon")
	float ProjectileSpeed = 1200.0f;

private:
	float FireTimer = 0.0f;

	/** Fires a projectile toward the player ship. */
	void FireAtPlayer(AAsteroidSurvivorShip* PlayerShip);
};
