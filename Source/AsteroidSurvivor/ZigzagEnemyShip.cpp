// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZigzagEnemyShip.h"
#include "AsteroidSurvivorShip.h"

AZigzagEnemyShip::AZigzagEnemyShip()
{
	// Zigzag enemy configuration – slightly tougher than standard
	EnemyType = EEnemyShipType::Zigzag;
	MaxHealth = 60.0f;
	ContactDamage = 20.0f;
	MoveSpeed = 200.0f;
	ScoreValue = 50;
	CollisionRadius = 35.0f;
	MeshScale = 0.65f;
	ShipColor = FLinearColor(3.0f, 1.5f, 0.2f, 1.0f); // Orange-yellow
	ScrapDropCount = 2;
	ScrapPerPickup = 1;
	WeaponDropChance = 0.05f; // 5% weapon drop chance

	// Randomise the starting phase so multiple zigzag ships don't move in sync
	ZigzagTimer = FMath::FRandRange(0.0f, 6.28f);
}

void AZigzagEnemyShip::UpdateMovement(float DeltaTime, AAsteroidSurvivorShip* PlayerShip)
{
	ZigzagTimer += DeltaTime;

	// Track the player with limited turn rate
	FVector ForwardDir = MoveDirection;
	if (PlayerShip)
	{
		const FVector ToPlayer = (PlayerShip->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		const float MaxAngleThisFrame = TurnRate * DeltaTime;
		const float AngleBetween = FMath::RadiansToDegrees(
			FMath::Acos(FMath::Clamp(FVector::DotProduct(MoveDirection, ToPlayer), -1.0f, 1.0f)));

		if (AngleBetween > KINDA_SMALL_NUMBER)
		{
			const float Alpha = FMath::Clamp(MaxAngleThisFrame / AngleBetween, 0.0f, 1.0f);
			ForwardDir = FMath::Lerp(MoveDirection, ToPlayer, Alpha).GetSafeNormal();
		}
	}
	MoveDirection = ForwardDir;

	// Compute lateral direction (perpendicular to forward, in the XY plane)
	FVector LateralDir = FVector(-ForwardDir.Y, ForwardDir.X, 0.0f);

	// Zigzag offset using a sine wave
	const float LateralOffset = FMath::Sin(ZigzagTimer * ZigzagFrequency * 2.0f * PI) * ZigzagAmplitude;
	const float LateralVelocity = FMath::Cos(ZigzagTimer * ZigzagFrequency * 2.0f * PI) *
	                               ZigzagAmplitude * ZigzagFrequency * 2.0f * PI;

	// Combined velocity: forward movement + lateral oscillation
	FVector CombinedVelocity = ForwardDir * MoveSpeed + LateralDir * LateralVelocity;

	// Rotate the actor to face movement direction
	if (!CombinedVelocity.IsNearlyZero())
	{
		SetActorRotation(CombinedVelocity.GetSafeNormal().Rotation());
	}

	FVector NewLocation = GetActorLocation() + CombinedVelocity * DeltaTime;
	SetActorLocation(NewLocation, true);
}
