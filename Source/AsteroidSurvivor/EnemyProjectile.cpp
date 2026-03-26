// Copyright Epic Games, Inc. All Rights Reserved.

#include "EnemyProjectile.h"
#include "AsteroidSurvivorShip.h"
#include "AsteroidSurvivorGameMode.h"
#include "EnemyShipBase.h"
#include "SolidColorMaterialHelper.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"

AEnemyProjectile::AEnemyProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	// Small collision sphere
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(10.0f);
	SetRootComponent(CollisionSphere);

	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	// Overlap with player pawn
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionSphere->SetGenerateOverlapEvents(true);

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(
		this, &AEnemyProjectile::OnOverlapBegin);

	// Visual – small sphere with red/orange emissive
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshAsset(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshAsset.Succeeded())
	{
		ProjectileMesh->SetStaticMesh(SphereMeshAsset.Object);
	}
	ProjectileMesh->SetRelativeScale3D(FVector(0.2f));

	// Red glow light
	GlowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("GlowLight"));
	GlowLight->SetupAttachment(RootComponent);
	GlowLight->SetIntensity(30000.0f);
	GlowLight->SetLightColor(FLinearColor(1.0f, 0.3f, 0.1f));
	GlowLight->SetAttenuationRadius(500.0f);
	GlowLight->SetCastShadows(false);
}

void AEnemyProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Red-orange emissive material to distinguish from player projectiles
	if (ProjectileMesh)
	{
		// Load the shared solid-colour material (with runtime fallback).
		UMaterial* SolidColorMat = FSolidColorMaterialHelper::GetOrCreateMaterial();
		if (SolidColorMat)
		{
			ProjectileMesh->SetMaterial(0, SolidColorMat);
		}

		UMaterialInstanceDynamic* DynMat = ProjectileMesh->CreateDynamicMaterialInstance(0);
		if (DynMat)
		{
			// Bright neon red-orange for enemy projectiles – clearly distinct from green player shots.
			const FLinearColor BaseRed(1.0f, 0.15f, 0.05f, 1.0f);
			const FLinearColor EmissiveRed = BaseRed * 6.0f;
			DynMat->SetVectorParameterValue(FName(TEXT("Color")), BaseRed);
			DynMat->SetVectorParameterValue(FName(TEXT("BaseColor")), BaseRed);
			DynMat->SetVectorParameterValue(FName(TEXT("EmissiveColor")), EmissiveRed);
			DynMat->SetVectorParameterValue(FName(TEXT("Emissive Color")), EmissiveRed);
		}
	}
}

void AEnemyProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Freeze movement during upgrade selection
	AAsteroidSurvivorGameMode* GM = Cast<AAsteroidSurvivorGameMode>(
		UGameplayStatics::GetGameMode(this));
	if (GM && GM->IsSelectingUpgrade())
	{
		return;
	}

	// Move forward along the firing direction
	FVector Delta = GetActorForwardVector() * Speed * DeltaTime;
	AddActorWorldOffset(Delta, true);

	// Auto-destroy after lifetime expires
	LifeTimer += DeltaTime;
	if (LifeTimer >= Lifetime)
	{
		Destroy();
	}
}

void AEnemyProjectile::OnOverlapBegin(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this || OtherActor == GetOwner())
	{
		return;
	}

	// Ignore other enemy ships and enemy projectiles
	if (OtherActor->IsA(AEnemyShipBase::StaticClass()) ||
	    OtherActor->IsA(AEnemyProjectile::StaticClass()))
	{
		return;
	}

	// Damage is applied by the ship's overlap handler (OnShipOverlapBegin)
	// – we just destroy ourselves on contact with the player.
	if (OtherActor->IsA(AAsteroidSurvivorShip::StaticClass()))
	{
		Destroy();
	}
}
