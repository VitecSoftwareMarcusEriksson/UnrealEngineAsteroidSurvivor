// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AsteroidSurvivorGameMode.generated.h"

class AAsteroidSurvivorAsteroidSpawner;
class AAsteroidSurvivorBackground;

/**
 * Game mode for Asteroid Survivor.
 * Manages wave progression, scoring, lives, and win/lose conditions.
 */
UCLASS(minimalapi)
class AAsteroidSurvivorGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AAsteroidSurvivorGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Called when the player ship is destroyed */
	void OnPlayerShipDestroyed();

	/** Called when an asteroid is destroyed, awards points */
	void OnAsteroidDestroyed(int32 Points);

	/** Returns current score */
	int32 GetScore() const { return Score; }

	/** Returns remaining lives */
	int32 GetLives() const { return Lives; }

	/** Returns current wave number */
	int32 GetWave() const { return CurrentWave; }

	/** Returns whether the game is over */
	bool IsGameOver() const { return bGameOver; }

protected:
	/** Initial number of lives the player starts with */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Rules")
	int32 InitialLives = 3;

	/** Delay in seconds before respawning the player after death */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Rules")
	float RespawnDelay = 2.0f;

	/** Delay in seconds before starting the next wave */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Rules")
	float NextWaveDelay = 3.0f;

private:
	int32 Score = 0;
	int32 Lives = 0;
	int32 CurrentWave = 1;
	bool bGameOver = false;
	bool bWaitingForRespawn = false;
	bool bWaitingForNextWave = false;
	float RespawnTimer = 0.0f;
	float NextWaveTimer = 0.0f;

	UPROPERTY()
	AAsteroidSurvivorAsteroidSpawner* AsteroidSpawner = nullptr;

	UPROPERTY()
	AAsteroidSurvivorBackground* Background = nullptr;

	void StartWave(int32 WaveNumber);
	void TriggerGameOver();
	void RespawnPlayer();

	/**
	 * Spawns a default directional light when the level has no lighting,
	 * so the game is visible even in an auto-generated empty map.
	 */
	void EnsureLightingExists();
};
