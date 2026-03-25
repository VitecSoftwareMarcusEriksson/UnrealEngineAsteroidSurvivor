// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AsteroidSurvivorAsteroid.generated.h"

class UStaticMeshComponent;
class USphereComponent;

/** Size category – determines radius, speed, health, score and split behaviour. */
UENUM(BlueprintType)
enum class EAsteroidSize : uint8
{
	Large  UMETA(DisplayName = "Large"),
	Medium UMETA(DisplayName = "Medium"),
	Small  UMETA(DisplayName = "Small")
};

/**
 * A drifting asteroid that can be destroyed by projectiles.
 *
 * Splitting rules:
 *   Large  -> 2 Medium
 *   Medium -> 2 Small
 *   Small  -> fully destroyed
 *
 * When two asteroids collide they both explode into smaller fragments
 * flying off in random directions.
 */
UCLASS()
class ASTEROIDSURVIVOR_API AAsteroidSurvivorAsteroid : public AActor
{
	GENERATED_BODY()

public:
	AAsteroidSurvivorAsteroid();

	virtual void Tick(float DeltaTime) override;

	/** Apply damage from a projectile. Destroys and optionally splits. */
	void TakeDamage_Asteroid(int32 DamageAmount);

	/** Size category – set by the spawner before FinishSpawning. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asteroid")
	EAsteroidSize AsteroidSize = EAsteroidSize::Large;

	/** Wave-based speed multiplier applied by the spawner. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Asteroid")
	float SpeedMultiplier = 1.0f;

	/** World-space drift direction (normalised). Set by the spawner. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Asteroid")
	FVector DriftDirection = FVector(1.0f, 0.0f, 0.0f);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionSphere = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* AsteroidMesh = nullptr;

	// -- Per-size tuning (Large) --
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Large")
	float LargeRadius = 120.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Large")
	float LargeSpeed = 150.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Large")
	int32 LargeHealth = 3;
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Large")
	int32 LargeScore = 20;

	// -- Per-size tuning (Medium) --
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Medium")
	float MediumRadius = 70.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Medium")
	float MediumSpeed = 250.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Medium")
	int32 MediumHealth = 2;
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Medium")
	int32 MediumScore = 50;

	// -- Per-size tuning (Small) --
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Small")
	float SmallRadius = 35.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Small")
	float SmallSpeed = 400.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Small")
	int32 SmallHealth = 1;
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Small")
	int32 SmallScore = 100;

	/** Class used when spawning child asteroids from splits/explosions. */
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid")
	TSubclassOf<AAsteroidSurvivorAsteroid> AsteroidClass;

private:
	/** Current health points. */
	int32 Health = 1;

	/** Drift speed in cm/s (set from size defaults * SpeedMultiplier). */
	float Speed = 150.0f;

	/** Score awarded when this asteroid is destroyed. */
	int32 ScoreValue = 20;

	/** Yaw-only spin rate (degrees/s) for visual tumble. */
	float YawSpinRate = 0.0f;

	/** True while this asteroid is in the process of exploding. */
	bool bExploding = false;

	/** Brief timer that prevents instant overlap with siblings after spawn. */
	float SpawnImmunityTimer = 0.0f;

	/** Configure radius, speed, health and score from the size category. */
	void ApplySizeParameters();

	/** Spawn child asteroids at +/-45 deg from drift (projectile kill). */
	void Split();

	/** Spawn 2-4 child asteroids in random directions (asteroid collision). */
	void ExplodeIntoFragments();

	/** Award score points via the game mode. */
	void NotifyGameMode() const;

	UFUNCTION()
	void OnAsteroidOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                            UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                            bool bFromSweep, const FHitResult& SweepResult);
};
