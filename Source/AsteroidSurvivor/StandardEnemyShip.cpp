// Copyright Epic Games, Inc. All Rights Reserved.

#include "StandardEnemyShip.h"
#include "AsteroidSurvivorShip.h"

AStandardEnemyShip::AStandardEnemyShip()
{
	// Standard enemy configuration
	EnemyType = EEnemyShipType::Standard;
	MaxHealth = 40.0f;
	ContactDamage = 15.0f;
	MoveSpeed = 150.0f;
	ScoreValue = 30;
	CollisionRadius = 35.0f;
	MeshScale = 0.6f;
	ShipColor = FLinearColor(12.0f, 0.3f, 0.3f, 1.0f); // Bright neon red
	ScrapDropCount = 1;
	ScrapPerPickup = 1;   // Small enemy, small drops
	WeaponDropChance = 0.03f; // 3% weapon drop chance
}

void AStandardEnemyShip::UpdateMovement(float DeltaTime, AAsteroidSurvivorShip* PlayerShip)
{
	if (PlayerShip)
	{
		// Gradually turn toward the player – limited by TurnRate
		const FVector ToPlayer = (PlayerShip->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		FVector DesiredDir = ToPlayer;

		// Blend in asteroid avoidance steering
		const FVector Avoidance = ComputeAsteroidAvoidance();
		if (!Avoidance.IsNearlyZero())
		{
			DesiredDir = (DesiredDir + Avoidance).GetSafeNormal();
		}

		// Use angular interpolation to smoothly turn
		const float MaxAngleThisFrame = TurnRate * DeltaTime;
		const float AngleBetween = FMath::RadiansToDegrees(
			FMath::Acos(FMath::Clamp(FVector::DotProduct(MoveDirection, DesiredDir), -1.0f, 1.0f)));

		if (AngleBetween > KINDA_SMALL_NUMBER)
		{
			const float Alpha = FMath::Clamp(MaxAngleThisFrame / AngleBetween, 0.0f, 1.0f);
			MoveDirection = FMath::Lerp(MoveDirection, DesiredDir, Alpha).GetSafeNormal();
		}
	}

	// Rotate the actor to face its movement direction
	if (!MoveDirection.IsNearlyZero())
	{
		SetActorRotation(MoveDirection.Rotation());
	}

	// Move forward
	FVector NewLocation = GetActorLocation() + MoveDirection * MoveSpeed * DeltaTime;
	SetActorLocation(NewLocation, true);
}
