// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShootingEnemyShip.h"
#include "EnemyProjectile.h"
#include "AsteroidSurvivorShip.h"
#include "AsteroidSurvivorGameMode.h"
#include "Kismet/GameplayStatics.h"

AShootingEnemyShip::AShootingEnemyShip()
{
	// Shooting enemy configuration – tougher, higher value
	EnemyType = EEnemyShipType::Shooting;
	MaxHealth = 80.0f;
	ContactDamage = 25.0f;
	MoveSpeed = 180.0f;
	ScoreValue = 80;
	CollisionRadius = 40.0f;
	MeshScale = 0.75f;
	ShipColor = FLinearColor(0.5f, 3.0f, 16.0f, 1.0f); // Bright neon electric-blue
	ScrapDropCount = 3;
	ScrapPerPickup = 2;   // Larger enemy, more scrap per drop
	WeaponDropChance = 0.10f; // 10% weapon drop chance

	// Randomise first shot delay so multiple shooters don't fire in sync
	FireTimer = FMath::FRandRange(0.0f, FireInterval);
}

void AShootingEnemyShip::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Freeze firing during upgrade selection (movement is already frozen in base class)
	AAsteroidSurvivorGameMode* GM = Cast<AAsteroidSurvivorGameMode>(
		UGameplayStatics::GetGameMode(this));
	if (GM && GM->IsSelectingUpgrade())
	{
		return;
	}

	// Firing logic (runs after UpdateMovement via Super::Tick)
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	AAsteroidSurvivorShip* Ship = Cast<AAsteroidSurvivorShip>(PlayerPawn);

	if (Ship)
	{
		FireTimer -= DeltaTime;
		if (FireTimer <= 0.0f)
		{
			FireAtPlayer(Ship);
			FireTimer = FireInterval;
		}
	}
}

void AShootingEnemyShip::UpdateMovement(float DeltaTime, AAsteroidSurvivorShip* PlayerShip)
{
	if (!PlayerShip)
	{
		// No player – drift in current direction
		FVector NewLocation = GetActorLocation() + MoveDirection * MoveSpeed * DeltaTime;
		SetActorLocation(NewLocation, true);
		return;
	}

	// Track the player with limited turn rate
	const FVector ToPlayer = (PlayerShip->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	FVector DesiredDir = ToPlayer;

	// Blend in asteroid avoidance steering
	const FVector Avoidance = ComputeAsteroidAvoidance();
	if (!Avoidance.IsNearlyZero())
	{
		DesiredDir = (DesiredDir + Avoidance).GetSafeNormal();
	}

	const float MaxAngleThisFrame = TurnRate * DeltaTime;
	const float AngleBetween = FMath::RadiansToDegrees(
		FMath::Acos(FMath::Clamp(FVector::DotProduct(MoveDirection, DesiredDir), -1.0f, 1.0f)));

	if (AngleBetween > KINDA_SMALL_NUMBER)
	{
		const float Alpha = FMath::Clamp(MaxAngleThisFrame / AngleBetween, 0.0f, 1.0f);
		MoveDirection = FMath::Lerp(MoveDirection, DesiredDir, Alpha).GetSafeNormal();
	}

	// Slow down when approaching the preferred engagement distance
	const float DistToPlayer = FVector::Dist(GetActorLocation(), PlayerShip->GetActorLocation());
	float SpeedFactor = 1.0f;
	if (DistToPlayer < PreferredDistance)
	{
		// Reduce speed as we get closer; stop at half the preferred distance
		SpeedFactor = FMath::Clamp(
			(DistToPlayer - PreferredDistance * 0.5f) / (PreferredDistance * 0.5f),
			0.0f, 1.0f);
	}

	// Rotate actor to face the player
	if (!MoveDirection.IsNearlyZero())
	{
		SetActorRotation(MoveDirection.Rotation());
	}

	FVector NewLocation = GetActorLocation() + MoveDirection * MoveSpeed * SpeedFactor * DeltaTime;
	SetActorLocation(NewLocation, true);
}

void AShootingEnemyShip::FireAtPlayer(AAsteroidSurvivorShip* PlayerShip)
{
	if (!PlayerShip)
	{
		return;
	}

	// Aim directly at the player's current position
	const FVector ToPlayer = (PlayerShip->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	const FRotator FireRotation = ToPlayer.Rotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector SpawnLoc = GetActorLocation() + ToPlayer * (CollisionRadius + 20.0f);

	// Projectile spawned – no further reference needed
	GetWorld()->SpawnActor<AEnemyProjectile>(
		AEnemyProjectile::StaticClass(), SpawnLoc, FireRotation, SpawnParams);
}
