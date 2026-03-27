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
	RingFireTimer = RingFireInterval;
	RapidBurstTimer = RapidBurstCooldown;
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
		// Standard volley attack
		FireTimer -= DeltaTime;
		if (FireTimer <= 0.0f)
		{
			FireVolleyAtPlayer(Ship);
			FireTimer = FireInterval;
		}

		// Ring burst attack
		if (bHasRingAttack)
		{
			RingFireTimer -= DeltaTime;
			if (RingFireTimer <= 0.0f)
			{
				FireRingAttack();
				RingFireTimer = RingFireInterval;
			}
		}

		// Rapid burst attack
		if (bHasRapidBurst)
		{
			if (RapidBurstShotsRemaining > 0)
			{
				// Currently in a burst – fire shots at rapid intervals
				RapidBurstShotTimer -= DeltaTime;
				if (RapidBurstShotTimer <= 0.0f)
				{
					FireAimedShot(Ship);
					RapidBurstShotsRemaining--;
					RapidBurstShotTimer = RapidBurstShotInterval;
				}
			}
			else
			{
				// Waiting for next burst
				RapidBurstTimer -= DeltaTime;
				if (RapidBurstTimer <= 0.0f)
				{
					RapidBurstShotsRemaining = RapidBurstShotCount;
					RapidBurstShotTimer = 0.0f; // Fire first shot immediately
					RapidBurstTimer = RapidBurstCooldown;
				}
			}
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

	const int32 Level = SpawnNumber - 1;

	// Each successive boss gets harder:
	// HP: scales by HpScalePerSpawn per level (default +50%)
	const float HpMultiplier = 1.0f + HpScalePerSpawn * Level;
	MaxHealth *= HpMultiplier;
	CurrentHealth = MaxHealth;

	// Contact damage: scales by DamageScalePerSpawn per level (default +25%)
	const float DamageMultiplier = 1.0f + DamageScalePerSpawn * Level;
	ContactDamage *= DamageMultiplier;

	// Damage resistance: increases each spawn (boss takes less damage over time)
	DamageResistance = FMath::Min(0.75f, DamageResistPerSpawn * Level);

	// Additional volley projectile every boss
	VolleyCount += Level;

	// Wider spread to accommodate more projectiles
	VolleySpread = 20.0f + 5.0f * Level;

	// Faster firing: reduce interval geometrically, with a minimum floor
	FireInterval = FMath::Max(MinFireInterval, FireInterval * FMath::Pow(FireIntervalDecayRate, static_cast<float>(Level)));

	// Unlock ring burst attack at level 2+
	if (Level >= 1)
	{
		bHasRingAttack = true;
		RingProjectileCount = 12 + Level * 4;
		RingFireInterval = FMath::Max(1.5f, 4.0f - 0.5f * Level);
	}

	// Unlock rapid burst attack at level 3+
	if (Level >= 2)
	{
		bHasRapidBurst = true;
		RapidBurstShotCount = 5 + Level * 2;
		RapidBurstShotInterval = FMath::Max(0.08f, 0.15f - 0.01f * Level);
		RapidBurstCooldown = FMath::Max(2.0f, 5.0f - 0.5f * Level);
	}

	// Score and drops scale with difficulty
	ScoreValue = static_cast<int32>(ScoreValue * HpMultiplier);
	ScrapDropCount = static_cast<int32>(ScrapDropCount * (1.0f + 0.3f * Level));
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

void ABossMotherShip::FireRingAttack()
{
	const float AngleStep = 360.0f / FMath::Max(1, RingProjectileCount);

	for (int32 i = 0; i < RingProjectileCount; ++i)
	{
		const float Angle = AngleStep * i;
		FVector ShotDir(
			FMath::Cos(FMath::DegreesToRadians(Angle)),
			FMath::Sin(FMath::DegreesToRadians(Angle)),
			0.0f);
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

void ABossMotherShip::FireAimedShot(AAsteroidSurvivorShip* PlayerShip)
{
	if (!PlayerShip)
	{
		return;
	}

	const FVector ToPlayer = (PlayerShip->GetActorLocation() - GetActorLocation()).GetSafeNormal();

	// Add slight random spread for rapid burst shots
	const float RandomSpread = FMath::FRandRange(-8.0f, 8.0f);
	FVector ShotDir = ToPlayer.RotateAngleAxis(RandomSpread, FVector::UpVector);
	FRotator FireRotation = ShotDir.Rotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector SpawnLoc = GetActorLocation() + ShotDir * (CollisionRadius + 30.0f);

	GetWorld()->SpawnActor<AEnemyProjectile>(
		AEnemyProjectile::StaticClass(), SpawnLoc, FireRotation, SpawnParams);
}
