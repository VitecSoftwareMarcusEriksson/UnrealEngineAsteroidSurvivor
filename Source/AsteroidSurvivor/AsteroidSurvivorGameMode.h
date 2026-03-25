// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AsteroidSurvivorGameMode.generated.h"

class AAsteroidSurvivorBackground;

/**
 * Game mode for Asteroid Survivor.
 * Manages scoring, lives, and win/lose conditions.
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

	/** Called when the player ship is hit (loses a life) */
	void OnPlayerShipHit();

	/** Returns current score */
	int32 GetScore() const { return Score; }

	/** Returns remaining lives */
	int32 GetLives() const { return Lives; }

	/** Returns whether the game is over */
	bool IsGameOver() const { return bGameOver; }

protected:
	/** Initial number of lives the player starts with */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Rules")
	int32 InitialLives = 3;

	/** Delay in seconds before respawning the player after death */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Rules")
	float RespawnDelay = 2.0f;

private:
	int32 Score = 0;
	int32 Lives = 0;
	bool bGameOver = false;
	bool bWaitingForRespawn = false;
	float RespawnTimer = 0.0f;

	UPROPERTY()
	AAsteroidSurvivorBackground* Background = nullptr;

	void TriggerGameOver();
	void RespawnPlayer();

	/**
	 * Spawns a default directional light when the level has no lighting,
	 * so the game is visible even in an auto-generated empty map.
	 */
	void EnsureLightingExists();
};
