// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AsteroidSurvivorAsteroid.h"
#include "AsteroidSurvivorAsteroidSpawner.generated.h"

/**
 * Spawns asteroids in waves just outside the camera view so they drift inward
 * toward the player.  Between waves, continuous spawning keeps the field busy.
 */
UCLASS()
class ASTEROIDSURVIVOR_API AAsteroidSurvivorAsteroidSpawner : public AActor
{
	GENERATED_BODY()

public:
	AAsteroidSurvivorAsteroidSpawner();

	virtual void Tick(float DeltaTime) override;

	/** Begin a new wave – called by the game mode. */
	void StartWave(int32 WaveNumber);

	/** Count every living asteroid in the world (including split fragments). */
	int32 GetActiveAsteroidCount() const;

protected:
	virtual void BeginPlay() override;

	/** Asteroid class to spawn (defaults to the C++ base class). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner")
	TSubclassOf<AAsteroidSurvivorAsteroid> AsteroidClass;

	/** Half-extent of the rectangular spawn border around the player (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner")
	float SpawnBorderHalfExtent = 2200.0f;

	/** Large asteroids spawned at wave start (wave 1). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner")
	int32 BaseAsteroidsPerWave = 8;

	/** Extra large asteroids added per wave after wave 1. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner")
	int32 AsteroidsPerWaveIncrement = 3;

	/** Per-wave speed multiplier increment (0.1 → +10 % per wave). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner")
	float SpeedScalePerWave = 0.1f;

	/** Minimum seconds between continuous spawns. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner")
	float ContinuousSpawnMinInterval = 1.5f;

	/** Maximum seconds between continuous spawns. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner")
	float ContinuousSpawnMaxInterval = 4.0f;

private:
	float ContinuousSpawnTimer = 0.0f;
	int32 CurrentSpeedWave = 1;

	/** Spawn a single asteroid of the given size on a random screen edge. */
	void SpawnAsteroid(EAsteroidSize Size, float SpeedMult);
};
