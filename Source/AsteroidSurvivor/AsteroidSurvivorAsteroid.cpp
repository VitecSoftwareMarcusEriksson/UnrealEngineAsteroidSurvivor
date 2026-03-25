// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsteroidSurvivorAsteroid.h"
#include "AsteroidSurvivorProjectile.h"
#include "AsteroidSurvivorGameMode.h"
#include "AsteroidSurvivorThoriumPickup.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"

AAsteroidSurvivorAsteroid::AAsteroidSurvivorAsteroid()
{
	PrimaryActorTick.bCanEverTick = true;

	// Collision sphere as root
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(80.0f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	// Respond to Pawn so the ship's overlap event can fire
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionSphere->SetGenerateOverlapEvents(true);
	SetRootComponent(CollisionSphere);

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(
		this, &AAsteroidSurvivorAsteroid::OnAsteroidOverlapBegin);

	// Visual mesh (sphere – no collision, purely cosmetic)
	AsteroidMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AsteroidMesh"));
	AsteroidMesh->SetupAttachment(CollisionSphere);
	AsteroidMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshAsset(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshAsset.Succeeded())
	{
		AsteroidMesh->SetStaticMesh(SphereMeshAsset.Object);
	}
}

void AAsteroidSurvivorAsteroid::BeginPlay()
{
	Super::BeginPlay();

	GraceTimer = SpawnGracePeriod;

	// Randomly decide if this asteroid contains Thorium
	bContainsThorium = (FMath::FRand() < ThoriumDropChance);

	// Apply a rocky brownish-orange emissive color.
	// Thorium-bearing asteroids get a subtle cyan tint to hint at their contents.
	if (AsteroidMesh)
	{
		UMaterialInstanceDynamic* DynMat = AsteroidMesh->CreateDynamicMaterialInstance(0);
		if (DynMat)
		{
			FLinearColor AsteroidColor(1.5f, 0.8f, 0.3f, 1.0f);
			FLinearColor EmissiveColor = AsteroidColor * 0.3f;

			if (bContainsThorium)
			{
				// Add a subtle cyan/teal tint to indicate Thorium content
				AsteroidColor = FLinearColor(1.2f, 1.0f, 0.5f, 1.0f);
				EmissiveColor = FLinearColor(0.4f, 0.8f, 1.0f, 1.0f) * 0.3f;
			}

			DynMat->SetVectorParameterValue(FName(TEXT("Color")), AsteroidColor);
			DynMat->SetVectorParameterValue(FName(TEXT("BaseColor")), AsteroidColor);
			DynMat->SetVectorParameterValue(FName(TEXT("EmissiveColor")), EmissiveColor);
			DynMat->SetVectorParameterValue(FName(TEXT("Emissive Color")), EmissiveColor);
		}
	}
}

void AAsteroidSurvivorAsteroid::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Move at constant velocity (speed is fixed for the asteroid's lifetime)
	FVector NewLocation = GetActorLocation() + Velocity * DeltaTime;
	SetActorLocation(NewLocation, true);

	// Count down the spawn grace period
	if (GraceTimer > 0.0f)
	{
		GraceTimer -= DeltaTime;
	}

	// Despawn when too far from the player ship
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (PlayerPawn)
	{
		const float DistSq = FVector::DistSquared(GetActorLocation(), PlayerPawn->GetActorLocation());
		if (DistSq > DespawnDistance * DespawnDistance)
		{
			Destroy();
		}
	}
}

// ────────────────────────────────────────────────────────────────────────────
// Initialisation helpers
// ────────────────────────────────────────────────────────────────────────────

void AAsteroidSurvivorAsteroid::InitAsteroid(EAsteroidSize InSize, const FVector& Direction, float InSpeed)
{
	AsteroidSize = InSize;
	Velocity = Direction.GetSafeNormal() * InSpeed;
	CurrentHealth = GetMaxHealthForSize();
	ApplySizeProperties();
}

float AAsteroidSurvivorAsteroid::GetCollisionRadius() const
{
	switch (AsteroidSize)
	{
	case EAsteroidSize::Large:  return 80.0f;
	case EAsteroidSize::Medium: return 50.0f;
	case EAsteroidSize::Small:  return 25.0f;
	default:                    return 50.0f;
	}
}

float AAsteroidSurvivorAsteroid::GetMeshScale() const
{
	switch (AsteroidSize)
	{
	case EAsteroidSize::Large:  return 1.6f;
	case EAsteroidSize::Medium: return 1.0f;
	case EAsteroidSize::Small:  return 0.5f;
	default:                    return 1.0f;
	}
}

float AAsteroidSurvivorAsteroid::GetDamageAmount() const
{
	switch (AsteroidSize)
	{
	case EAsteroidSize::Large:  return LargeDamage;
	case EAsteroidSize::Medium: return MediumDamage;
	case EAsteroidSize::Small:  return SmallDamage;
	default:                    return MediumDamage;
	}
}

float AAsteroidSurvivorAsteroid::GetMaxHealthForSize() const
{
	switch (AsteroidSize)
	{
	case EAsteroidSize::Large:  return LargeHealth;
	case EAsteroidSize::Medium: return MediumHealth;
	case EAsteroidSize::Small:  return SmallHealth;
	default:                    return MediumHealth;
	}
}

void AAsteroidSurvivorAsteroid::ApplySizeProperties()
{
	if (CollisionSphere)
	{
		CollisionSphere->SetSphereRadius(GetCollisionRadius());
	}
	if (AsteroidMesh)
	{
		const float Scale = GetMeshScale();
		AsteroidMesh->SetRelativeScale3D(FVector(Scale));
	}
}

// ────────────────────────────────────────────────────────────────────────────
// Explosion / splitting
// ────────────────────────────────────────────────────────────────────────────

void AAsteroidSurvivorAsteroid::Explode(bool bDropThorium)
{
	if (bExploding)
	{
		return;
	}
	bExploding = true;

	// Drop Thorium pickups if this asteroid contains Thorium and was hit by a projectile
	if (bDropThorium && bContainsThorium)
	{
		SpawnThoriumPickups();
	}

	// Determine whether this size can split into a smaller tier
	EAsteroidSize ChildSize = EAsteroidSize::Small;
	bool bCanSplit = true;

	switch (AsteroidSize)
	{
	case EAsteroidSize::Large:
		ChildSize = EAsteroidSize::Medium;
		break;
	case EAsteroidSize::Medium:
		ChildSize = EAsteroidSize::Small;
		break;
	default:
		bCanSplit = false;
		break;
	}

	if (bCanSplit)
	{
		for (int32 i = 0; i < SplitCount; ++i)
		{
			const float RandomAngle = FMath::FRandRange(0.0f, 360.0f);
			FVector ChildDirection(
				FMath::Cos(FMath::DegreesToRadians(RandomAngle)),
				FMath::Sin(FMath::DegreesToRadians(RandomAngle)),
				0.0f);

			// Offset the spawn position so children don't immediately overlap
			const FVector SpawnLocation =
				GetActorLocation() + ChildDirection * (GetCollisionRadius() + 30.0f);

			const float ChildSpeed = FMath::FRandRange(150.0f, 400.0f);

			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			AAsteroidSurvivorAsteroid* Child = GetWorld()->SpawnActor<AAsteroidSurvivorAsteroid>(
				GetClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParams);

			if (Child)
			{
				Child->InitAsteroid(ChildSize, ChildDirection, ChildSpeed);
			}
		}
	}

	Destroy();
}

void AAsteroidSurvivorAsteroid::SpawnThoriumPickups()
{
	// Determine pickup count and energy per pickup based on asteroid size
	int32 PickupCount = 0;
	int32 ThoriumPerPickup = 0;

	switch (AsteroidSize)
	{
	case EAsteroidSize::Large:
		PickupCount = FMath::RandRange(3, 5);
		ThoriumPerPickup = LargeThoriumPerPickup;
		break;
	case EAsteroidSize::Medium:
		PickupCount = FMath::RandRange(2, 3);
		ThoriumPerPickup = MediumThoriumPerPickup;
		break;
	case EAsteroidSize::Small:
		PickupCount = FMath::RandRange(1, 2);
		ThoriumPerPickup = SmallThoriumPerPickup;
		break;
	}

	for (int32 i = 0; i < PickupCount; ++i)
	{
		// Scatter pickups outward from the explosion center
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

		AAsteroidSurvivorThoriumPickup* Pickup = GetWorld()->SpawnActor<AAsteroidSurvivorThoriumPickup>(
			AAsteroidSurvivorThoriumPickup::StaticClass(), SpawnLoc, FRotator::ZeroRotator, SpawnParams);

		if (Pickup)
		{
			Pickup->InitPickup(ThoriumPerPickup, DriftVelocity);
		}
	}
}

// ────────────────────────────────────────────────────────────────────────────
// Overlap handling
// ────────────────────────────────────────────────────────────────────────────

void AAsteroidSurvivorAsteroid::OnAsteroidOverlapBegin(
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

	// ── Projectile hit ─────────────────────────────────────────────────────
	if (OtherActor->IsA(AAsteroidSurvivorProjectile::StaticClass()))
	{
		// Apply projectile damage to asteroid health
		AAsteroidSurvivorProjectile* Projectile = Cast<AAsteroidSurvivorProjectile>(OtherActor);
		const int32 ProjectileDamage = Projectile ? Projectile->GetDamage() : 25;
		CurrentHealth -= static_cast<float>(ProjectileDamage);

		if (CurrentHealth <= 0.0f)
		{
			// Award score via the game mode
			AAsteroidSurvivorGameMode* GM = Cast<AAsteroidSurvivorGameMode>(
				UGameplayStatics::GetGameMode(this));
			if (GM)
			{
				int32 Points = 0;
				switch (AsteroidSize)
				{
				case EAsteroidSize::Large:  Points = 20;  break;
				case EAsteroidSize::Medium: Points = 50;  break;
				case EAsteroidSize::Small:  Points = 100; break;
				}
				GM->AddScore(Points);
			}

			Explode(/*bDropThorium=*/ true);
		}
		return;
	}

	// ── Asteroid-asteroid collision ────────────────────────────────────────
	AAsteroidSurvivorAsteroid* OtherAsteroid = Cast<AAsteroidSurvivorAsteroid>(OtherActor);
	if (OtherAsteroid && GraceTimer <= 0.0f)
	{
		// Explode only if the other asteroid is the same size or larger
		if (OtherAsteroid->GetAsteroidSize() >= AsteroidSize)
		{
			Explode(/*bDropThorium=*/ false);
		}
		return;
	}

	// All other overlaps (ship, etc.) are handled by the other actor.
}
