// Copyright Epic Games, Inc. All Rights Reserved.

#include "EnemyShipBase.h"
#include "AsteroidSurvivorAsteroid.h"
#include "AsteroidSurvivorProjectile.h"
#include "AsteroidSurvivorShip.h"
#include "AsteroidSurvivorGameMode.h"
#include "ScrapPickup.h"
#include "WeaponUpgradePickup.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"

AEnemyShipBase::AEnemyShipBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// Collision sphere as root – overlap detection for projectiles and player
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(CollisionRadius);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionSphere->SetGenerateOverlapEvents(true);
	SetRootComponent(CollisionSphere);

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(
		this, &AEnemyShipBase::OnEnemyOverlapBegin);

	// Visual mesh – cube shape to visually distinguish from spherical asteroids
	ShipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
	ShipMesh->SetupAttachment(CollisionSphere);
	ShipMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshAsset.Succeeded())
	{
		ShipMesh->SetStaticMesh(CubeMeshAsset.Object);
	}
	ShipMesh->SetRelativeScale3D(FVector(MeshScale));

	// Glow light for visual flair
	GlowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("GlowLight"));
	GlowLight->SetupAttachment(RootComponent);
	GlowLight->SetIntensity(8000.0f);
	GlowLight->SetLightColor(FLinearColor(1.0f, 0.2f, 0.2f));
	GlowLight->SetAttenuationRadius(300.0f);
	GlowLight->SetCastShadows(false);
}

void AEnemyShipBase::BeginPlay()
{
	Super::BeginPlay();

	// Initialise health
	CurrentHealth = MaxHealth;

	// Apply collision radius (may differ from constructor default if set by subclass)
	if (CollisionSphere)
	{
		CollisionSphere->SetSphereRadius(CollisionRadius);
	}

	// Apply mesh scale and emissive colour
	if (ShipMesh)
	{
		ShipMesh->SetRelativeScale3D(FVector(MeshScale));

		UMaterialInstanceDynamic* DynMat = ShipMesh->CreateDynamicMaterialInstance(0);
		if (DynMat)
		{
			DynMat->SetVectorParameterValue(FName(TEXT("Color")), ShipColor);
			DynMat->SetVectorParameterValue(FName(TEXT("BaseColor")), ShipColor);
			DynMat->SetVectorParameterValue(FName(TEXT("EmissiveColor")), ShipColor);
			DynMat->SetVectorParameterValue(FName(TEXT("Emissive Color")), ShipColor);
		}
	}

	// Apply light colour based on ship colour
	if (GlowLight)
	{
		GlowLight->SetLightColor(ShipColor);
	}
}

void AEnemyShipBase::InitEnemy(const FVector& InDirection, float InSpeed)
{
	MoveDirection = InDirection.GetSafeNormal();
	CurrentSpeed = InSpeed;
}

void AEnemyShipBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Find the player ship for AI behaviour
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	AAsteroidSurvivorShip* Ship = Cast<AAsteroidSurvivorShip>(PlayerPawn);

	// Delegate movement to the subclass
	UpdateMovement(DeltaTime, Ship);

	// Despawn when too far from the player ship
	if (PlayerPawn)
	{
		const float DistSq = FVector::DistSquared(GetActorLocation(), PlayerPawn->GetActorLocation());
		if (DistSq > DespawnDistance * DespawnDistance)
		{
			Destroy();
		}
	}
}

void AEnemyShipBase::UpdateMovement(float DeltaTime, AAsteroidSurvivorShip* PlayerShip)
{
	// Default: move in a straight line at the initial direction/speed.
	// Subclasses override this for more interesting behaviour.
	FVector NewLocation = GetActorLocation() + MoveDirection * CurrentSpeed * DeltaTime;
	SetActorLocation(NewLocation, true);
}

FVector AEnemyShipBase::ComputeAsteroidAvoidance() const
{
	if (!ShouldAvoidAsteroids())
	{
		return FVector::ZeroVector;
	}

	FVector AvoidanceForce = FVector::ZeroVector;

	TArray<AActor*> Asteroids;
	UGameplayStatics::GetAllActorsOfClass(this, AAsteroidSurvivorAsteroid::StaticClass(), Asteroids);

	const FVector MyLocation = GetActorLocation();

	for (AActor* Actor : Asteroids)
	{
		const FVector ToAsteroid = Actor->GetActorLocation() - MyLocation;
		const float Distance = ToAsteroid.Size();

		if (Distance < AsteroidAvoidanceRadius && Distance > KINDA_SMALL_NUMBER)
		{
			// Stronger repulsion when closer (quadratic falloff)
			const float NormDist = Distance / AsteroidAvoidanceRadius;
			const float Falloff = 1.0f - NormDist;
			const float Strength = Falloff * Falloff;
			AvoidanceForce -= ToAsteroid.GetSafeNormal() * Strength;
		}
	}

	if (!AvoidanceForce.IsNearlyZero())
	{
		AvoidanceForce = AvoidanceForce.GetClampedToMaxSize(1.0f) * AsteroidAvoidanceStrength;
	}

	return AvoidanceForce;
}

// ────────────────────────────────────────────────────────────────────────────
// Damage & destruction
// ────────────────────────────────────────────────────────────────────────────

bool AEnemyShipBase::ApplyDamage(float DamageAmount)
{
	CurrentHealth -= DamageAmount;
	if (CurrentHealth <= 0.0f)
	{
		CurrentHealth = 0.0f;
		return true; // Destroyed
	}
	return false;
}

void AEnemyShipBase::SpawnScrapDrops()
{
	for (int32 i = 0; i < ScrapDropCount; ++i)
	{
		// Scatter scrap outward from the explosion centre
		const float RandomAngle = FMath::FRandRange(0.0f, 360.0f);
		FVector ScatterDir(
			FMath::Cos(FMath::DegreesToRadians(RandomAngle)),
			FMath::Sin(FMath::DegreesToRadians(RandomAngle)),
			0.0f);

		const FVector SpawnLoc = GetActorLocation() + ScatterDir * FMath::FRandRange(10.0f, 40.0f);
		const FVector DriftVelocity = ScatterDir * FMath::FRandRange(80.0f, 200.0f);

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AScrapPickup* Pickup = GetWorld()->SpawnActor<AScrapPickup>(
			AScrapPickup::StaticClass(), SpawnLoc, FRotator::ZeroRotator, SpawnParams);

		if (Pickup)
		{
			Pickup->InitPickup(ScrapPerPickup, DriftVelocity);
		}
	}
}

void AEnemyShipBase::TrySpawnWeaponDrop()
{
	if (FMath::FRand() > WeaponDropChance)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AWeaponUpgradePickup* Pickup = GetWorld()->SpawnActor<AWeaponUpgradePickup>(
		AWeaponUpgradePickup::StaticClass(), GetActorLocation(), FRotator::ZeroRotator, SpawnParams);

	if (Pickup)
	{
		Pickup->InitPickup();
	}
}

// ────────────────────────────────────────────────────────────────────────────
// Overlap handling
// ────────────────────────────────────────────────────────────────────────────

void AEnemyShipBase::OnEnemyOverlapBegin(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this || bExploding)
	{
		return;
	}

	// ── Player projectile hit ──────────────────────────────────────────────
	if (OtherActor->IsA(AAsteroidSurvivorProjectile::StaticClass()))
	{
		AAsteroidSurvivorProjectile* Projectile = Cast<AAsteroidSurvivorProjectile>(OtherActor);
		const float ProjectileDamage = Projectile ? static_cast<float>(Projectile->GetDamage()) : 25.0f;

		const bool bDestroyed = ApplyDamage(ProjectileDamage);
		if (bDestroyed)
		{
			bExploding = true;

			// Award score
			AAsteroidSurvivorGameMode* GM = Cast<AAsteroidSurvivorGameMode>(
				UGameplayStatics::GetGameMode(this));
			if (GM)
			{
				GM->AddScore(ScoreValue);
			}

			// Drop scrap and possibly a weapon upgrade
			SpawnScrapDrops();
			TrySpawnWeaponDrop();

			Destroy();
		}
		return;
	}

	// Player contact damage is handled by the ship's overlap callback
	// (see AsteroidSurvivorShip::OnShipOverlapBegin).

	// ── Asteroid collision ─────────────────────────────────────────────────
	AAsteroidSurvivorAsteroid* Asteroid = Cast<AAsteroidSurvivorAsteroid>(OtherActor);
	if (Asteroid)
	{
		// Boss motherships are immune – the asteroid's overlap handler destroys
		// the asteroid instead (see AsteroidSurvivorAsteroid::OnAsteroidOverlapBegin).
		if (EnemyType == EEnemyShipType::Boss)
		{
			return;
		}

		// Non-boss enemy ships are destroyed on asteroid contact
		bExploding = true;

		// Award score to the player
		AAsteroidSurvivorGameMode* GM = Cast<AAsteroidSurvivorGameMode>(
			UGameplayStatics::GetGameMode(this));
		if (GM)
		{
			GM->AddScore(ScoreValue);
		}

		SpawnScrapDrops();
		TrySpawnWeaponDrop();
		Destroy();
		return;
	}
}
