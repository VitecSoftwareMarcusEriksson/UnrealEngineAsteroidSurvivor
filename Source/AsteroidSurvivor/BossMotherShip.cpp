// Copyright Epic Games, Inc. All Rights Reserved.

#include "BossMotherShip.h"
#include "EnemyProjectile.h"
#include "AsteroidSurvivorShip.h"
#include "AsteroidSurvivorGameMode.h"
#include "Kismet/GameplayStatics.h"

ABossMotherShip::ABossMotherShip()
{
	// Boss configuration – high HP, large size, valuable drops
	EnemyType = EEnemyShipType::Boss;
	MaxHealth = 500.0f;
	ContactDamage = 40.0f;
	MoveSpeed = 160.0f;
	ScoreValue = 500;
	CollisionRadius = 100.0f;
	MeshScale = 2.0f;
	ShipColor = FLinearColor(1.0f, 0.1f, 0.6f, 1.0f); // Bright neon hot-pink / magenta
	ScrapDropCount = 10;
	ScrapPerPickup = 4;   // Largest enemy, most scrap per drop
	WeaponDropChance = 0.50f; // 50% weapon drop chance
	DespawnDistance = 5000.0f; // Bosses linger longer

	// Stagger first volley
	FireTimer = FMath::FRandRange(0.5f, FireInterval);
}

void ABossMotherShip::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Freeze firing during upgrade selection (movement is already frozen in base class)
	AAsteroidSurvivorGameMode* GM = Cast<AAsteroidSurvivorGameMode>(
		UGameplayStatics::GetGameMode(this));
	if (GM && GM->IsSelectingUpgrade())
	{
		return;
	}

	// Firing logic
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	AAsteroidSurvivorShip* Ship = Cast<AAsteroidSurvivorShip>(PlayerPawn);

	if (Ship)
	{
		FireTimer -= DeltaTime;
		if (FireTimer <= 0.0f)
		{
			FireVolleyAtPlayer(Ship);
			FireTimer = FireInterval;
		}
	}
}

void ABossMotherShip::ApplyDifficultyScaling(int32 SpawnNumber)
{
	if (SpawnNumber <= 1)
	{
		// First boss uses default stats
		return;
	}

	// Each successive boss gets harder:
	// HP: +50% per spawn (boss 2 = 750, boss 3 = 1000, boss 4 = 1250, ...)
	const float HpMultiplier = 1.0f + 0.5f * (SpawnNumber - 1);
	MaxHealth *= HpMultiplier;
	CurrentHealth = MaxHealth;

	// Contact damage: +25% per spawn
	const float DamageMultiplier = 1.0f + 0.25f * (SpawnNumber - 1);
	ContactDamage *= DamageMultiplier;

	// Additional volley projectile every 2 bosses (boss 3 = 4 shots, boss 5 = 5, ...)
	VolleyCount += (SpawnNumber - 1) / 2;

	// Wider spread to accommodate more projectiles
	VolleySpread = 20.0f + 5.0f * ((SpawnNumber - 1) / 2);

	// Faster firing: reduce interval by 10% per spawn, minimum 0.5s
	FireInterval = FMath::Max(0.5f, FireInterval * FMath::Pow(0.9f, static_cast<float>(SpawnNumber - 1)));

	// Score and drops scale with difficulty
	ScoreValue = static_cast<int32>(ScoreValue * HpMultiplier);
	ScrapDropCount = static_cast<int32>(ScrapDropCount * (1.0f + 0.3f * (SpawnNumber - 1)));
}

void ABossMotherShip::UpdateMovement(float DeltaTime, AAsteroidSurvivorShip* PlayerShip)
{
	if (!PlayerShip)
	{
		// No player – drift forward slowly
		FVector NewLocation = GetActorLocation() + MoveDirection * MoveSpeed * 0.5f * DeltaTime;
		SetActorLocation(NewLocation, true);
		return;
	}

	// Actively chase the player with limited turn rate
	const FVector ToPlayer = (PlayerShip->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	const float MaxAngleThisFrame = TurnRate * DeltaTime;
	const float AngleBetween = FMath::RadiansToDegrees(
		FMath::Acos(FMath::Clamp(FVector::DotProduct(MoveDirection, ToPlayer), -1.0f, 1.0f)));

	if (AngleBetween > KINDA_SMALL_NUMBER)
	{
		const float Alpha = FMath::Clamp(MaxAngleThisFrame / AngleBetween, 0.0f, 1.0f);
		MoveDirection = FMath::Lerp(MoveDirection, ToPlayer, Alpha).GetSafeNormal();
	}

	// Rotate actor to face the player
	if (!MoveDirection.IsNearlyZero())
	{
		SetActorRotation(MoveDirection.Rotation());
	}

	// Boss always moves at full speed – relentless chase
	FVector NewLocation = GetActorLocation() + MoveDirection * MoveSpeed * DeltaTime;
	SetActorLocation(NewLocation, true);
}

void ABossMotherShip::FireVolleyAtPlayer(AAsteroidSurvivorShip* PlayerShip)
{
	if (!PlayerShip)
	{
		return;
	}

	const FVector ToPlayer = (PlayerShip->GetActorLocation() - GetActorLocation()).GetSafeNormal();

	// Spread the volley evenly across the spread angle
	const float HalfSpread = VolleySpread * 0.5f;
	const float AngleStep = (VolleyCount > 1) ? VolleySpread / (VolleyCount - 1) : 0.0f;

	for (int32 i = 0; i < VolleyCount; ++i)
	{
		float AngleOffset = (VolleyCount > 1) ? (-HalfSpread + AngleStep * i) : 0.0f;
		FVector ShotDir = ToPlayer.RotateAngleAxis(AngleOffset, FVector::UpVector);
		FRotator FireRotation = ShotDir.Rotation();

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		const FVector SpawnLoc = GetActorLocation() + ShotDir * (CollisionRadius + 30.0f);

		GetWorld()->SpawnActor<AEnemyProjectile>(
			AEnemyProjectile::StaticClass(), SpawnLoc, FireRotation, SpawnParams);
	}
}
