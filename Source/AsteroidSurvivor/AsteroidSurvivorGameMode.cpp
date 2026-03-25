// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsteroidSurvivorGameMode.h"
#include "AsteroidSurvivorAsteroid.h"
#include "AsteroidSurvivorBackground.h"
#include "AsteroidSurvivorShip.h"
#include "AsteroidSurvivorPlayerController.h"
#include "AsteroidSurvivorHUD.h"
#include "WaveManager.h"
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

	ThoriumForNextLevel = BaseThoriumPerLevel;

	// Spawn a directional light when the level has none, so the game is
	// visible even in an empty map.
	EnsureLightingExists();

	// Spawn parallax scrolling background
	Background = GetWorld()->SpawnActor<AAsteroidSurvivorBackground>(
		AAsteroidSurvivorBackground::StaticClass());

	// Spawn the wave manager that handles enemy waves, boss spawns, and the global timer
	WaveManagerActor = GetWorld()->SpawnActor<AWaveManager>(
		AWaveManager::StaticClass());
}

void AAsteroidSurvivorGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Pause/resume the wave manager based on game state
	if (WaveManagerActor)
	{
		WaveManagerActor->SetSpawningPaused(bGameOver || bSelectingUpgrade);
	}

	// Asteroid spawning (paused during upgrade selection)
	if (!bGameOver && !bSelectingUpgrade)
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
	TriggerGameOver();
}

void AAsteroidSurvivorGameMode::AddScore(int32 Points)
{
	Score += Points;
}

// ────────────────────────────────────────────────────────────────────────────
// Thorium & leveling
// ────────────────────────────────────────────────────────────────────────────

void AAsteroidSurvivorGameMode::AddThorium(int32 Amount)
{
	if (bGameOver || bSelectingUpgrade)
	{
		return;
	}

	CurrentThorium += Amount;

	// Check for level-up
	if (CurrentThorium >= ThoriumForNextLevel)
	{
		CurrentThorium -= ThoriumForNextLevel;
		CurrentLevel++;
		ThoriumForNextLevel *= 2;
		PresentUpgradeOptions();
	}
}

void AAsteroidSurvivorGameMode::AddScrap(int32 Amount)
{
	if (bGameOver)
	{
		return;
	}

	CurrentScrap += Amount;

	// Scrap auto-upgrades: every 25 scrap collected enhances the base blaster
	// by adding extra projectiles in a fan pattern.
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	AAsteroidSurvivorShip* Ship = Cast<AAsteroidSurvivorShip>(PlayerPawn);
	if (Ship)
	{
		Ship->OnScrapCollected(CurrentScrap);
	}
}

void AAsteroidSurvivorGameMode::PresentUpgradeOptions()
{
	bSelectingUpgrade = true;
	CurrentUpgradeOptions.Empty();

	TArray<FUpgradeOption> Pool = BuildUpgradePool();

	// Shuffle and pick up to 3 unique options
	const int32 NumOptions = FMath::Min(3, Pool.Num());
	for (int32 i = 0; i < Pool.Num(); ++i)
	{
		const int32 SwapIdx = FMath::RandRange(i, Pool.Num() - 1);
		Pool.Swap(i, SwapIdx);
	}
	for (int32 i = 0; i < NumOptions; ++i)
	{
		CurrentUpgradeOptions.Add(Pool[i]);
	}
}

void AAsteroidSurvivorGameMode::SelectUpgrade(int32 Index)
{
	if (!bSelectingUpgrade || !CurrentUpgradeOptions.IsValidIndex(Index))
	{
		return;
	}

	ApplyUpgrade(CurrentUpgradeOptions[Index]);
	CurrentUpgradeOptions.Empty();
	bSelectingUpgrade = false;
}

void AAsteroidSurvivorGameMode::ApplyUpgrade(const FUpgradeOption& Upgrade)
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	AAsteroidSurvivorShip* Ship = Cast<AAsteroidSurvivorShip>(PlayerPawn);
	if (!Ship)
	{
		return;
	}

	switch (Upgrade.Type)
	{
	case EUpgradeType::MaxHealth:
		Ship->UpgradeMaxHealth(25.0f);
		break;
	case EUpgradeType::SpeedBoost:
		Ship->UpgradeSpeed(1.15f);
		break;
	case EUpgradeType::TurnRate:
		Ship->UpgradeTurnRate(1.20f);
		break;
	case EUpgradeType::PassiveHealing:
		Ship->UpgradePassiveHealing(2.0f);
		break;
	case EUpgradeType::Shield:
		Ship->UpgradeShield();
		break;
	case EUpgradeType::FireRate:
		Ship->UpgradeFireRate(1.25f);
		break;
	case EUpgradeType::ThoriumMagnet:
		Ship->UpgradeThoriumMagnet(1.50f);
		break;
	}
}

TArray<FUpgradeOption> AAsteroidSurvivorGameMode::BuildUpgradePool()
{
	TArray<FUpgradeOption> Pool;

	{
		FUpgradeOption Opt;
		Opt.Type = EUpgradeType::MaxHealth;
		Opt.Name = TEXT("Hull Reinforcement");
		Opt.Description = TEXT("+25 Max Health & full repair");
		Pool.Add(Opt);
	}
	{
		FUpgradeOption Opt;
		Opt.Type = EUpgradeType::SpeedBoost;
		Opt.Name = TEXT("Thruster Boost");
		Opt.Description = TEXT("+15% thrust & max speed");
		Pool.Add(Opt);
	}
	{
		FUpgradeOption Opt;
		Opt.Type = EUpgradeType::TurnRate;
		Opt.Name = TEXT("Gyro Stabiliser");
		Opt.Description = TEXT("+20% rotation speed");
		Pool.Add(Opt);
	}
	{
		FUpgradeOption Opt;
		Opt.Type = EUpgradeType::PassiveHealing;
		Opt.Name = TEXT("Nano Repair Bots");
		Opt.Description = TEXT("+2 HP/s passive regeneration");
		Pool.Add(Opt);
	}
	{
		FUpgradeOption Opt;
		Opt.Type = EUpgradeType::Shield;
		Opt.Name = TEXT("Energy Shield");
		Opt.Description = TEXT("Absorbs one hit, recharges over time");
		Pool.Add(Opt);
	}
	{
		FUpgradeOption Opt;
		Opt.Type = EUpgradeType::FireRate;
		Opt.Name = TEXT("Rapid Fire");
		Opt.Description = TEXT("+25% fire rate");
		Pool.Add(Opt);
	}
	{
		FUpgradeOption Opt;
		Opt.Type = EUpgradeType::ThoriumMagnet;
		Opt.Name = TEXT("Thorium Magnet");
		Opt.Description = TEXT("+50% Thorium pull radius");
		Pool.Add(Opt);
	}

	return Pool;
}

// ────────────────────────────────────────────────────────────────────────────
// Game over
// ────────────────────────────────────────────────────────────────────────────

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

	const float SpawnRadius = 2000.0f;
	const float RandomAngle = FMath::FRandRange(0.0f, 360.0f);

	const FVector Offset(
		FMath::Cos(FMath::DegreesToRadians(RandomAngle)) * SpawnRadius,
		FMath::Sin(FMath::DegreesToRadians(RandomAngle)) * SpawnRadius,
		0.0f);

	return Center + Offset;
}
