// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "WeaponUpgradePickup.h"
#include "AsteroidSurvivorShip.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UCameraComponent;
class USpringArmComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class AAsteroidSurvivorTrailParticle;
class AAsteroidSurvivorProjectile;

/**
 * The player-controlled spaceship in Asteroid Survivor.
 *
 * Movement: WASD / left stick – apply thrust in the ship's facing direction.
 * Rotation: Mouse / right stick – rotate the ship.
 * Fire:     Left mouse button / gamepad face button – shoot a projectile.
 *
 * The ship has a health-based damage system. Different asteroids deal
 * different amounts of damage. Collecting Thorium Energy from destroyed
 * asteroids allows the ship to level up and choose upgrades.
 */
UCLASS()
class ASTEROIDSURVIVOR_API AAsteroidSurvivorShip : public APawn
{
	GENERATED_BODY()

public:
	AAsteroidSurvivorShip();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// ── Health system ───────────────────────────────────────────────────────
	/** Apply damage to the ship. Returns true if the ship was destroyed. */
	bool ApplyDamageToShip(float DamageAmount);

	float GetCurrentHealth() const { return CurrentHealth; }
	float GetMaxHealth() const { return MaxHealth; }
	bool IsShieldActive() const { return bShieldActive; }

	// ── Upgrade multipliers (read by other systems) ─────────────────────────
	float GetThoriumMagnetMultiplier() const { return ThoriumMagnetMultiplier; }
	float GetScrapMagnetMultiplier() const { return ScrapMagnetMultiplier; }
	float GetDamageMultiplier() const { return DamageMultiplier; }
	float GetProjectileSizeMultiplier() const { return ProjectileSizeMultiplier; }
	bool HasExplosiveRounds() const { return bExplosiveRounds; }
	float GetExplosionRadius() const { return ExplosionRadius; }
	int32 GetExtraShotCount() const { return ExtraShotCount; }
	float GetCritChance() const { return CritChance; }
	float GetCritMultiplier() const { return CritMultiplier; }

	/** Apply an upgrade that increases max health and fully heals. */
	void UpgradeMaxHealth(float BonusHealth);

	/** Apply an upgrade that increases speed. */
	void UpgradeSpeed(float Multiplier);

	/** Apply an upgrade that increases turn rate. */
	void UpgradeTurnRate(float Multiplier);

	/** Apply an upgrade that adds or increases passive healing. */
	void UpgradePassiveHealing(float HealPerSecond);

	/** Apply an upgrade that grants or refreshes the shield. */
	void UpgradeShield();

	/** Apply an upgrade that increases fire rate. */
	void UpgradeFireRate(float Multiplier);

	/** Apply an upgrade that increases Thorium pull radius. */
	void UpgradeThoriumMagnet(float Multiplier);

	/** Apply an upgrade that increases projectile damage. */
	void UpgradeDamageBoost();

	/** Apply an upgrade that increases projectile size. */
	void UpgradeProjectileSize();

	/** Apply an upgrade that makes projectiles explode on hit. */
	void UpgradeExplosiveRounds();

	/** Apply an upgrade that fires an extra side-by-side projectile. */
	void UpgradeDoubleShot();

	/** Apply an upgrade that grants a critical hit chance. */
	void UpgradeCriticalHit();

	/** Apply an upgrade that increases scrap/pickup pull radius. */
	void UpgradeScrapMagnet(float Multiplier);

	/** Apply an upgrade that reduces incoming damage. */
	void UpgradeArmor();

	// ── Weapon arsenal system ───────────────────────────────────────────────
	/** Add a new weapon or upgrade an existing one. Called when a weapon pickup is collected. */
	void AddOrUpgradeWeapon(EWeaponType Type);

	/** Called by the GameMode when scrap is collected, to auto-enhance base weapons. */
	void OnScrapCollected(int32 TotalScrap);

	/** Returns the current weapon loadout (for HUD display). */
	const TMap<EWeaponType, int32>& GetWeaponArsenal() const { return WeaponArsenal; }

	/** Returns the current base blaster level (for HUD display). */
	int32 GetBlasterLevel() const { return BlasterLevel; }

	/** Returns whether the ship is currently firing (toggled on). */
	bool IsFiring() const { return bFiring; }

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;

	// ── Components ──────────────────────────────────────────────────────────
	/** Collision sphere root – provides reliable overlap detection. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionSphere = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ShipMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* SpringArm = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* Camera = nullptr;

	/** Visual shield sphere shown when shield is active. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ShieldMesh = nullptr;

	// ── Input ────────────────────────────────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* MoveAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* RotateAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* FireAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* SelectUpgrade1Action = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* SelectUpgrade2Action = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* SelectUpgrade3Action = nullptr;

	// ── Tuning ───────────────────────────────────────────────────────────────
	/** Base thrust force (cm/s²) before upgrades. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float BaseThrustForce = 600.0f;

	/** Base maximum speed cap (cm/s) before upgrades. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float BaseMaxSpeed = 900.0f;

	/** Velocity retention factor per frame at 60 FPS (0 = instant stop, 1 = no drag).
	 *  Applied in a frame-rate independent manner. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float Drag = 0.98f;

	/** Base rotation speed (degrees/s) before upgrades. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float BaseRotationSpeed = 200.0f;

	/** Base fire interval (seconds) before upgrades. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	float BaseFireRate = 0.25f;

	/** Muzzle offset from ship origin */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FVector MuzzleOffset = FVector(120.0f, 0.0f, 0.0f);

	/** Class of projectile to spawn */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<class AAsteroidSurvivorProjectile> ProjectileClass;

	// ── Health ───────────────────────────────────────────────────────────────
	/** Starting / base maximum health. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	float BaseMaxHealth = 100.0f;

	/** Duration of invulnerability after being hit (seconds) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	float InvulnerabilityDuration = 2.0f;

	/** Blink interval during invulnerability (seconds) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	float BlinkInterval = 0.15f;

	/** Seconds after shield breaks before it regenerates (if shield upgrade is active). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	float ShieldRechargeTime = 15.0f;

private:
	// ── Effective stats (base × upgrade multipliers) ────────────────────────
	float MaxHealth = 100.0f;
	float CurrentHealth = 100.0f;
	float SpeedMultiplier = 1.0f;
	float TurnRateMultiplier = 1.0f;
	float FireRateMultiplier = 1.0f;
	float PassiveHealRate = 0.0f;      // HP restored per second
	float ThoriumMagnetMultiplier = 1.0f;

	// New upgrade stats
	float DamageMultiplier = 1.0f;
	float ProjectileSizeMultiplier = 1.0f;
	bool bExplosiveRounds = false;
	float ExplosionRadius = 150.0f;
	int32 ExtraShotCount = 0;
	float CritChance = 0.0f;
	float CritMultiplier = 2.0f;
	float ScrapMagnetMultiplier = 1.0f;
	float DamageReduction = 0.0f;

	// Shield
	bool bHasShieldUpgrade = false;
	bool bShieldActive = false;
	float ShieldRechargeTimer = 0.0f;

	// Input handlers
	void Move(const FInputActionValue& Value);
	void Rotate(const FInputActionValue& Value);
	void StartFire();
	void StopFire();
	void Fire();
	void OnSelectUpgrade1();
	void OnSelectUpgrade2();
	void OnSelectUpgrade3();

	FVector Velocity = FVector::ZeroVector;
	float FireTimer = 0.0f;
	bool bFiring = false;

	// Homing missile cooldown (independent of the main fire rate)
	float HomingMissileTimer = 0.0f;
	float HomingMissileInterval = 1.0f;

	// Cooldowns for new weapons with independent fire rates
	float OrbitalDroneTimer = 0.0f;
	static constexpr float OrbitalDroneInterval = 0.5f;

	float PlasmaCannonTimer = 0.0f;
	static constexpr float PlasmaCannonInterval = 2.0f;

	float MineLauncherTimer = 0.0f;

	// Engine trail
	float TrailSpawnTimer = 0.0f;
	float TrailSpawnInterval = 0.03f;

	// Invulnerability state
	bool bInvulnerable = false;
	float InvulnerabilityTimer = 0.0f;
	float BlinkTimer = 0.0f;
	bool bBlinkVisible = true;

	void StartInvulnerability();

	/** Update the shield mesh visibility. */
	void UpdateShieldVisual();

	/** Overlap handler for collision */
	UFUNCTION()
	void OnShipOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                        bool bFromSweep, const FHitResult& SweepResult);

	/** Creates default Enhanced Input actions and mapping context in the constructor. */
	void SetupDefaultInputActions();

	/** Registers the Enhanced Input mapping context with the local player subsystem. */
	void RegisterInputMappingContext();

	// ── Weapon arsenal ──────────────────────────────────────────────────────
	/** Map of weapon type → upgrade level (1 = base, 2+ = enhanced, stacks without cap). */
	TMap<EWeaponType, int32> WeaponArsenal;

	/** Base blaster enhancement level. Increases every 5 scrap collected. */
	int32 BlasterLevel = 1;

	/** The last scrap threshold that triggered a blaster upgrade. */
	int32 LastScrapThreshold = 0;

	/** Fires extra weapon projectiles based on the current arsenal. */
	void FireExtraWeapons();

	/** Fires spread-shot projectiles (fan pattern). */
	void FireSpreadShot(int32 Level);

	/** Fires a rear turret projectile (backward). */
	void FireRearTurret(int32 Level);

	/** Fires a homing missile toward the nearest enemy. */
	void FireHomingMissile(int32 Level);

	/** Fires drone projectiles at positions orbiting the ship. */
	void FireOrbitalDrones(int32 Level);

	/** Fires a slow, large, high-damage plasma projectile. */
	void FirePlasmaCannon(int32 Level);

	/** Fires a burst of chain projectiles in the forward direction. */
	void FireLightningChain(int32 Level);

	/** Drops a stationary mine at the ship's rear. */
	void FireMineLauncher(int32 Level);

	/** Fires projectiles perpendicular to the ship (left and right). */
	void FireSideGuns(int32 Level);

	/** Applies damage multiplier, crit, and size upgrades to a freshly spawned projectile. */
	void ApplyProjectileUpgrades(AAsteroidSurvivorProjectile* Projectile) const;
};
