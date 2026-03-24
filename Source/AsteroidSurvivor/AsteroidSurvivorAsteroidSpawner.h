// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AsteroidSurvivorAsteroid.h"
#include "AsteroidSurvivorAsteroidSpawner.generated.h"

/**
 * Manages wave-based asteroid spawning.
 * Asteroids are spawned just outside the visible play area and drift inward.
 */
UCLASS()
class ASTEROIDSURVIVOR_API AAsteroidSurvivorAsteroidSpawner : public AActor
{
	GENERATED_BODY()

public:
	AAsteroidSurvivorAsteroidSpawner();

	virtual void Tick(float DeltaTime) override;

	/** Called by the game mode to begin a new wave */
	void StartWave(int32 WaveNumber);

	/** Returns the number of asteroids still alive */
	int32 GetActiveAsteroidCount() const;

protected:
	virtual void BeginPlay() override;

	/** Class to spawn – set this in your Blueprint subclass */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner")
	TSubclassOf<AAsteroidSurvivorAsteroid> AsteroidClass;

	/** Half-extent of the spawn zone border (cm) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner")
	float SpawnBorderHalfExtent = 2200.0f;

	/** Number of large asteroids spawned per wave (scales with wave number) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner")
	int32 BaseAsteroidsPerWave = 3;

	/** How many additional asteroids each new wave adds */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner")
	int32 AsteroidsPerWaveIncrement = 2;

	/** Speed multiplier applied to asteroid drift each wave */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner")
	float SpeedScalePerWave = 0.1f;

private:
	UPROPERTY()
	TArray<AAsteroidSurvivorAsteroid*> ActiveAsteroids;

	void SpawnAsteroid(EAsteroidSize Size, float SpeedMultiplier);
	void CleanDestroyedRefs();
};
