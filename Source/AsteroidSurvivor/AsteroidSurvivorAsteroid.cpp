// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsteroidSurvivorAsteroid.h"
#include "AsteroidSurvivorGameMode.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"

AAsteroidSurvivorAsteroid::AAsteroidSurvivorAsteroid()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CollisionSphere->SetGenerateOverlapEvents(true);
	SetRootComponent(CollisionSphere);

	AsteroidMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AsteroidMesh"));
	AsteroidMesh->SetupAttachment(RootComponent);
	AsteroidMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		AsteroidMesh->SetStaticMesh(SphereMesh.Object);
	}

	// Random slow tumble
	RotationSpeed = FMath::FRandRange(30.0f, 120.0f);
}

void AAsteroidSurvivorAsteroid::BeginPlay()
{
	Super::BeginPlay();
	ApplySizeParameters();

	// Default class to self if not assigned
	if (!AsteroidClass)
	{
		AsteroidClass = GetClass();
	}
}

void AAsteroidSurvivorAsteroid::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Drift
	AddActorWorldOffset(DriftDirection * Speed * DeltaTime, true);

	// Tumble
	AddActorLocalRotation(FRotator(0.0f, RotationSpeed * DeltaTime, 0.0f));
}

void AAsteroidSurvivorAsteroid::TakeDamage_Asteroid(int32 DamageAmount)
{
	Health -= DamageAmount;
	if (Health <= 0)
	{
		Split();
		NotifyGameMode();
		Destroy();
	}
}

// ─────────────────────────────────────────────────────────────────────────────

void AAsteroidSurvivorAsteroid::ApplySizeParameters()
{
	switch (AsteroidSize)
	{
	case EAsteroidSize::Large:
		CollisionSphere->SetSphereRadius(LargeRadius);
		AsteroidMesh->SetRelativeScale3D(FVector(LargeRadius / 50.0f));
		Speed = LargeSpeed;
		Health = LargeHealth;
		ScoreValue = LargeScore;
		break;

	case EAsteroidSize::Medium:
		CollisionSphere->SetSphereRadius(MediumRadius);
		AsteroidMesh->SetRelativeScale3D(FVector(MediumRadius / 50.0f));
		Speed = MediumSpeed;
		Health = MediumHealth;
		ScoreValue = MediumScore;
		break;

	case EAsteroidSize::Small:
		CollisionSphere->SetSphereRadius(SmallRadius);
		AsteroidMesh->SetRelativeScale3D(FVector(SmallRadius / 50.0f));
		Speed = SmallSpeed;
		Health = SmallHealth;
		ScoreValue = SmallScore;
		break;
	}
}

void AAsteroidSurvivorAsteroid::Split()
{
	EAsteroidSize ChildSize;

	switch (AsteroidSize)
	{
	case EAsteroidSize::Large:
		ChildSize = EAsteroidSize::Medium;
		break;
	case EAsteroidSize::Medium:
		ChildSize = EAsteroidSize::Small;
		break;
	default:
		return; // Small asteroids do not split
	}

	const int32 NumChildren = 2;
	for (int32 i = 0; i < NumChildren; i++)
	{
		// Spread the two children at ±45° from the original drift
		float AngleOffset = (i == 0) ? 45.0f : -45.0f;
		FVector ChildDir = DriftDirection.RotateAngleAxis(AngleOffset, FVector::UpVector);
		ChildDir.Normalize();

		FTransform ChildTransform(GetActorRotation(), GetActorLocation());

		AAsteroidSurvivorAsteroid* Child = GetWorld()->SpawnActorDeferred<AAsteroidSurvivorAsteroid>(
			AsteroidClass, ChildTransform, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

		if (Child)
		{
			Child->AsteroidSize = ChildSize;
			Child->DriftDirection = ChildDir;
			UGameplayStatics::FinishSpawningActor(Child, ChildTransform);
		}
	}
}

void AAsteroidSurvivorAsteroid::NotifyGameMode() const
{
	if (AAsteroidSurvivorGameMode* GM = Cast<AAsteroidSurvivorGameMode>(
	    UGameplayStatics::GetGameMode(this)))
	{
		GM->OnAsteroidDestroyed(ScoreValue);
	}
}
