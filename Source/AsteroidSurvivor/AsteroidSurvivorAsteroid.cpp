// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsteroidSurvivorAsteroid.h"
#include "AsteroidSurvivorGameMode.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"

AAsteroidSurvivorAsteroid::AAsteroidSurvivorAsteroid()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	// Use overlap-based collision so projectiles, ship, and other asteroids can detect hits
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionSphere->SetGenerateOverlapEvents(true);
	SetRootComponent(CollisionSphere);

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(
		this, &AAsteroidSurvivorAsteroid::OnAsteroidOverlapBegin);

	AsteroidMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AsteroidMesh"));
	AsteroidMesh->SetupAttachment(RootComponent);
	AsteroidMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		AsteroidMesh->SetStaticMesh(SphereMesh.Object);
	}

	// Random multi-axis tumble for a more natural look
	TumbleRate = FRotator(
		FMath::FRandRange(-60.0f, 60.0f),
		FMath::FRandRange(30.0f, 120.0f),
		FMath::FRandRange(-40.0f, 40.0f)
	);
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

	// Brief immunity after spawning to prevent instant collision with siblings
	SpawnImmunityTimer = 0.3f;

	// Apply a rocky material with per-asteroid color variation
	if (AsteroidMesh)
	{
		UMaterialInstanceDynamic* DynMat = AsteroidMesh->CreateDynamicMaterialInstance(0);
		if (DynMat)
		{
			FLinearColor BaseColor(0.4f, 0.3f, 0.25f, 1.0f);
			switch (AsteroidSize)
			{
			case EAsteroidSize::Large:
				BaseColor = FLinearColor(0.35f, 0.25f, 0.2f, 1.0f);
				break;
			case EAsteroidSize::Medium:
				BaseColor = FLinearColor(0.4f, 0.3f, 0.22f, 1.0f);
				break;
			case EAsteroidSize::Small:
				BaseColor = FLinearColor(0.5f, 0.35f, 0.25f, 1.0f);
				break;
			default:
				break;
			}
			// Random tint for visual diversity
			constexpr float ColorVariation = 0.06f;
			BaseColor.R += FMath::FRandRange(-ColorVariation, ColorVariation);
			BaseColor.G += FMath::FRandRange(-ColorVariation, ColorVariation);
			BaseColor.B += FMath::FRandRange(-ColorVariation, ColorVariation);
			DynMat->SetVectorParameterValue(FName(TEXT("Color")), BaseColor);
		}
	}
}

void AAsteroidSurvivorAsteroid::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Count down spawn immunity
	if (SpawnImmunityTimer > 0.0f)
	{
		SpawnImmunityTimer -= DeltaTime;
	}

	// Drift
	AddActorWorldOffset(DriftDirection * Speed * DeltaTime, false);

	// Multi-axis tumble
	AddActorLocalRotation(TumbleRate * DeltaTime);

	// Despawn if too far from the player (spawn border is at ~2200cm from the ship)
	constexpr float DespawnDistance = 5000.0f;
	APawn* PlayerShip = UGameplayStatics::GetPlayerPawn(this, 0);
	FVector RefPos = PlayerShip ? PlayerShip->GetActorLocation() : FVector::ZeroVector;
	if (FVector::DistSquared(GetActorLocation(), RefPos) > DespawnDistance * DespawnDistance)
	{
		Destroy();
	}
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

void AAsteroidSurvivorAsteroid::OnAsteroidOverlapBegin(UPrimitiveComponent* OverlappedComp,
                                                        AActor* OtherActor,
                                                        UPrimitiveComponent* OtherComp,
                                                        int32 OtherBodyIndex,
                                                        bool bFromSweep,
                                                        const FHitResult& SweepResult)
{
	if (!OtherActor || bExploding || SpawnImmunityTimer > 0.0f)
	{
		return;
	}

	AAsteroidSurvivorAsteroid* OtherAsteroid = Cast<AAsteroidSurvivorAsteroid>(OtherActor);
	if (OtherAsteroid && !OtherAsteroid->bExploding && OtherAsteroid->SpawnImmunityTimer <= 0.0f)
	{
		bExploding = true;
		ExplodeIntoFragments();
		NotifyGameMode();
		Destroy();
	}
}

// ─────────────────────────────────────────────────────────────────────────────

void AAsteroidSurvivorAsteroid::ApplySizeParameters()
{
	float BaseRadius = 0.0f;

	switch (AsteroidSize)
	{
	case EAsteroidSize::Large:
		BaseRadius = LargeRadius;
		Speed = LargeSpeed;
		Health = LargeHealth;
		ScoreValue = LargeScore;
		break;

	case EAsteroidSize::Medium:
		BaseRadius = MediumRadius;
		Speed = MediumSpeed;
		Health = MediumHealth;
		ScoreValue = MediumScore;
		break;

	case EAsteroidSize::Small:
		BaseRadius = SmallRadius;
		Speed = SmallSpeed;
		Health = SmallHealth;
		ScoreValue = SmallScore;
		break;
	}

	CollisionSphere->SetSphereRadius(BaseRadius);

	// Non-uniform scaling for an irregular rocky shape
	float BaseScale = BaseRadius / 50.0f;
	FVector IrregularScale(
		BaseScale * FMath::FRandRange(0.8f, 1.2f),
		BaseScale * FMath::FRandRange(0.8f, 1.2f),
		BaseScale * FMath::FRandRange(0.8f, 1.2f)
	);
	AsteroidMesh->SetRelativeScale3D(IrregularScale);
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

void AAsteroidSurvivorAsteroid::ExplodeIntoFragments()
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
		return; // Small asteroids produce no fragments
	}

	// Spawn 2-4 fragments in random directions
	const int32 NumFragments = FMath::RandRange(2, 4);
	for (int32 i = 0; i < NumFragments; i++)
	{
		float RandomAngle = FMath::FRandRange(0.0f, 360.0f);
		FVector FragDir = FVector(1.0f, 0.0f, 0.0f).RotateAngleAxis(RandomAngle, FVector::UpVector);
		FragDir.Normalize();

		// Offset spawn position slightly to avoid immediate re-collision
		constexpr float FragmentSpawnOffset = 60.0f;
		FVector SpawnPos = GetActorLocation() + FragDir * FragmentSpawnOffset;
		FTransform ChildTransform(GetActorRotation(), SpawnPos);

		AAsteroidSurvivorAsteroid* Child = GetWorld()->SpawnActorDeferred<AAsteroidSurvivorAsteroid>(
			AsteroidClass, ChildTransform, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

		if (Child)
		{
			Child->AsteroidSize = ChildSize;
			Child->DriftDirection = FragDir;
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
