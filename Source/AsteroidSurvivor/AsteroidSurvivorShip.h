// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AsteroidSurvivorShip.generated.h"

class USceneComponent;
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

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;

	// ── Components ──────────────────────────────────────────────────────────
	/** Invisible root that keeps the actor oriented in the XY play-plane. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* SceneRoot = nullptr;

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

	/** Velocity retention factor per frame at 60 FPS (0 = instant stop, 1 = no drag).
	 *  Applied in a frame-rate independent manner. */
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

	/** Duration of invulnerability after being hit (seconds) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	float InvulnerabilityDuration = 3.0f;

	/** Blink interval during invulnerability (seconds) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	float BlinkInterval = 0.15f;

private:
	// Input handlers
	void Move(const FInputActionValue& Value);
	void Rotate(const FInputActionValue& Value);
	void StartFire();
	void StopFire();
	void Fire();

	FVector Velocity = FVector::ZeroVector;
	float FireTimer = 0.0f;
	bool bFiring = false;

	// Invulnerability state
	bool bInvulnerable = false;
	float InvulnerabilityTimer = 0.0f;
	float BlinkTimer = 0.0f;
	bool bBlinkVisible = true;

	void StartInvulnerability();

	/** Overlap handler for asteroid collision */
	UFUNCTION()
	void OnShipOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                        bool bFromSweep, const FHitResult& SweepResult);

	/** Creates default Enhanced Input actions and mapping context in the constructor. */
	void SetupDefaultInputActions();

	/** Registers the Enhanced Input mapping context with the local player subsystem. */
	void RegisterInputMappingContext();
};
