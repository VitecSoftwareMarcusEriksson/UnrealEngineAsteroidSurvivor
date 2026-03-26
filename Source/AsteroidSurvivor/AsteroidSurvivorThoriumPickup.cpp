// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsteroidSurvivorThoriumPickup.h"
#include "AsteroidSurvivorShip.h"
#include "AsteroidSurvivorGameMode.h"
#include "SolidColorMaterialHelper.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"

AAsteroidSurvivorThoriumPickup::AAsteroidSurvivorThoriumPickup()
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
		this, &AAsteroidSurvivorThoriumPickup::OnPickupOverlapBegin);

	// Glowing mesh
	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(CollisionSphere);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshAsset(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshAsset.Succeeded())
	{
		PickupMesh->SetStaticMesh(SphereMeshAsset.Object);
	}
	PickupMesh->SetRelativeScale3D(FVector(0.18f));

	// Outer glow mesh for enhanced visibility
	OuterGlowMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OuterGlowMesh"));
	OuterGlowMesh->SetupAttachment(CollisionSphere);
	OuterGlowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (SphereMeshAsset.Succeeded())
	{
		OuterGlowMesh->SetStaticMesh(SphereMeshAsset.Object);
	}
	OuterGlowMesh->SetRelativeScale3D(FVector(OuterGlowScale));

	// Bright glow light
	GlowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("GlowLight"));
	GlowLight->SetupAttachment(RootComponent);
	GlowLight->SetIntensity(30000.0f);
	GlowLight->SetLightColor(FLinearColor(0.2f, 0.8f, 1.0f));
	GlowLight->SetAttenuationRadius(450.0f);
	GlowLight->SetCastShadows(false);
}

void AAsteroidSurvivorThoriumPickup::BeginPlay()
{
	Super::BeginPlay();

	// Bright cyan / teal emissive material for the Thorium particle
	// Load the shared solid-colour material (with runtime fallback).
	UMaterial* SolidColorMat = FSolidColorMaterialHelper::GetOrCreateMaterial();

	if (PickupMesh)
	{
		if (SolidColorMat)
		{
			PickupMesh->SetMaterial(0, SolidColorMat);
		}

		UMaterialInstanceDynamic* DynMat = PickupMesh->CreateDynamicMaterialInstance(0);
		if (DynMat)
		{
			const FLinearColor ThoriumBaseColor(0.1f, 0.8f, 1.0f, 1.0f);
			const FLinearColor ThoriumEmissive = ThoriumBaseColor * 3.0f;
			DynMat->SetVectorParameterValue(FName(TEXT("Color")), ThoriumBaseColor);
			DynMat->SetVectorParameterValue(FName(TEXT("BaseColor")), ThoriumBaseColor);
			DynMat->SetVectorParameterValue(FName(TEXT("EmissiveColor")), ThoriumEmissive);
			DynMat->SetVectorParameterValue(FName(TEXT("Emissive Color")), ThoriumEmissive);
		}
	}

	// Outer glow mesh – translucent bright aura
	if (OuterGlowMesh)
	{
		if (SolidColorMat)
		{
			OuterGlowMesh->SetMaterial(0, SolidColorMat);
		}

		UMaterialInstanceDynamic* GlowMat = OuterGlowMesh->CreateDynamicMaterialInstance(0);
		if (GlowMat)
		{
			const FLinearColor GlowBaseColor(0.1f, 0.6f, 0.8f, 0.4f);
			const FLinearColor GlowEmissive = FLinearColor(GlowBaseColor.R, GlowBaseColor.G, GlowBaseColor.B, 1.0f) * 1.5f;
			GlowMat->SetVectorParameterValue(FName(TEXT("Color")), GlowBaseColor);
			GlowMat->SetVectorParameterValue(FName(TEXT("BaseColor")), GlowBaseColor);
			GlowMat->SetVectorParameterValue(FName(TEXT("EmissiveColor")), GlowEmissive);
			GlowMat->SetVectorParameterValue(FName(TEXT("Emissive Color")), GlowEmissive);
		}
	}
}

void AAsteroidSurvivorThoriumPickup::InitPickup(int32 InThoriumAmount, const FVector& InDriftVelocity)
{
	ThoriumAmount = InThoriumAmount;
	Velocity = InDriftVelocity;
}

void AAsteroidSurvivorThoriumPickup::Tick(float DeltaTime)
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

		// Determine effective pull radius (the ship may have an upgraded magnet)
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

			// Clamp pull speed
			if (Velocity.SizeSquared() > MaxPullSpeed * MaxPullSpeed)
			{
				Velocity = Velocity.GetSafeNormal() * MaxPullSpeed;
			}
		}
		else if (!bBeingPulled)
		{
			// Apply drift drag to slow down the initial scatter velocity
			Velocity *= FMath::Exp(-DriftDrag * DeltaTime);
		}
	}

	// Move the pickup
	FVector NewLocation = GetActorLocation() + Velocity * DeltaTime;
	SetActorLocation(NewLocation, true);

	// Gentle bob effect (sine wave on Z for visual interest)
	float Bob = FMath::Sin(LifeTimer * 4.0f) * 2.0f;
	if (PickupMesh)
	{
		PickupMesh->SetRelativeLocation(FVector(0.0f, 0.0f, Bob));
	}

	// Pulsing glow effect – scale the outer glow and modulate light intensity
	const float PulsePhase = LifeTimer * 6.0f;
	const float PulseFactor = 0.7f + 0.3f * FMath::Sin(PulsePhase);
	if (OuterGlowMesh)
	{
		const float OuterScale = OuterGlowScale * PulseFactor;
		OuterGlowMesh->SetRelativeScale3D(FVector(OuterScale));
		OuterGlowMesh->SetRelativeLocation(FVector(0.0f, 0.0f, Bob));
	}
	if (GlowLight)
	{
		GlowLight->SetIntensity(30000.0f * PulseFactor);
	}
}

void AAsteroidSurvivorThoriumPickup::OnPickupOverlapBegin(
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

	// Add Thorium energy to the game mode
	AAsteroidSurvivorGameMode* GM = Cast<AAsteroidSurvivorGameMode>(
		UGameplayStatics::GetGameMode(this));
	if (GM)
	{
		GM->AddThorium(ThoriumAmount);
	}

	Destroy();
}
