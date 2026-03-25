// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsteroidSurvivorGameMode.h"
#include "AsteroidSurvivorAsteroidSpawner.h"
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

	// Find or spawn the asteroid spawner
	for (TActorIterator<AAsteroidSurvivorAsteroidSpawner> It(GetWorld()); It; ++It)
	{
		AsteroidSpawner = *It;
		break;
	}

	if (!AsteroidSpawner)
	{
		AsteroidSpawner = GetWorld()->SpawnActor<AAsteroidSurvivorAsteroidSpawner>(
			AAsteroidSurvivorAsteroidSpawner::StaticClass());
	}

	// Spawn parallax scrolling background
	Background = GetWorld()->SpawnActor<AAsteroidSurvivorBackground>(
		AAsteroidSurvivorBackground::StaticClass());

	StartWave(CurrentWave);
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

	if (bWaitingForNextWave)
	{
		NextWaveTimer -= DeltaSeconds;
		if (NextWaveTimer <= 0.0f)
		{
			bWaitingForNextWave = false;
			CurrentWave++;
			StartWave(CurrentWave);
		}
	}

	// Safety check: advance wave if all asteroids are cleared (e.g. by ship collision)
	if (!bGameOver && !bWaitingForNextWave && AsteroidSpawner &&
	    AsteroidSpawner->GetActiveAsteroidCount() == 0)
	{
		bWaitingForNextWave = true;
		NextWaveTimer = NextWaveDelay;
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

void AAsteroidSurvivorGameMode::OnAsteroidDestroyed(int32 Points)
{
	Score += Points;

	// Check if all asteroids are cleared – start next wave.
	// Use <= 1 because the dying asteroid has not yet been destroyed when
	// NotifyGameMode is called, so it is still counted by GetActiveAsteroidCount().
	if (AsteroidSpawner && AsteroidSpawner->GetActiveAsteroidCount() <= 1 && !bWaitingForNextWave)
	{
		bWaitingForNextWave = true;
		NextWaveTimer = NextWaveDelay;
	}
}

void AAsteroidSurvivorGameMode::StartWave(int32 WaveNumber)
{
	if (AsteroidSpawner)
	{
		AsteroidSpawner->StartWave(WaveNumber);
	}
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
