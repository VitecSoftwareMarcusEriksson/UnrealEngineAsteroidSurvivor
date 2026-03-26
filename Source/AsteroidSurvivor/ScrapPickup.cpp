// Copyright Epic Games, Inc. All Rights Reserved.

#include "ScrapPickup.h"
#include "AsteroidSurvivorShip.h"
#include "AsteroidSurvivorGameMode.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"

AScrapPickup::AScrapPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	// Small collision sphere for pickup detection
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(15.0f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionSphere->SetGenerateOverlapEvents(true);
	SetRootComponent(CollisionSphere);

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(
		this, &AScrapPickup::OnPickupOverlapBegin);

	// Visual mesh – small cube to distinguish from spherical Thorium pickups
	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(CollisionSphere);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshAsset.Succeeded())
	{
		PickupMesh->SetStaticMesh(CubeMeshAsset.Object);
	}
	PickupMesh->SetRelativeScale3D(FVector(0.12f));

	// Override with M_SolidColor for reliable per-instance colour
	static ConstructorHelpers::FObjectFinder<UMaterial> SolidColorMat(
		TEXT("/Game/Materials/M_SolidColor.M_SolidColor"));
	if (SolidColorMat.Succeeded())
	{
		PickupMesh->SetMaterial(0, SolidColorMat.Object);
	}

	// Orange / gold glow
	GlowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("GlowLight"));
	GlowLight->SetupAttachment(RootComponent);
	GlowLight->SetIntensity(22000.0f);
	GlowLight->SetLightColor(FLinearColor(1.0f, 0.7f, 0.1f));
	GlowLight->SetAttenuationRadius(350.0f);
	GlowLight->SetCastShadows(false);
}

void AScrapPickup::BeginPlay()
{
	Super::BeginPlay();

	// Orange / gold emissive material
	if (PickupMesh)
	{
		UMaterialInstanceDynamic* DynMat = PickupMesh->CreateDynamicMaterialInstance(0);
		if (DynMat)
		{
			const FLinearColor ScrapBaseColor(1.0f, 0.7f, 0.1f, 1.0f);
			const FLinearColor ScrapEmissive = ScrapBaseColor * 3.0f;
			DynMat->SetVectorParameterValue(FName(TEXT("Color")), ScrapBaseColor);
			DynMat->SetVectorParameterValue(FName(TEXT("BaseColor")), ScrapBaseColor);
			DynMat->SetVectorParameterValue(FName(TEXT("EmissiveColor")), ScrapEmissive);
			DynMat->SetVectorParameterValue(FName(TEXT("Emissive Color")), ScrapEmissive);
		}
	}
}

void AScrapPickup::InitPickup(int32 InScrapAmount, const FVector& InDriftVelocity)
{
	ScrapAmount = InScrapAmount;
	Velocity = InDriftVelocity;
}

void AScrapPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Freeze movement during upgrade selection
	AAsteroidSurvivorGameMode* GM = Cast<AAsteroidSurvivorGameMode>(
		UGameplayStatics::GetGameMode(this));
	if (GM && GM->IsSelectingUpgrade())
	{
		return;
	}

	// Lifetime countdown
	LifeTimer += DeltaTime;
	if (LifeTimer >= Lifetime)
	{
		Destroy();
		return;
	}

	// Find the player ship
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);

	if (PlayerPawn)
	{
		const float DistToShip = FVector::Dist(GetActorLocation(), PlayerPawn->GetActorLocation());

		// Pull radius is affected by the player's Thorium magnet upgrade (reuse same mechanic)
		float EffectivePullRadius = PullRadius;
		AAsteroidSurvivorShip* Ship = Cast<AAsteroidSurvivorShip>(PlayerPawn);
		if (Ship)
		{
			EffectivePullRadius *= Ship->GetThoriumMagnetMultiplier();
		}

		if (DistToShip <= EffectivePullRadius)
		{
			// Pull toward ship
			bBeingPulled = true;
			FVector DirectionToShip = (PlayerPawn->GetActorLocation() - GetActorLocation()).GetSafeNormal();
			Velocity += DirectionToShip * PullAcceleration * DeltaTime;

			if (Velocity.SizeSquared() > MaxPullSpeed * MaxPullSpeed)
			{
				Velocity = Velocity.GetSafeNormal() * MaxPullSpeed;
			}
		}
		else if (!bBeingPulled)
		{
			// Apply drift drag
			Velocity *= FMath::Exp(-DriftDrag * DeltaTime);
		}
	}

	// Move the pickup
	FVector NewLocation = GetActorLocation() + Velocity * DeltaTime;
	SetActorLocation(NewLocation, true);

	// Slow rotation for visual interest
	if (PickupMesh)
	{
		FRotator MeshRot = PickupMesh->GetRelativeRotation();
		MeshRot.Yaw += 120.0f * DeltaTime;
		PickupMesh->SetRelativeRotation(MeshRot);
	}

	// Pulsing glow
	const float PulsePhase = LifeTimer * 5.0f;
	const float PulseFactor = 0.7f + 0.3f * FMath::Sin(PulsePhase);
	if (GlowLight)
	{
		GlowLight->SetIntensity(22000.0f * PulseFactor);
	}
}

void AScrapPickup::OnPickupOverlapBegin(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this)
	{
		return;
	}

	AAsteroidSurvivorShip* Ship = Cast<AAsteroidSurvivorShip>(OtherActor);
	if (!Ship)
	{
		return;
	}

	// Add scrap to the game mode
	AAsteroidSurvivorGameMode* GM = Cast<AAsteroidSurvivorGameMode>(
		UGameplayStatics::GetGameMode(this));
	if (GM)
	{
		GM->AddScrap(ScrapAmount);
	}

	Destroy();
}
