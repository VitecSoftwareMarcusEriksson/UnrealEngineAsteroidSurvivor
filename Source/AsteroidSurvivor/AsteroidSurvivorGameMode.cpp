// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsteroidSurvivorGameMode.h"
#include "AsteroidSurvivorAsteroidSpawner.h"
#include "AsteroidSurvivorShip.h"
#include "AsteroidSurvivorPlayerController.h"
#include "AsteroidSurvivorHUD.h"
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

void AAsteroidSurvivorGameMode::OnAsteroidDestroyed(int32 Points)
{
	Score += Points;

	// Check if all asteroids are cleared – start next wave
	if (AsteroidSpawner && AsteroidSpawner->GetActiveAsteroidCount() == 0 && !bWaitingForNextWave)
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
