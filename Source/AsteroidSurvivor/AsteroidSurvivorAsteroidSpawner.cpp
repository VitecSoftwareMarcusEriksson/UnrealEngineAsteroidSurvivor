// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsteroidSurvivorAsteroidSpawner.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"
#include "EngineUtils.h"

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

void AAsteroidSurvivorAsteroidSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	CleanDestroyedRefs();

	// Continuous spawning at random intervals
	ContinuousSpawnTimer -= DeltaTime;
	if (ContinuousSpawnTimer <= 0.0f)
	{
		float SpeedMult = 1.0f + (CurrentSpeedWave - 1) * SpeedScalePerWave;

		// Pick a random size, weighted towards larger asteroids
		EAsteroidSize SpawnSize;
		int32 SizeRoll = FMath::RandRange(0, 99);
		if (SizeRoll < 50)
		{
			SpawnSize = EAsteroidSize::Large;
		}
		else if (SizeRoll < 80)
		{
			SpawnSize = EAsteroidSize::Medium;
		}
		else
		{
			SpawnSize = EAsteroidSize::Small;
		}

		SpawnAsteroid(SpawnSize, SpeedMult);
		ContinuousSpawnTimer = FMath::FRandRange(ContinuousSpawnMinInterval, ContinuousSpawnMaxInterval);
	}
}

void AAsteroidSurvivorAsteroidSpawner::StartWave(int32 WaveNumber)
{
	CleanDestroyedRefs();
	CurrentSpeedWave = WaveNumber;

	int32 NumAsteroids = BaseAsteroidsPerWave + (WaveNumber - 1) * AsteroidsPerWaveIncrement;
	float SpeedMult = 1.0f + (WaveNumber - 1) * SpeedScalePerWave;

	for (int32 i = 0; i < NumAsteroids; i++)
	{
		SpawnAsteroid(EAsteroidSize::Large, SpeedMult);
	}

	// Reset continuous spawn timer
	ContinuousSpawnTimer = FMath::FRandRange(ContinuousSpawnMinInterval, ContinuousSpawnMaxInterval);
}

int32 AAsteroidSurvivorAsteroidSpawner::GetActiveAsteroidCount() const
{
	// Count ALL asteroids in the world, including split/exploded fragments
	int32 Count = 0;
	for (TActorIterator<AAsteroidSurvivorAsteroid> It(GetWorld()); It; ++It)
	{
		if (IsValid(*It))
		{
			Count++;
		}
	}
	return Count;
}

void AAsteroidSurvivorAsteroidSpawner::SpawnAsteroid(EAsteroidSize Size, float SpeedMultiplier)
{
	if (!AsteroidClass || !GetWorld())
	{
		return;
	}

	// Spawn relative to the player ship so asteroids always appear outside the camera view
	APawn* Ship = UGameplayStatics::GetPlayerPawn(this, 0);
	FVector ShipPos = Ship ? Ship->GetActorLocation() : FVector::ZeroVector;
	ShipPos.Z = 0.0f;

	// Pick a random edge (top, bottom, left, right) relative to the ship
	int32 Edge = FMath::RandRange(0, 3);
	float HalfExt = SpawnBorderHalfExtent;

	FVector SpawnLocation;
	switch (Edge)
	{
	case 0: // Top
		SpawnLocation = ShipPos + FVector(FMath::FRandRange(-HalfExt, HalfExt),  HalfExt, 0.0f);
		break;
	case 1: // Bottom
		SpawnLocation = ShipPos + FVector(FMath::FRandRange(-HalfExt, HalfExt), -HalfExt, 0.0f);
		break;
	case 2: // Left
		SpawnLocation = ShipPos + FVector(-HalfExt, FMath::FRandRange(-HalfExt, HalfExt), 0.0f);
		break;
	default: // Right
		SpawnLocation = ShipPos + FVector( HalfExt, FMath::FRandRange(-HalfExt, HalfExt), 0.0f);
		break;
	}

	// Drift roughly toward the ship with a random spread of ±30°
	FVector DriftDir = (ShipPos - SpawnLocation).GetSafeNormal();
	float RandomAngle = FMath::FRandRange(-30.0f, 30.0f);
	DriftDir = DriftDir.RotateAngleAxis(RandomAngle, FVector::UpVector);

	FTransform SpawnTransform(FRotator::ZeroRotator, SpawnLocation);

	AAsteroidSurvivorAsteroid* Asteroid = GetWorld()->SpawnActorDeferred<AAsteroidSurvivorAsteroid>(
		AsteroidClass, SpawnTransform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

	if (Asteroid)
	{
		Asteroid->AsteroidSize = Size;
		Asteroid->DriftDirection = DriftDir;
		UGameplayStatics::FinishSpawningActor(Asteroid, SpawnTransform);
		ActiveAsteroids.Add(Asteroid);
	}
}

void AAsteroidSurvivorAsteroidSpawner::CleanDestroyedRefs()
{
	ActiveAsteroids.RemoveAll([](AAsteroidSurvivorAsteroid* A) { return !IsValid(A); });
}
