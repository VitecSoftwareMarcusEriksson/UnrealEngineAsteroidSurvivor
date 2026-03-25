// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsteroidSurvivorAsteroidSpawner.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"
#include "EngineUtils.h"

// ---------------------------------------------------------------------------

AAsteroidSurvivorAsteroidSpawner::AAsteroidSurvivorAsteroidSpawner()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AAsteroidSurvivorAsteroidSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (!AsteroidClass)
	{
		AsteroidClass = AAsteroidSurvivorAsteroid::StaticClass();
	}
}

// ---------------------------------------------------------------------------
// Tick – continuous spawning between waves
// ---------------------------------------------------------------------------

void AAsteroidSurvivorAsteroidSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ContinuousSpawnTimer -= DeltaTime;
	if (ContinuousSpawnTimer <= 0.0f)
	{
		const float SpeedMult = 1.0f + (CurrentSpeedWave - 1) * SpeedScalePerWave;

		// Weighted random size: 50 % Large, 30 % Medium, 20 % Small.
		const int32 Roll = FMath::RandRange(0, 99);
		EAsteroidSize Size;
		if (Roll < 50)
		{
			Size = EAsteroidSize::Large;
		}
		else if (Roll < 80)
		{
			Size = EAsteroidSize::Medium;
		}
		else
		{
			Size = EAsteroidSize::Small;
		}

		SpawnAsteroid(Size, SpeedMult);
		ContinuousSpawnTimer = FMath::FRandRange(
			ContinuousSpawnMinInterval, ContinuousSpawnMaxInterval);
	}
}

// ---------------------------------------------------------------------------
// StartWave
// ---------------------------------------------------------------------------

void AAsteroidSurvivorAsteroidSpawner::StartWave(int32 WaveNumber)
{
	CurrentSpeedWave = WaveNumber;

	const int32 Count     = BaseAsteroidsPerWave + (WaveNumber - 1) * AsteroidsPerWaveIncrement;
	const float SpeedMult = 1.0f + (WaveNumber - 1) * SpeedScalePerWave;

	for (int32 i = 0; i < Count; ++i)
	{
		SpawnAsteroid(EAsteroidSize::Large, SpeedMult);
	}

	ContinuousSpawnTimer = FMath::FRandRange(
		ContinuousSpawnMinInterval, ContinuousSpawnMaxInterval);
}

// ---------------------------------------------------------------------------
// GetActiveAsteroidCount
// ---------------------------------------------------------------------------

int32 AAsteroidSurvivorAsteroidSpawner::GetActiveAsteroidCount() const
{
	int32 Count = 0;
	for (TActorIterator<AAsteroidSurvivorAsteroid> It(GetWorld()); It; ++It)
	{
		if (IsValid(*It))
		{
			++Count;
		}
	}
	return Count;
}

// ---------------------------------------------------------------------------
// SpawnAsteroid – places one asteroid on a random screen edge
// ---------------------------------------------------------------------------

void AAsteroidSurvivorAsteroidSpawner::SpawnAsteroid(EAsteroidSize Size, float SpeedMult)
{
	if (!AsteroidClass || !GetWorld())
	{
		return;
	}

	// Reference position: player ship (or world origin when none exists).
	APawn* Ship = UGameplayStatics::GetPlayerPawn(this, 0);
	FVector ShipPos = Ship ? Ship->GetActorLocation() : FVector::ZeroVector;
	ShipPos.Z = 0.0f;

	// Pick a random edge of the spawn rectangle around the player.
	const float H = SpawnBorderHalfExtent;
	const int32 Edge = FMath::RandRange(0, 3);

	FVector SpawnPos;
	switch (Edge)
	{
	case 0: // Top
		SpawnPos = ShipPos + FVector(FMath::FRandRange(-H, H),  H, 0.0f);
		break;
	case 1: // Bottom
		SpawnPos = ShipPos + FVector(FMath::FRandRange(-H, H), -H, 0.0f);
		break;
	case 2: // Left
		SpawnPos = ShipPos + FVector(-H, FMath::FRandRange(-H, H), 0.0f);
		break;
	default: // Right
		SpawnPos = ShipPos + FVector( H, FMath::FRandRange(-H, H), 0.0f);
		break;
	}

	// Drift roughly toward the player with ±30° random spread.
	FVector DriftDir = (ShipPos - SpawnPos).GetSafeNormal();
	DriftDir = DriftDir.RotateAngleAxis(FMath::FRandRange(-30.0f, 30.0f), FVector::UpVector);
	DriftDir.Z = 0.0f;
	DriftDir = DriftDir.GetSafeNormal();

	FTransform SpawnTransform(FRotator::ZeroRotator, SpawnPos);

	AAsteroidSurvivorAsteroid* Asteroid =
		GetWorld()->SpawnActorDeferred<AAsteroidSurvivorAsteroid>(
			AsteroidClass, SpawnTransform, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (Asteroid)
	{
		Asteroid->AsteroidSize    = Size;
		Asteroid->SpeedMultiplier = SpeedMult;
		Asteroid->DriftDirection  = DriftDir;
		UGameplayStatics::FinishSpawningActor(Asteroid, SpawnTransform);
	}
}
