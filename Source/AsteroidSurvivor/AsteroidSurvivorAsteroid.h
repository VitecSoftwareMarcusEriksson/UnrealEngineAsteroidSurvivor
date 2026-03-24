// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AsteroidSurvivorAsteroid.generated.h"

class UStaticMeshComponent;
class USphereComponent;

/** Size category of an asteroid – determines scale, health, score and split behaviour */
UENUM(BlueprintType)
enum class EAsteroidSize : uint8
{
	Large  UMETA(DisplayName = "Large"),
	Medium UMETA(DisplayName = "Medium"),
	Small  UMETA(DisplayName = "Small")
};

/**
 * An asteroid that drifts through space.
 * When destroyed it may split into smaller asteroids.
 * Large  → splits into 2 Medium
 * Medium → splits into 2 Small
 * Small  → fully destroyed
 */
UCLASS()
class ASTEROIDSURVIVOR_API AAsteroidSurvivorAsteroid : public AActor
{
	GENERATED_BODY()

public:
	AAsteroidSurvivorAsteroid();

	virtual void Tick(float DeltaTime) override;

	/** Apply damage; destroys and optionally splits the asteroid */
	void TakeDamage_Asteroid(int32 DamageAmount);

	/** Size category – set before BeginPlay by the spawner */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asteroid")
	EAsteroidSize AsteroidSize = EAsteroidSize::Large;

	/** Current drift direction (world-space, normalised) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Asteroid")
	FVector DriftDirection = FVector(1.0f, 0.0f, 0.0f);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionSphere = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* AsteroidMesh = nullptr;

	// ── Tuning per size (set via data table or defaults in BeginPlay) ─────
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Large")
	float LargeRadius = 120.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Large")
	float LargeSpeed = 150.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Large")
	int32 LargeHealth = 3;
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Large")
	int32 LargeScore = 20;

	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Medium")
	float MediumRadius = 70.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Medium")
	float MediumSpeed = 250.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Medium")
	int32 MediumHealth = 2;
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Medium")
	int32 MediumScore = 50;

	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Small")
	float SmallRadius = 35.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Small")
	float SmallSpeed = 400.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Small")
	int32 SmallHealth = 1;
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Small")
	int32 SmallScore = 100;

	/** Class used when spawning split pieces */
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid")
	TSubclassOf<AAsteroidSurvivorAsteroid> AsteroidClass;

private:
	int32 Health = 1;
	float Speed = 150.0f;
	int32 ScoreValue = 20;
	float RotationSpeed = 0.0f;

	void ApplySizeParameters();
	void Split();
	void NotifyGameMode() const;
};
