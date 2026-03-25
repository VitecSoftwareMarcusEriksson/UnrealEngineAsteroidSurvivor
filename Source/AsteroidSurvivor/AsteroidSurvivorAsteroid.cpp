// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsteroidSurvivorAsteroid.h"
#include "AsteroidSurvivorGameMode.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

AAsteroidSurvivorAsteroid::AAsteroidSurvivorAsteroid()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// --- Collision sphere (root) ---
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionSphere->SetGenerateOverlapEvents(true);
	CollisionSphere->SetSimulatePhysics(false);
	CollisionSphere->SetMobility(EComponentMobility::Movable);
	SetRootComponent(CollisionSphere);

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(
		this, &AAsteroidSurvivorAsteroid::OnAsteroidOverlapBegin);

	// --- Visual mesh ---
	AsteroidMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AsteroidMesh"));
	AsteroidMesh->SetupAttachment(RootComponent);
	AsteroidMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AsteroidMesh->SetSimulatePhysics(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		AsteroidMesh->SetStaticMesh(SphereMesh.Object);
	}

	// Random yaw spin for visual interest. Z-axis only rotation keeps the
	// sphere round when viewed from the top-down camera.
	YawSpinRate = FMath::FRandRange(-120.0f, 120.0f);
}

// ---------------------------------------------------------------------------
// BeginPlay
// ---------------------------------------------------------------------------

void AAsteroidSurvivorAsteroid::BeginPlay()
{
	Super::BeginPlay();

	ApplySizeParameters();

	// Fall back to own class when spawning children.
	if (!AsteroidClass)
	{
		AsteroidClass = GetClass();
	}

	// Ensure a valid drift direction.
	if (DriftDirection.IsNearlyZero())
	{
		DriftDirection = FVector(1.0f, 0.0f, 0.0f);
	}
	DriftDirection = DriftDirection.GetSafeNormal();

	// Keep everything on the XY plane.
	DriftDirection.Z = 0.0f;
	DriftDirection = DriftDirection.GetSafeNormal();

	// Safety: speed must be positive.
	if (Speed <= 0.0f)
	{
		Speed = 150.0f;
	}

	// Brief immunity so siblings spawned at the same spot don't immediately
	// overlap-collide with each other.
	SpawnImmunityTimer = 0.3f;

	// --- Emissive material per size ---
	if (AsteroidMesh)
	{
		UMaterialInstanceDynamic* DynMat = AsteroidMesh->CreateDynamicMaterialInstance(0);
		if (DynMat)
		{
			FLinearColor Color;
			switch (AsteroidSize)
			{
			case EAsteroidSize::Large:
				Color = FLinearColor(2.5f, 1.6f, 1.0f, 1.0f);
				break;
			case EAsteroidSize::Medium:
				Color = FLinearColor(2.0f, 1.4f, 0.8f, 1.0f);
				break;
			case EAsteroidSize::Small:
				Color = FLinearColor(3.0f, 2.0f, 1.2f, 1.0f);
				break;
			}

			// Slight random tint for visual variety.
			constexpr float V = 0.3f;
			Color.R += FMath::FRandRange(-V, V);
			Color.G += FMath::FRandRange(-V, V);
			Color.B += FMath::FRandRange(-V, V);

			DynMat->SetVectorParameterValue(FName(TEXT("Color")), Color);
			DynMat->SetVectorParameterValue(FName(TEXT("BaseColor")), Color);
			DynMat->SetVectorParameterValue(FName(TEXT("EmissiveColor")), Color);
			DynMat->SetVectorParameterValue(FName(TEXT("Emissive Color")), Color);
		}
	}
}

// ---------------------------------------------------------------------------
// Tick – movement & visual spin
// ---------------------------------------------------------------------------

void AAsteroidSurvivorAsteroid::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Count down spawn immunity.
	if (SpawnImmunityTimer > 0.0f)
	{
		SpawnImmunityTimer -= DeltaTime;
	}

	// --- Drift movement (world-space, XY plane only) ---
	const FVector Delta = DriftDirection * Speed * DeltaTime;
	SetActorLocation(GetActorLocation() + Delta, false);

	// --- Yaw-only visual spin (keeps the sphere round from top-down view) ---
	FRotator Spin(0.0f, YawSpinRate * DeltaTime, 0.0f);
	AsteroidMesh->AddLocalRotation(Spin);

	// --- Despawn when too far from the player ---
	constexpr float DespawnDistance = 5000.0f;
	APawn* PlayerShip = UGameplayStatics::GetPlayerPawn(this, 0);
	const FVector RefPos = PlayerShip ? PlayerShip->GetActorLocation() : FVector::ZeroVector;
	if (FVector::DistSquared(GetActorLocation(), RefPos) > DespawnDistance * DespawnDistance)
	{
		Destroy();
	}
}

// ---------------------------------------------------------------------------
// Damage & destruction
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Overlap callback (asteroid-asteroid collisions)
// ---------------------------------------------------------------------------

void AAsteroidSurvivorAsteroid::OnAsteroidOverlapBegin(
	UPrimitiveComponent* OverlappedComp,
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

	AAsteroidSurvivorAsteroid* Other = Cast<AAsteroidSurvivorAsteroid>(OtherActor);
	if (Other && !Other->bExploding && Other->SpawnImmunityTimer <= 0.0f)
	{
		bExploding = true;
		ExplodeIntoFragments();
		NotifyGameMode();
		Destroy();
	}
}

// ---------------------------------------------------------------------------
// Apply per-size parameters
// ---------------------------------------------------------------------------

void AAsteroidSurvivorAsteroid::ApplySizeParameters()
{
	float Radius = 0.0f;

	switch (AsteroidSize)
	{
	case EAsteroidSize::Large:
		Radius     = LargeRadius;
		Speed      = LargeSpeed;
		Health     = LargeHealth;
		ScoreValue = LargeScore;
		break;
	case EAsteroidSize::Medium:
		Radius     = MediumRadius;
		Speed      = MediumSpeed;
		Health     = MediumHealth;
		ScoreValue = MediumScore;
		break;
	case EAsteroidSize::Small:
		Radius     = SmallRadius;
		Speed      = SmallSpeed;
		Health     = SmallHealth;
		ScoreValue = SmallScore;
		break;
	}

	CollisionSphere->SetSphereRadius(Radius);

	// Apply wave-based speed multiplier.
	Speed *= SpeedMultiplier;

	// Uniform scale so the sphere remains perfectly round.
	// Engine BasicShapes/Sphere has a 50 cm default radius.
	const float UniformScale = Radius / 50.0f;
	AsteroidMesh->SetRelativeScale3D(FVector(UniformScale, UniformScale, UniformScale));
}

// ---------------------------------------------------------------------------
// Split – called when a projectile destroys the asteroid
// ---------------------------------------------------------------------------

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
		return; // Small asteroids do not split.
	}

	for (int32 i = 0; i < 2; ++i)
	{
		// Two children at ±45° from the original drift direction.
		const float Angle = (i == 0) ? 45.0f : -45.0f;
		FVector ChildDir = DriftDirection.RotateAngleAxis(Angle, FVector::UpVector);
		ChildDir.Z = 0.0f;
		ChildDir = ChildDir.GetSafeNormal();

		FTransform SpawnTransform(FRotator::ZeroRotator, GetActorLocation());

		AAsteroidSurvivorAsteroid* Child =
			GetWorld()->SpawnActorDeferred<AAsteroidSurvivorAsteroid>(
				AsteroidClass, SpawnTransform, nullptr, nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		if (Child)
		{
			Child->AsteroidSize    = ChildSize;
			Child->SpeedMultiplier = SpeedMultiplier;
			Child->DriftDirection  = ChildDir;
			UGameplayStatics::FinishSpawningActor(Child, SpawnTransform);
		}
	}
}

// ---------------------------------------------------------------------------
// ExplodeIntoFragments – called on asteroid-asteroid collision
// ---------------------------------------------------------------------------

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
		return; // Small asteroids produce no fragments.
	}

	const int32 NumFragments = FMath::RandRange(2, 4);
	for (int32 i = 0; i < NumFragments; ++i)
	{
		const float RandomAngle = FMath::FRandRange(0.0f, 360.0f);
		FVector FragDir = FVector(1.0f, 0.0f, 0.0f).RotateAngleAxis(RandomAngle, FVector::UpVector);
		FragDir.Z = 0.0f;
		FragDir = FragDir.GetSafeNormal();

		// Offset slightly so fragments don't immediately re-collide.
		constexpr float SpawnOffset = 60.0f;
		const FVector SpawnPos = GetActorLocation() + FragDir * SpawnOffset;
		FTransform SpawnTransform(FRotator::ZeroRotator, SpawnPos);

		AAsteroidSurvivorAsteroid* Child =
			GetWorld()->SpawnActorDeferred<AAsteroidSurvivorAsteroid>(
				AsteroidClass, SpawnTransform, nullptr, nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		if (Child)
		{
			Child->AsteroidSize    = ChildSize;
			Child->SpeedMultiplier = SpeedMultiplier;
			Child->DriftDirection  = FragDir;
			UGameplayStatics::FinishSpawningActor(Child, SpawnTransform);
		}
	}
}

// ---------------------------------------------------------------------------
// Notify game mode of destruction (score)
// ---------------------------------------------------------------------------

void AAsteroidSurvivorAsteroid::NotifyGameMode() const
{
	if (AAsteroidSurvivorGameMode* GM = Cast<AAsteroidSurvivorGameMode>(
		UGameplayStatics::GetGameMode(this)))
	{
		GM->OnAsteroidDestroyed(ScoreValue);
	}
}
