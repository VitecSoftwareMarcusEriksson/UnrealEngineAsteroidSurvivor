// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AsteroidSurvivorShip.generated.h"

class UStaticMeshComponent;
class UCameraComponent;
class USpringArmComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

/**
 * The player-controlled spaceship in Asteroid Survivor.
 *
 * Movement: WASD / left stick – apply thrust in the ship's facing direction.
 * Rotation: Mouse / right stick – rotate the ship.
 * Fire:     Left mouse button / gamepad face button – shoot a projectile.
 */
UCLASS()
class ASTEROIDSURVIVOR_API AAsteroidSurvivorShip : public APawn
{
	GENERATED_BODY()

public:
	AAsteroidSurvivorShip();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other,
	                        class UPrimitiveComponent* OtherComp, bool bSelfMoved,
	                        FVector HitLocation, FVector HitNormal,
	                        FVector NormalImpulse, const FHitResult& Hit) override;

	/** Called to apply damage to the ship */
	void TakeDamage_Ship(int32 DamageAmount);

protected:
	virtual void BeginPlay() override;

	// ── Components ──────────────────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ShipMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* SpringArm = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* Camera = nullptr;

	// ── Input ────────────────────────────────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* MoveAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* RotateAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* FireAction = nullptr;

	// ── Tuning ───────────────────────────────────────────────────────────────
	/** Maximum movement speed (cm/s) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float ThrustForce = 600.0f;

	/** Maximum speed cap (cm/s) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float MaxSpeed = 900.0f;

	/** Linear drag applied each frame (0 = no drag, 1 = instant stop) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float Drag = 0.98f;

	/** Rotation speed (degrees/s) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float RotationSpeed = 200.0f;

	/** Minimum interval between shots (seconds) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	float FireRate = 0.25f;

	/** Muzzle offset from ship origin */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FVector MuzzleOffset = FVector(120.0f, 0.0f, 0.0f);

	/** Class of projectile to spawn */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<class AAsteroidSurvivorProjectile> ProjectileClass;

	/** Maximum hit points */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	int32 MaxHealth = 3;

private:
	// Input handlers
	void Move(const FInputActionValue& Value);
	void Rotate(const FInputActionValue& Value);
	void StartFire();
	void StopFire();
	void Fire();

	FVector Velocity = FVector::ZeroVector;
	int32 Health = 0;
	float FireTimer = 0.0f;
	bool bFiring = false;

	void Die();

	/** Creates default Enhanced Input actions and mapping context in the constructor. */
	void SetupDefaultInputActions();
};
