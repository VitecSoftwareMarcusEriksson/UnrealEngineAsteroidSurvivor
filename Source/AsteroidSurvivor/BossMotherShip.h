// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnemyShipBase.h"
#include "BossMotherShip.generated.h"

/**
 * Boss Mother Ship – a large, powerful enemy that spawns every 60 seconds.
 *
 * Behaviour:
 * - Actively chases the player at moderate speed.
 * - Has a very large health pool (requires many hits to destroy).
 * - Larger collision radius and visual size than regular enemies.
 * - Higher contact damage.
 * - Drops large amounts of scrap and has a high weapon-drop chance.
 * - Fires volleys of projectiles at the player periodically.
 *
 * The WaveManager handles boss spawning on a separate 60-second timer.
 */
UCLASS()
class ASTEROIDSURVIVOR_API ABossMotherShip : public AEnemyShipBase
{
	GENERATED_BODY()

public:
	ABossMotherShip();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void UpdateMovement(float DeltaTime, AAsteroidSurvivorShip* PlayerShip) override;

	/** Turn rate for chasing the player (degrees/s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Movement")
	float TurnRate = 35.0f;

	/** Seconds between projectile volleys. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Weapon")
	float FireInterval = 1.5f;

	/** Number of projectiles per volley. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Weapon")
	int32 VolleyCount = 3;

	/** Spread angle for volley projectiles (degrees). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Weapon")
	float VolleySpread = 20.0f;

private:
	float FireTimer = 0.0f;

	/** Fires a volley of projectiles toward the player. */
	void FireVolleyAtPlayer(AAsteroidSurvivorShip* PlayerShip);
};
