// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsteroidSurvivorAsteroid.h"
#include "AsteroidSurvivorProjectile.h"
#include "AsteroidSurvivorGameMode.h"
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

	// Apply a rocky brownish-orange emissive color
	if (AsteroidMesh)
	{
		UMaterialInstanceDynamic* DynMat = AsteroidMesh->CreateDynamicMaterialInstance(0);
		if (DynMat)
		{
			const FLinearColor AsteroidColor(1.5f, 0.8f, 0.3f, 1.0f);
			DynMat->SetVectorParameterValue(FName(TEXT("Color")), AsteroidColor);
			DynMat->SetVectorParameterValue(FName(TEXT("BaseColor")), AsteroidColor);
			DynMat->SetVectorParameterValue(FName(TEXT("EmissiveColor")), AsteroidColor * 0.3f);
			DynMat->SetVectorParameterValue(FName(TEXT("Emissive Color")), AsteroidColor * 0.3f);
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

void AAsteroidSurvivorAsteroid::Explode()
{
	if (bExploding)
	{
		return;
	}
	bExploding = true;

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

		Explode();
		return;
	}

	// ── Asteroid-asteroid collision ────────────────────────────────────────
	AAsteroidSurvivorAsteroid* OtherAsteroid = Cast<AAsteroidSurvivorAsteroid>(OtherActor);
	if (OtherAsteroid && GraceTimer <= 0.0f)
	{
		// Explode only if the other asteroid is the same size or larger
		if (OtherAsteroid->GetAsteroidSize() >= AsteroidSize)
		{
			Explode();
		}
		return;
	}

	// All other overlaps (ship, etc.) are handled by the other actor.
}
