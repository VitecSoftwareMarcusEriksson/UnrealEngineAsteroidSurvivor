// Copyright Epic Games, Inc. All Rights Reserved.

#include "WaveManager.h"
#include "StandardEnemyShip.h"
#include "ZigzagEnemyShip.h"
#include "ShootingEnemyShip.h"
#include "BossMotherShip.h"
#include "AsteroidSurvivorShip.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

AWaveManager::AWaveManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AWaveManager::BeginPlay()
{
	Super::BeginPlay();

	// First wave spawns after a short delay to let the player get oriented
	WaveTimer = 3.0f;
	BossTimer = 0.0f;
}

void AWaveManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bSpawningPaused)
	{
		return;
	}

	// Advance the global game timer
	ElapsedTime += DeltaTime;

	// ── Wave spawning ───────────────────────────────────────────────────────
	WaveTimer -= DeltaTime;
	if (WaveTimer <= 0.0f)
	{
		CurrentWave++;

		const FWaveComposition Composition = BuildWaveForCurrentDifficulty();
		SpawnWave(Composition);

		// Waves come faster as time progresses (minimum 4 seconds apart)
		const float TimeFactor = FMath::Clamp(1.0f - (ElapsedTime / 600.0f), 0.5f, 1.0f);
		WaveTimer = WaveInterval * TimeFactor;
	}

	// ── Boss spawning (separate 60-second timer) ────────────────────────────
	BossTimer += DeltaTime;
	if (BossTimer >= BossSpawnInterval)
	{
		SpawnBoss();
		BossTimer = 0.0f;
	}

	// Clear stale boss reference if the boss was destroyed
	if (ActiveBoss && ActiveBoss->IsActorBeingDestroyed())
	{
		ActiveBoss = nullptr;
	}
	if (ActiveBoss && !IsValid(ActiveBoss))
	{
		ActiveBoss = nullptr;
	}
}

// ────────────────────────────────────────────────────────────────────────────
// Wave composition – scales with elapsed time
// ────────────────────────────────────────────────────────────────────────────

FWaveComposition AWaveManager::BuildWaveForCurrentDifficulty() const
{
	FWaveComposition Comp;

	// Phase 1 (0–30s): Only standard ships
	// Phase 2 (30–60s): Add zigzag ships
	// Phase 3 (60–120s): Add shooting ships
	// Phase 4 (120s+): Full mix, increasing counts

	const float T = ElapsedTime;

	// Standard ships: always present, count increases over time
	Comp.StandardCount = FMath::Clamp(2 + static_cast<int32>(T / 30.0f), 2, 8);

	// Zigzag ships: introduced after 30 seconds
	if (T >= 30.0f)
	{
		Comp.ZigzagCount = FMath::Clamp(1 + static_cast<int32>((T - 30.0f) / 40.0f), 1, 5);
	}
	else
	{
		Comp.ZigzagCount = 0;
	}

	// Shooting ships: introduced after 60 seconds
	if (T >= 60.0f)
	{
		Comp.ShootingCount = FMath::Clamp(1 + static_cast<int32>((T - 60.0f) / 50.0f), 1, 4);
	}
	else
	{
		Comp.ShootingCount = 0;
	}

	return Comp;
}

// ────────────────────────────────────────────────────────────────────────────
// Spawning
// ────────────────────────────────────────────────────────────────────────────

void AWaveManager::SpawnWave(const FWaveComposition& Composition)
{
	// Respect the enemy cap
	const int32 CurrentEnemies = CountActiveEnemies();
	int32 Budget = MaxEnemies - CurrentEnemies;
	if (Budget <= 0)
	{
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	const FVector PlayerLoc = PlayerPawn ? PlayerPawn->GetActorLocation() : FVector::ZeroVector;

	// Helper lambda: spawn N enemies of a given type, decrementing budget
	auto SpawnGroup = [&](EEnemyShipType Type, int32 Count)
	{
		for (int32 i = 0; i < Count && Budget > 0; ++i)
		{
			const FVector SpawnLoc = GetSpawnLocationOutsideCamera();
			const FVector DirToPlayer = (PlayerLoc - SpawnLoc).GetSafeNormal();
			SpawnEnemyOfType(Type, SpawnLoc, DirToPlayer);
			Budget--;
		}
	};

	SpawnGroup(EEnemyShipType::Standard, Composition.StandardCount);
	SpawnGroup(EEnemyShipType::Zigzag,   Composition.ZigzagCount);
	SpawnGroup(EEnemyShipType::Shooting,  Composition.ShootingCount);
}

void AWaveManager::SpawnBoss()
{
	// Only one boss at a time
	if (ActiveBoss && IsValid(ActiveBoss))
	{
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn)
	{
		return;
	}

	const FVector SpawnLoc = GetSpawnLocationOutsideCamera();
	const FVector DirToPlayer = (PlayerPawn->GetActorLocation() - SpawnLoc).GetSafeNormal();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ABossMotherShip* Boss = GetWorld()->SpawnActor<ABossMotherShip>(
		ABossMotherShip::StaticClass(), SpawnLoc, DirToPlayer.Rotation(), SpawnParams);

	if (Boss)
	{
		// InitEnemy sets direction and uses the boss's own default speed
		Boss->InitEnemy(DirToPlayer, 160.0f);
		ActiveBoss = Boss;
	}
}

void AWaveManager::SpawnEnemyOfType(EEnemyShipType Type, const FVector& SpawnLocation,
                                     const FVector& DirectionToPlayer)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AEnemyShipBase* Enemy = nullptr;
	const FRotator SpawnRot = DirectionToPlayer.Rotation();

	switch (Type)
	{
	case EEnemyShipType::Standard:
		Enemy = GetWorld()->SpawnActor<AStandardEnemyShip>(
			AStandardEnemyShip::StaticClass(), SpawnLocation, SpawnRot, SpawnParams);
		break;

	case EEnemyShipType::Zigzag:
		Enemy = GetWorld()->SpawnActor<AZigzagEnemyShip>(
			AZigzagEnemyShip::StaticClass(), SpawnLocation, SpawnRot, SpawnParams);
		break;

	case EEnemyShipType::Shooting:
		Enemy = GetWorld()->SpawnActor<AShootingEnemyShip>(
			AShootingEnemyShip::StaticClass(), SpawnLocation, SpawnRot, SpawnParams);
		break;

	case EEnemyShipType::Boss:
		// Bosses are spawned separately via SpawnBoss()
		break;
	}

	if (Enemy)
	{
		// Add slight random angle deviation (±15°) to avoid perfectly stacked spawns
		FVector AdjustedDir = DirectionToPlayer.RotateAngleAxis(
			FMath::FRandRange(-15.0f, 15.0f), FVector::UpVector);
		// Use a default speed; each enemy subclass sets its own MoveSpeed
		// internally and UpdateMovement uses it directly.
		Enemy->InitEnemy(AdjustedDir, 200.0f);
	}
}

FVector AWaveManager::GetSpawnLocationOutsideCamera() const
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	const FVector Center = PlayerPawn ? PlayerPawn->GetActorLocation() : FVector::ZeroVector;

	const float RandomAngle = FMath::FRandRange(0.0f, 360.0f);
	const FVector Offset(
		FMath::Cos(FMath::DegreesToRadians(RandomAngle)) * SpawnRadius,
		FMath::Sin(FMath::DegreesToRadians(RandomAngle)) * SpawnRadius,
		0.0f);

	return Center + Offset;
}

int32 AWaveManager::CountActiveEnemies() const
{
	int32 Count = 0;
	for (TActorIterator<AEnemyShipBase> It(GetWorld()); It; ++It)
	{
		// Don't count bosses toward the regular enemy cap
		if ((*It)->GetEnemyType() != EEnemyShipType::Boss)
		{
			Count++;
		}
	}
	return Count;
}
