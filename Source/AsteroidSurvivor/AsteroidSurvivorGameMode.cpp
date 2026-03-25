// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsteroidSurvivorGameMode.h"
#include "AsteroidSurvivorAsteroid.h"
#include "AsteroidSurvivorBackground.h"
#include "AsteroidSurvivorShip.h"
#include "AsteroidSurvivorPlayerController.h"
#include "AsteroidSurvivorHUD.h"
#include "Engine/DirectionalLight.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

AAsteroidSurvivorGameMode::AAsteroidSurvivorGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = AAsteroidSurvivorShip::StaticClass();
	PlayerControllerClass = AAsteroidSurvivorPlayerController::StaticClass();
	HUDClass = AAsteroidSurvivorHUD::StaticClass();
}

void AAsteroidSurvivorGameMode::BeginPlay()
{
	Super::BeginPlay();

	Lives = InitialLives;

	// Spawn a directional light when the level has none, so the game is
	// visible even in an empty map.
	EnsureLightingExists();

	// Spawn parallax scrolling background
	Background = GetWorld()->SpawnActor<AAsteroidSurvivorBackground>(
		AAsteroidSurvivorBackground::StaticClass());
}

void AAsteroidSurvivorGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bWaitingForRespawn)
	{
		RespawnTimer -= DeltaSeconds;
		if (RespawnTimer <= 0.0f)
		{
			bWaitingForRespawn = false;
			RespawnPlayer();
		}
	}

	// Asteroid spawning
	if (!bGameOver)
	{
		AsteroidSpawnTimer -= DeltaSeconds;
		if (AsteroidSpawnTimer <= 0.0f)
		{
			SpawnAsteroid();
			AsteroidSpawnTimer = AsteroidSpawnInterval;
		}
	}
}

void AAsteroidSurvivorGameMode::OnPlayerShipDestroyed()
{
	Lives--;
	if (Lives <= 0)
	{
		TriggerGameOver();
	}
	else
	{
		bWaitingForRespawn = true;
		RespawnTimer = RespawnDelay;
	}
}

void AAsteroidSurvivorGameMode::OnPlayerShipHit()
{
	Lives--;
	if (Lives <= 0)
	{
		TriggerGameOver();
	}
}

void AAsteroidSurvivorGameMode::AddScore(int32 Points)
{
	Score += Points;
}

void AAsteroidSurvivorGameMode::TriggerGameOver()
{
	bGameOver = true;

	// Disable player input
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		PC->DisableInput(PC);
	}
}

void AAsteroidSurvivorGameMode::RespawnPlayer()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	// Destroy old pawn if present
	APawn* OldPawn = PC->GetPawn();
	if (OldPawn)
	{
		OldPawn->Destroy();
	}

	// Find a start spot and respawn
	AActor* StartSpot = FindPlayerStart(PC);
	FVector SpawnLocation = StartSpot ? StartSpot->GetActorLocation() : FVector::ZeroVector;
	FRotator SpawnRotation = StartSpot ? StartSpot->GetActorRotation() : FRotator::ZeroRotator;

	AAsteroidSurvivorShip* NewShip = GetWorld()->SpawnActor<AAsteroidSurvivorShip>(
		AAsteroidSurvivorShip::StaticClass(), SpawnLocation, SpawnRotation);

	if (NewShip)
	{
		PC->Possess(NewShip);
	}
}

void AAsteroidSurvivorGameMode::EnsureLightingExists()
{
	// If the level already contains a directional light, do nothing.
	for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
	{
		return;
	}

	// Spawn a default sun-like directional light so geometry is visible.
	FVector Location(0.0f, 0.0f, 1000.0f);
	FRotator Rotation(-50.0f, -45.0f, 0.0f);
	GetWorld()->SpawnActor<ADirectionalLight>(
		ADirectionalLight::StaticClass(), Location, Rotation);
}

// ────────────────────────────────────────────────────────────────────────────
// Asteroid spawning
// ────────────────────────────────────────────────────────────────────────────

void AAsteroidSurvivorGameMode::SpawnAsteroid()
{
	// Enforce the maximum asteroid count
	int32 AsteroidCount = 0;
	for (TActorIterator<AAsteroidSurvivorAsteroid> It(GetWorld()); It; ++It)
	{
		++AsteroidCount;
	}
	if (AsteroidCount >= MaxAsteroids)
	{
		return;
	}

	const FVector SpawnLocation = GetSpawnLocationOutsideCamera();

	// Pick a random size with a bias toward larger asteroids
	EAsteroidSize Size;
	const float SizeRoll = FMath::FRand();
	if (SizeRoll < 0.5f)
	{
		Size = EAsteroidSize::Large;
	}
	else if (SizeRoll < 0.8f)
	{
		Size = EAsteroidSize::Medium;
	}
	else
	{
		Size = EAsteroidSize::Small;
	}

	// Aim roughly toward the player with some random deviation (±30°)
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	FVector TargetLocation = PlayerPawn ? PlayerPawn->GetActorLocation() : FVector::ZeroVector;
	FVector Direction = (TargetLocation - SpawnLocation).GetSafeNormal();
	const float AngleDeviation = FMath::FRandRange(-30.0f, 30.0f);
	Direction = Direction.RotateAngleAxis(AngleDeviation, FVector::UpVector);

	const float Speed = FMath::FRandRange(MinAsteroidSpeed, MaxAsteroidSpeed);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AAsteroidSurvivorAsteroid* Asteroid = GetWorld()->SpawnActor<AAsteroidSurvivorAsteroid>(
		AAsteroidSurvivorAsteroid::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParams);

	if (Asteroid)
	{
		Asteroid->InitAsteroid(Size, Direction, Speed);
	}
}

FVector AAsteroidSurvivorGameMode::GetSpawnLocationOutsideCamera() const
{
	// The camera is orthographic with OrthoWidth 2048, so the visible
	// half-extent along X is ~1024 cm.  Spawn on a circle well beyond that.
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	const FVector Center = PlayerPawn ? PlayerPawn->GetActorLocation() : FVector::ZeroVector;

	const float SpawnRadius = 1500.0f;
	const float RandomAngle = FMath::FRandRange(0.0f, 360.0f);

	const FVector Offset(
		FMath::Cos(FMath::DegreesToRadians(RandomAngle)) * SpawnRadius,
		FMath::Sin(FMath::DegreesToRadians(RandomAngle)) * SpawnRadius,
		0.0f);

	return Center + Offset;
}
