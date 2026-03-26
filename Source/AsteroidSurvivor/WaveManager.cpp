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

		// Waves come faster as time progresses – no lower floor, keeps accelerating
		// At 600s the interval is halved, at 1200s it's a third, etc.
		const float TimeFactor = 1.0f / (1.0f + ElapsedTime / 600.0f);
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
	// Phase 4 (120s+): Full mix, counts keep growing with no upper limit

	const float T = ElapsedTime;

	// Standard ships: always present, count increases over time with no cap
	Comp.StandardCount = FMath::Max(2, 2 + static_cast<int32>(T / 30.0f));

	// Zigzag ships: introduced after 30 seconds, always in groups of at least MinZigzagGroupSize
	if (T >= 30.0f)
	{
		const int32 BaseCount = 1 + static_cast<int32>((T - 30.0f) / 40.0f);
		Comp.ZigzagCount = FMath::Max(MinZigzagGroupSize, BaseCount);
	}
	else
	{
		Comp.ZigzagCount = 0;
	}

	// Shooting ships: introduced after 60 seconds, count grows with no cap
	if (T >= 60.0f)
	{
		Comp.ShootingCount = FMath::Max(1, 1 + static_cast<int32>((T - 60.0f) / 50.0f));
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

int32 AWaveManager::GetCurrentMaxEnemies() const
{
	// Enemy cap grows over time: +5 enemies per 60 seconds, no upper limit
	return BaseMaxEnemies + static_cast<int32>(ElapsedTime / 60.0f) * 5;
}

void AWaveManager::SpawnWave(const FWaveComposition& Composition)
{
	// Respect the enemy cap (which grows over time)
	const int32 CurrentEnemies = CountActiveEnemies();
	int32 Budget = GetCurrentMaxEnemies() - CurrentEnemies;
	if (Budget <= 0)
	{
		return;
	}

	// Standard and shooting enemies spawn in clustered waves from one direction
	SpawnClusteredGroup(EEnemyShipType::Standard, Composition.StandardCount, Budget);
	SpawnClusteredGroup(EEnemyShipType::Shooting, Composition.ShootingCount, Budget);

	// Zigzag enemies spawn in a line formation
	SpawnZigzagFormation(Composition.ZigzagCount, Budget);
}

void AWaveManager::SpawnZigzagFormation(int32 Count, int32& Budget)
{
	if (Count <= 0 || Budget <= 0)
	{
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	const FVector PlayerLoc = PlayerPawn ? PlayerPawn->GetActorLocation() : FVector::ZeroVector;

	// Pick a random angle for the formation center
	const float CenterAngle = FMath::FRandRange(0.0f, 360.0f);
	const FVector CenterOffset(
		FMath::Cos(FMath::DegreesToRadians(CenterAngle)) * SpawnRadius,
		FMath::Sin(FMath::DegreesToRadians(CenterAngle)) * SpawnRadius,
		0.0f);
	const FVector CenterSpawn = PlayerLoc + CenterOffset;
	const FVector DirToPlayer = (PlayerLoc - CenterSpawn).GetSafeNormal();

	// Line direction perpendicular to the direction toward the player
	const FVector LineDir = FVector(-DirToPlayer.Y, DirToPlayer.X, 0.0f);

	// Center the line formation
	const float HalfLineLength = (Count - 1) * ZigzagLineSpacing * 0.5f;

	for (int32 i = 0; i < Count && Budget > 0; ++i)
	{
		const float Offset = -HalfLineLength + i * ZigzagLineSpacing;
		const FVector SpawnLoc = CenterSpawn + LineDir * Offset;
		SpawnEnemyOfType(EEnemyShipType::Zigzag, SpawnLoc, DirToPlayer);
		Budget--;
	}
}

void AWaveManager::SpawnClusteredGroup(EEnemyShipType Type, int32 Count, int32& Budget)
{
	if (Count <= 0 || Budget <= 0)
	{
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	const FVector PlayerLoc = PlayerPawn ? PlayerPawn->GetActorLocation() : FVector::ZeroVector;

	// Pick a single direction for the whole wave cluster
	const float BaseAngle = FMath::FRandRange(0.0f, 360.0f);

	for (int32 i = 0; i < Count && Budget > 0; ++i)
	{
		// Cluster enemies within ±20° of the base direction
		const float AngleDeviation = FMath::FRandRange(-20.0f, 20.0f);
		const float Angle = BaseAngle + AngleDeviation;
		// Slight radius variation so they don't stack exactly
		const float RadiusVariation = FMath::FRandRange(-200.0f, 200.0f);
		const FVector Offset(
			FMath::Cos(FMath::DegreesToRadians(Angle)) * (SpawnRadius + RadiusVariation),
			FMath::Sin(FMath::DegreesToRadians(Angle)) * (SpawnRadius + RadiusVariation),
			0.0f);
		const FVector SpawnLoc = PlayerLoc + Offset;
		const FVector DirToPlayer = (PlayerLoc - SpawnLoc).GetSafeNormal();
		SpawnEnemyOfType(Type, SpawnLoc, DirToPlayer);
		Budget--;
	}
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
		BossSpawnCount++;
		// Scale boss difficulty based on how many bosses have been spawned
		Boss->ApplyDifficultyScaling(BossSpawnCount);
		// InitEnemy sets direction and uses the boss's own default speed
		Boss->InitEnemy(DirToPlayer, Boss->GetMoveSpeed());
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
