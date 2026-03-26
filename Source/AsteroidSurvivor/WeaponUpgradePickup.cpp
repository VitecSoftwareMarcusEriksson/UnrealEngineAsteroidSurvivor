// Copyright Epic Games, Inc. All Rights Reserved.

#include "WeaponUpgradePickup.h"
#include "AsteroidSurvivorShip.h"
#include "AsteroidSurvivorGameMode.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"

AWeaponUpgradePickup::AWeaponUpgradePickup()
{
	PrimaryActorTick.bCanEverTick = true;

	// Collision sphere for pickup detection
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(20.0f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionSphere->SetGenerateOverlapEvents(true);
	SetRootComponent(CollisionSphere);

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(
		this, &AWeaponUpgradePickup::OnPickupOverlapBegin);

	// Visual – cylinder shape to distinguish from other pickups
	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(CollisionSphere);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshAsset(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMeshAsset.Succeeded())
	{
		PickupMesh->SetStaticMesh(CylinderMeshAsset.Object);
	}
	PickupMesh->SetRelativeScale3D(FVector(0.2f, 0.2f, 0.1f));

	// Bright white-blue glow
	GlowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("GlowLight"));
	GlowLight->SetupAttachment(RootComponent);
	GlowLight->SetIntensity(40000.0f);
	GlowLight->SetLightColor(FLinearColor(0.5f, 0.8f, 1.0f));
	GlowLight->SetAttenuationRadius(500.0f);
	GlowLight->SetCastShadows(false);
}

void AWeaponUpgradePickup::BeginPlay()
{
	Super::BeginPlay();

	// Bright white / blue emissive material
	if (PickupMesh)
	{
		UMaterialInstanceDynamic* DynMat = PickupMesh->CreateDynamicMaterialInstance(0);
		if (DynMat)
		{
			const FLinearColor WeaponBaseColor(0.3f, 0.6f, 1.0f, 1.0f);
			const FLinearColor WeaponEmissive = WeaponBaseColor * 2.0f;
			DynMat->SetVectorParameterValue(FName(TEXT("Color")), WeaponBaseColor);
			DynMat->SetVectorParameterValue(FName(TEXT("BaseColor")), WeaponBaseColor);
			DynMat->SetVectorParameterValue(FName(TEXT("EmissiveColor")), WeaponEmissive);
			DynMat->SetVectorParameterValue(FName(TEXT("Emissive Color")), WeaponEmissive);
			DynMat->SetScalarParameterValue(FName(TEXT("Metallic")), 0.0f);
			DynMat->SetScalarParameterValue(FName(TEXT("Roughness")), 1.0f);
		}
	}
}

void AWeaponUpgradePickup::InitPickup()
{
	// Randomly select a weapon type
	const int32 TypeIndex = FMath::RandRange(0, 3);
	WeaponType = static_cast<EWeaponType>(TypeIndex);
}

FString AWeaponUpgradePickup::GetWeaponDisplayName(EWeaponType Type)
{
	switch (Type)
	{
	case EWeaponType::SpreadShot:      return TEXT("Spread Shot");
	case EWeaponType::RapidBlaster:    return TEXT("Rapid Blaster");
	case EWeaponType::RearTurret:      return TEXT("Rear Turret");
	case EWeaponType::HomingMissile:   return TEXT("Homing Missile");
	case EWeaponType::BlasterUpgrade:  return TEXT("Blaster Upgrade");
	default:                           return TEXT("Unknown Weapon");
	}
}

void AWeaponUpgradePickup::Tick(float DeltaTime)
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

		float EffectivePullRadius = PullRadius;
		AAsteroidSurvivorShip* Ship = Cast<AAsteroidSurvivorShip>(PlayerPawn);
		if (Ship)
		{
			EffectivePullRadius *= Ship->GetThoriumMagnetMultiplier();
		}

		if (DistToShip <= EffectivePullRadius)
		{
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
			// Slow drift decay
			Velocity *= FMath::Exp(-2.0f * DeltaTime);
		}
	}

	// Move the pickup
	FVector NewLocation = GetActorLocation() + Velocity * DeltaTime;
	SetActorLocation(NewLocation, true);

	// Rotate and pulse for visual interest
	if (PickupMesh)
	{
		FRotator MeshRot = PickupMesh->GetRelativeRotation();
		MeshRot.Yaw += 180.0f * DeltaTime;
		PickupMesh->SetRelativeRotation(MeshRot);

		// Pulsing scale
		const float PulseFactor = 0.8f + 0.2f * FMath::Sin(LifeTimer * 4.0f);
		PickupMesh->SetRelativeScale3D(FVector(0.2f * PulseFactor, 0.2f * PulseFactor, 0.1f * PulseFactor));
	}

	if (GlowLight)
	{
		const float PulseFactor = 0.7f + 0.3f * FMath::Sin(LifeTimer * 5.0f);
		GlowLight->SetIntensity(40000.0f * PulseFactor);
	}
}

void AWeaponUpgradePickup::OnPickupOverlapBegin(
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

	// Add or upgrade the weapon on the player ship
	Ship->AddOrUpgradeWeapon(WeaponType);

	Destroy();
}
