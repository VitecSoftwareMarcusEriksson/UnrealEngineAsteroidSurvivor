// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyShipBase.h"
#include "WaveManager.generated.h"

class AAsteroidSurvivorShip;

/**
 * Describes the composition of a single enemy wave.
 * Each wave can contain a mix of different enemy ship types.
 */
USTRUCT(BlueprintType)
struct FWaveComposition
{
	GENERATED_BODY()

	/** Number of standard enemy ships in this wave. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 StandardCount = 3;

	/** Number of zigzag enemy ships in this wave. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 ZigzagCount = 0;

	/** Number of shooting enemy ships in this wave. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 ShootingCount = 0;
};

/**
 * Manages enemy wave spawning, boss timers, and difficulty scaling.
 *
 * The WaveManager is spawned by the GameMode and drives the enemy
 * encounter system:
 *
 * - Waves spawn at configurable intervals, with composition that
 *   scales in difficulty over time.
 * - Boss mother ships spawn on a separate 60-second timer with a
 *   visible countdown.
 * - Difficulty increases as the global game timer advances:
 *   more enemies per wave, tougher enemy types introduced over time.
 * - Enemy spawning pauses during upgrade selection.
 *
 * The global game timer is tracked here and exposed to the HUD.
 */
UCLASS()
class ASTEROIDSURVIVOR_API AWaveManager : public AActor
{
	GENERATED_BODY()

public:
	AWaveManager();

	virtual void Tick(float DeltaTime) override;

	// ── Accessors for HUD / GameMode ────────────────────────────────────────
	/** Returns the elapsed game time in seconds. */
	float GetElapsedTime() const { return ElapsedTime; }

	/** Returns seconds until the next boss spawn. */
	float GetBossCountdown() const { return BossSpawnInterval - BossTimer; }

	/** Returns the current wave number. */
	int32 GetCurrentWave() const { return CurrentWave; }

	/** Returns the current active boss (null if no boss alive). */
	AEnemyShipBase* GetActiveBoss() const { return ActiveBoss; }

	/** Pause/resume spawning (e.g. during upgrade selection). */
	void SetSpawningPaused(bool bPaused) { bSpawningPaused = bPaused; }

protected:
	virtual void BeginPlay() override;

	// ── Wave configuration ──────────────────────────────────────────────────
	/** Seconds between enemy waves. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Waves")
	float WaveInterval = 8.0f;

	/** Seconds between boss mother ship spawns. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Waves")
	float BossSpawnInterval = 60.0f;

	/** Base maximum number of enemy ships alive at once (excluding bosses). Scales up over time. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Waves")
	int32 BaseMaxEnemies = 25;

	/** Distance from the player at which enemies spawn (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Waves")
	float SpawnRadius = 2500.0f;

	/** Minimum number of zigzag enemies per group when they spawn. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Waves")
	int32 MinZigzagGroupSize = 5;

	/** Spacing between zigzag enemies in a line formation (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Waves")
	float ZigzagLineSpacing = 300.0f;

	/** How many additional enemies the cap increases per 60 seconds of elapsed time. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Waves")
	int32 EnemyCapIncreasePerMinute = 10;

	/** Angular spread (degrees, ±) for clustered enemy wave spawning. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Waves")
	float ClusterSpreadAngle = 20.0f;

	/** Radius variation (cm, ±) applied to clustered spawn positions. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Waves")
	float ClusterRadiusVariation = 200.0f;

private:
	float ElapsedTime = 0.0f;      // Global game timer
	float WaveTimer = 0.0f;        // Countdown to next wave
	float BossTimer = 0.0f;        // Countdown to next boss spawn
	int32 CurrentWave = 0;         // Wave counter
	int32 BossSpawnCount = 0;      // How many bosses have been spawned (for scaling)
	bool bSpawningPaused = false;

	UPROPERTY()
	AEnemyShipBase* ActiveBoss = nullptr;

	/** Returns the current max enemy cap, which scales up with elapsed time. */
	int32 GetCurrentMaxEnemies() const;

	/** Builds the wave composition based on the current difficulty level. */
	FWaveComposition BuildWaveForCurrentDifficulty() const;

	/** Spawns a complete wave of enemies. */
	void SpawnWave(const FWaveComposition& Composition);

	/** Spawns a boss mother ship with difficulty scaling. */
	void SpawnBoss();

	/** Spawns zigzag enemies in a line formation perpendicular to the player direction. */
	void SpawnZigzagFormation(int32 Count, int32& Budget);

	/** Spawns a group of enemies of the given type in a clustered wave from one direction. */
	void SpawnClusteredGroup(EEnemyShipType Type, int32 Count, int32& Budget);

	/** Spawns a single enemy of the given type at a location outside the camera. Returns the spawned enemy, or null on failure. */
	AEnemyShipBase* SpawnEnemyOfType(EEnemyShipType Type, const FVector& SpawnLocation, const FVector& DirectionToPlayer);

	/** Returns a random spawn location on a circle outside the camera view. */
	FVector GetSpawnLocationOutsideCamera() const;

	/** Counts the number of enemy ships currently alive in the world. */
	int32 CountActiveEnemies() const;
};
