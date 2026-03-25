// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AsteroidSurvivorProjectile.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class UPointLightComponent;

/**
 * Projectile fired by the player's ship.
 * Moves in a straight line and destroys itself on hit or after a lifetime.
 */
UCLASS()
class ASTEROIDSURVIVOR_API AAsteroidSurvivorProjectile : public AActor
{
	GENERATED_BODY()

public:
	AAsteroidSurvivorProjectile();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	/** Collision sphere */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionSphere = nullptr;

	/** Visual representation of the projectile */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ProjectileMesh = nullptr;

	/** Point light for projectile glow effect */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPointLightComponent* GlowLight = nullptr;

	/** Speed of the projectile in cm/s */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	float Speed = 1800.0f;

	/** Seconds before the projectile auto-destroys */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	float Lifetime = 2.0f;

	/** Amount of damage dealt to an asteroid on hit */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	int32 Damage = 1;

private:
	float LifeTimer = 0.0f;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                    bool bFromSweep, const FHitResult& SweepResult);
};
