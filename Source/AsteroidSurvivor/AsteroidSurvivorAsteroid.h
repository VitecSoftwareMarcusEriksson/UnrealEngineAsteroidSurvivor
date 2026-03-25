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
 * - Colliding with the player ship deals damage based on asteroid size.
 * - Hit by a projectile: splits into smaller asteroids (or is destroyed
 *   if already Small). May drop Thorium Energy pickups.
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

	/** Returns the damage this asteroid deals to the player ship on collision. */
	float GetDamageAmount() const;

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

	// ── Damage per size ─────────────────────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Asteroid|Damage")
	float SmallDamage = 15.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Asteroid|Damage")
	float MediumDamage = 25.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Asteroid|Damage")
	float LargeDamage = 40.0f;

	// ── Thorium drops ───────────────────────────────────────────────────────
	/** Probability (0-1) that this asteroid contains Thorium energy. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Asteroid|Thorium")
	float ThoriumDropChance = 0.6f;

	/** Thorium energy per pickup for a Small asteroid. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Asteroid|Thorium")
	int32 SmallThoriumPerPickup = 3;

	/** Thorium energy per pickup for a Medium asteroid. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Asteroid|Thorium")
	int32 MediumThoriumPerPickup = 5;

	/** Thorium energy per pickup for a Large asteroid. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Asteroid|Thorium")
	int32 LargeThoriumPerPickup = 8;

private:
	EAsteroidSize AsteroidSize = EAsteroidSize::Large;
	FVector Velocity = FVector::ZeroVector;
	float GraceTimer = 0.0f;
	bool bExploding = false;
	bool bContainsThorium = false;

	/** Returns the collision sphere radius for the current size. */
	float GetCollisionRadius() const;

	/** Returns the mesh scale for the current size. */
	float GetMeshScale() const;

	/** Applies collision radius and mesh scale based on AsteroidSize. */
	void ApplySizeProperties();

	/** Splits into smaller asteroids (or simply destroys if Small).
	 *  @param bDropThorium  If true (projectile hit), spawns Thorium pickups. */
	void Explode(bool bDropThorium = false);

	/** Spawns Thorium Energy pickups at the asteroid's location. */
	void SpawnThoriumPickups();

	UFUNCTION()
	void OnAsteroidOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                            UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                            bool bFromSweep, const FHitResult& SweepResult);
};
