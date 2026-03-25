// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AsteroidSurvivorAsteroid.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/** Size categories for asteroids. Ordered small → large so enum comparisons work. */
UENUM(BlueprintType)
enum class EAsteroidSize : uint8
{
	Small,
	Medium,
	Large
};

/**
 * An asteroid that drifts through space at a constant velocity.
 *
 * - Spawned outside the camera view and moves inward.
 * - Colliding with another asteroid of equal or greater size causes this
 *   asteroid to explode into smaller fragments.
 * - Colliding with the player ship causes the ship to lose a life;
 *   the asteroid is destroyed (handled by the ship's overlap logic).
 * - Hit by a projectile: splits into smaller asteroids (or is destroyed
 *   if already Small).
 * - Despawns when it moves too far from the player.
 */
UCLASS()
class ASTEROIDSURVIVOR_API AAsteroidSurvivorAsteroid : public AActor
{
	GENERATED_BODY()

public:
	AAsteroidSurvivorAsteroid();

	virtual void Tick(float DeltaTime) override;

	/** Set the asteroid's size, travel direction and speed. Call once after spawning. */
	void InitAsteroid(EAsteroidSize InSize, const FVector& Direction, float InSpeed);

	/** Returns the size category of this asteroid. */
	EAsteroidSize GetAsteroidSize() const { return AsteroidSize; }

protected:
	virtual void BeginPlay() override;

	// ── Components ──────────────────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionSphere = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* AsteroidMesh = nullptr;

	// ── Tuning ──────────────────────────────────────────────────────────────
	/** Maximum distance from the player ship before this asteroid despawns (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Asteroid")
	float DespawnDistance = 3000.0f;

	/** Number of child asteroids spawned when this asteroid splits. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Asteroid")
	int32 SplitCount = 2;

	/** Brief period after spawning during which asteroid-asteroid collisions
	 *  are ignored, preventing chain-reaction explosions from freshly split
	 *  fragments. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Asteroid")
	float SpawnGracePeriod = 0.5f;

private:
	EAsteroidSize AsteroidSize = EAsteroidSize::Large;
	FVector Velocity = FVector::ZeroVector;
	float GraceTimer = 0.0f;
	bool bExploding = false;

	/** Returns the collision sphere radius for the current size. */
	float GetCollisionRadius() const;

	/** Returns the mesh scale for the current size. */
	float GetMeshScale() const;

	/** Applies collision radius and mesh scale based on AsteroidSize. */
	void ApplySizeProperties();

	/** Splits into smaller asteroids (or simply destroys if Small). */
	void Explode();

	UFUNCTION()
	void OnAsteroidOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                            UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                            bool bFromSweep, const FHitResult& SweepResult);
};
