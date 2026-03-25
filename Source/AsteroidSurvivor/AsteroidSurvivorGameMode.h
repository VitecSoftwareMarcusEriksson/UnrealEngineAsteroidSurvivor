// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AsteroidSurvivorGameMode.generated.h"

class AAsteroidSurvivorBackground;
class AAsteroidSurvivorAsteroid;
class AAsteroidSurvivorShip;
enum class EAsteroidSize : uint8;

/** Types of upgrades the player can choose on level-up. */
UENUM(BlueprintType)
enum class EUpgradeType : uint8
{
	MaxHealth,        // +25 max HP, heal to full
	SpeedBoost,       // +15% thrust & max speed
	TurnRate,         // +20% rotation speed
	PassiveHealing,   // +2 HP/s regeneration (stacks)
	Shield,           // Energy shield – absorbs one hit, recharges
	FireRate,         // +25% fire rate
	ThoriumMagnet     // +50% Thorium pull radius
};

/** Describes a single upgrade option presented during level-up. */
USTRUCT(BlueprintType)
struct FUpgradeOption
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	EUpgradeType Type = EUpgradeType::MaxHealth;

	UPROPERTY(BlueprintReadOnly)
	FString Name;

	UPROPERTY(BlueprintReadOnly)
	FString Description;
};

/**
 * Game mode for Asteroid Survivor.
 * Manages scoring, health, Thorium energy / leveling, upgrades, and win/lose
 * conditions.
 */
UCLASS(minimalapi)
class AAsteroidSurvivorGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AAsteroidSurvivorGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Called when the player ship is destroyed (health reached zero). */
	void OnPlayerShipDestroyed();

	/** Add points to the current score */
	void AddScore(int32 Points);

	/** Add Thorium energy. Triggers level-up when threshold is reached. */
	void AddThorium(int32 Amount);

	/** Player selected an upgrade option (index 0–2). */
	void SelectUpgrade(int32 Index);

	// ── Accessors ───────────────────────────────────────────────────────────
	int32 GetScore() const { return Score; }
	bool IsGameOver() const { return bGameOver; }

	int32 GetCurrentLevel() const { return CurrentLevel; }
	int32 GetCurrentThorium() const { return CurrentThorium; }
	int32 GetThoriumForNextLevel() const { return ThoriumForNextLevel; }

	bool IsSelectingUpgrade() const { return bSelectingUpgrade; }
	const TArray<FUpgradeOption>& GetCurrentUpgradeOptions() const { return CurrentUpgradeOptions; }

protected:
	// ── Thorium / leveling ─────────────────────────────────────────────────
	/** Base Thorium required for the first level-up. Each subsequent level
	 *  requires BaseThoriumPerLevel × CurrentLevel. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Leveling")
	int32 BaseThoriumPerLevel = 50;

	// ── Asteroid spawning ──────────────────────────────────────────────────
	/** Seconds between asteroid spawns */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Asteroids")
	float AsteroidSpawnInterval = 1.5f;

	/** Hard cap on the number of asteroids alive at once */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Asteroids")
	int32 MaxAsteroids = 30;

	/** Minimum travel speed for a newly spawned asteroid (cm/s) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Asteroids")
	float MinAsteroidSpeed = 100.0f;

	/** Maximum travel speed for a newly spawned asteroid (cm/s) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Asteroids")
	float MaxAsteroidSpeed = 350.0f;

private:
	int32 Score = 0;
	bool bGameOver = false;

	// Thorium / level state
	int32 CurrentLevel = 1;
	int32 CurrentThorium = 0;
	int32 ThoriumForNextLevel = 50;

	// Upgrade selection
	bool bSelectingUpgrade = false;
	TArray<FUpgradeOption> CurrentUpgradeOptions;

	UPROPERTY()
	AAsteroidSurvivorBackground* Background = nullptr;

	float AsteroidSpawnTimer = 0.0f;

	void TriggerGameOver();

	/** Spawns a single asteroid outside the camera view. */
	void SpawnAsteroid();

	/** Returns a world-space point on a circle outside the visible camera area. */
	FVector GetSpawnLocationOutsideCamera() const;

	/** Begins the upgrade-selection phase by picking 3 random upgrade options. */
	void PresentUpgradeOptions();

	/** Applies the chosen upgrade to the player ship. */
	void ApplyUpgrade(const FUpgradeOption& Upgrade);

	/** Builds the master list of all possible upgrade options. */
	static TArray<FUpgradeOption> BuildUpgradePool();

	/**
	 * Spawns a default directional light when the level has no lighting,
	 * so the game is visible even in an auto-generated empty map.
	 */
	void EnsureLightingExists();
};
