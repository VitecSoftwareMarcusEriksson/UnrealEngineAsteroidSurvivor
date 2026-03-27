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
 * - Gains new weapon types (ring burst, rapid burst) as difficulty scales.
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

	/**
	 * Scale boss stats based on spawn number. Called by WaveManager before InitEnemy.
	 * Each successive boss gets more HP, damage, volley count, new weapon types, etc.
	 * @param SpawnNumber  How many bosses have been spawned so far (1 for first boss).
	 */
	void ApplyDifficultyScaling(int32 SpawnNumber);

	/** Returns current move speed (may be modified by difficulty scaling). */
	float GetMoveSpeed() const { return MoveSpeed; }

protected:
	virtual void UpdateMovement(float DeltaTime, AAsteroidSurvivorShip* PlayerShip) override;

	/** Motherships do not avoid asteroids – they plow right through them. */
	virtual bool ShouldAvoidAsteroids() const override { return false; }

	/** Turn rate for chasing the player (degrees/s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Movement")
	float TurnRate = 35.0f;

	// ── Volley weapon (always available) ───────────────────────────────────
	/** Seconds between projectile volleys. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Weapon")
	float FireInterval = 1.5f;

	/** Number of projectiles per volley. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Weapon")
	int32 VolleyCount = 3;

	/** Spread angle for volley projectiles (degrees). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Weapon")
	float VolleySpread = 20.0f;

	// ── Ring burst weapon (unlocked at boss level 2+) ──────────────────────
	/** Whether the ring burst attack is enabled. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Weapon|Ring")
	bool bHasRingAttack = false;

	/** Number of projectiles in the ring burst. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Weapon|Ring")
	int32 RingProjectileCount = 12;

	/** Seconds between ring burst attacks. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Weapon|Ring")
	float RingFireInterval = 4.0f;

	// ── Rapid burst weapon (unlocked at boss level 3+) ─────────────────────
	/** Whether the rapid burst attack is enabled. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Weapon|Rapid")
	bool bHasRapidBurst = false;

	/** Number of rapid-fire shots in a burst. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Weapon|Rapid")
	int32 RapidBurstShotCount = 5;

	/** Seconds between each rapid-fire shot within a burst. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Weapon|Rapid")
	float RapidBurstShotInterval = 0.15f;

	/** Seconds between rapid burst attacks. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Weapon|Rapid")
	float RapidBurstCooldown = 5.0f;

	// ── Difficulty scaling parameters ───────────────────────────────────────
	/** HP multiplier increase per boss spawn (e.g. 0.5 = +50% per spawn). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Scaling")
	float HpScalePerSpawn = 0.5f;

	/** Contact damage multiplier increase per boss spawn. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Scaling")
	float DamageScalePerSpawn = 0.25f;

	/** Fire interval reduction factor per spawn (multiplied each time). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Scaling")
	float FireIntervalDecayRate = 0.82f;

	/** Minimum fire interval in seconds (floor for scaling). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Scaling")
	float MinFireInterval = 0.3f;

	/** Damage resistance increase per boss spawn (added each time). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Scaling")
	float DamageResistPerSpawn = 0.08f;

private:
	float FireTimer = 0.0f;
	float RingFireTimer = 0.0f;
	float RapidBurstTimer = 0.0f;
	int32 RapidBurstShotsRemaining = 0;
	float RapidBurstShotTimer = 0.0f;

	/** Fires a volley of projectiles toward the player. */
	void FireVolleyAtPlayer(AAsteroidSurvivorShip* PlayerShip);

	/** Fires projectiles in a ring/circle around the boss. */
	void FireRingAttack();

	/** Fires a single aimed shot at the player (used for rapid burst). */
	void FireAimedShot(AAsteroidSurvivorShip* PlayerShip);
};
