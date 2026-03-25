// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyShipBase.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class AAsteroidSurvivorShip;

/**
 * Enum identifying the type of enemy ship.
 * Used by the wave manager to configure spawns and by the HUD for display.
 */
UENUM(BlueprintType)
enum class EEnemyShipType : uint8
{
	Standard,       // Slow-moving, does not shoot
	Zigzag,         // Zigzag pattern toward player
	Shooting,       // Moves and shoots at the player
	Boss            // Mother ship – high HP, chases the player
};

/**
 * Base class for all enemy ships in Asteroid Survivor.
 *
 * Provides:
 * - Health system with configurable max HP
 * - Collision detection (responds to player projectiles)
 * - Scrap drop on destruction
 * - Contact damage to the player ship
 * - Virtual movement hooks for subclass-specific AI behaviour
 *
 * Subclasses override UpdateMovement() to implement unique flight patterns
 * (standard, zigzag, shooting, boss chase).
 */
UCLASS(Abstract)
class ASTEROIDSURVIVOR_API AEnemyShipBase : public AActor
{
	GENERATED_BODY()

public:
	AEnemyShipBase();

	virtual void Tick(float DeltaTime) override;

	/** Initialise the enemy with its spawn direction and movement speed. */
	void InitEnemy(const FVector& InDirection, float InSpeed);

	/** Apply damage from a player projectile. Returns true if the enemy was destroyed. */
	bool ApplyDamage(float DamageAmount);

	// ── Accessors ───────────────────────────────────────────────────────────
	EEnemyShipType GetEnemyType() const { return EnemyType; }
	float GetCurrentHealth() const { return CurrentHealth; }
	float GetMaxHealth() const { return MaxHealth; }
	float GetContactDamage() const { return ContactDamage; }

protected:
	virtual void BeginPlay() override;

	/**
	 * Called every frame to update movement.
	 * Subclasses must override this to implement their specific AI pattern.
	 * @param DeltaTime  Frame delta in seconds.
	 * @param PlayerShip The player's ship actor (may be null if dead).
	 */
	virtual void UpdateMovement(float DeltaTime, AAsteroidSurvivorShip* PlayerShip);

	// ── Components ──────────────────────────────────────────────────────────
	/** Collision sphere root – overlap detection for projectiles and player. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionSphere = nullptr;

	/** Visual mesh representing the enemy ship hull. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ShipMesh = nullptr;

	/** Glow light for visual flair. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPointLightComponent* GlowLight = nullptr;

	// ── Configuration ───────────────────────────────────────────────────────
	/** Type identifier – set by each subclass constructor. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	EEnemyShipType EnemyType = EEnemyShipType::Standard;

	/** Maximum hit points for this enemy. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	float MaxHealth = 50.0f;

	/** Damage dealt to the player ship on contact. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	float ContactDamage = 20.0f;

	/** Base movement speed (cm/s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Movement")
	float MoveSpeed = 200.0f;

	/** Score awarded to the player when this enemy is destroyed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	int32 ScoreValue = 50;

	/** Collision sphere radius (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	float CollisionRadius = 40.0f;

	/** Mesh scale multiplier. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	float MeshScale = 0.8f;

	/** Base emissive colour for the ship material. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Visual")
	FLinearColor ShipColor = FLinearColor(3.0f, 0.3f, 0.3f, 1.0f);

	/** Number of scrap pickups dropped on destruction. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Drops")
	int32 ScrapDropCount = 2;

	/** Amount of scrap per pickup. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Drops")
	int32 ScrapPerPickup = 1;

	/** Probability (0-1) that a weapon upgrade pickup drops. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Drops")
	float WeaponDropChance = 0.08f;

	/** Distance from the player at which this enemy auto-despawns (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	float DespawnDistance = 3500.0f;

	// ── Runtime state ───────────────────────────────────────────────────────
	float CurrentHealth = 0.0f;
	FVector MoveDirection = FVector::ForwardVector;
	float CurrentSpeed = 0.0f;

private:
	bool bExploding = false;

	/** Spawns scrap pickups at the enemy's location. */
	void SpawnScrapDrops();

	/** Possibly spawns a weapon upgrade pickup (based on WeaponDropChance). */
	void TrySpawnWeaponDrop();

	/** Handles overlap events for projectile hits and player contact. */
	UFUNCTION()
	void OnEnemyOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                         UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                         bool bFromSweep, const FHitResult& SweepResult);
};
