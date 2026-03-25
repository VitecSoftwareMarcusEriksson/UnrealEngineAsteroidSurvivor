// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsteroidSurvivorProjectile.h"
#include "AsteroidSurvivorAsteroid.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AAsteroidSurvivorProjectile::AAsteroidSurvivorProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(12.0f);
	SetRootComponent(CollisionSphere);

	// Set up overlap-based collision so the projectile can move freely
	// and detect asteroid hits via overlap events.
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	CollisionSphere->SetGenerateOverlapEvents(true);

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(
		this, &AAsteroidSurvivorProjectile::OnOverlapBegin);

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Default simple sphere mesh (can be replaced via Blueprint)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshAsset(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshAsset.Succeeded())
	{
		ProjectileMesh->SetStaticMesh(SphereMeshAsset.Object);
	}
	ProjectileMesh->SetRelativeScale3D(FVector(0.25f));

	// Point light for a subtle glow; kept small to avoid illuminating the
	// ship when the projectile spawns nearby.
	GlowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("GlowLight"));
	GlowLight->SetupAttachment(RootComponent);
	GlowLight->SetIntensity(3000.0f);
	GlowLight->SetLightColor(FLinearColor(0.0f, 1.0f, 0.3f));
	GlowLight->SetAttenuationRadius(120.0f);
	GlowLight->SetCastShadows(false);
}

void AAsteroidSurvivorProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Create a bright emissive material so the projectile is clearly visible.
	// Try several common parameter names to cover different engine materials.
	if (ProjectileMesh)
	{
		UMaterialInstanceDynamic* DynMat = ProjectileMesh->CreateDynamicMaterialInstance(0);
		if (DynMat)
		{
			const FLinearColor BrightGreen(0.0f, 1.0f, 0.3f, 1.0f);
			const FLinearColor EmissiveGreen(0.0f, 10.0f, 3.0f, 1.0f);

			// BasicShapeMaterial uses "Color"
			DynMat->SetVectorParameterValue(FName(TEXT("Color")), BrightGreen);
			// Standard PBR materials use "BaseColor"
			DynMat->SetVectorParameterValue(FName(TEXT("BaseColor")), BrightGreen);
			// Emissive parameters (various naming conventions)
			DynMat->SetVectorParameterValue(FName(TEXT("EmissiveColor")), EmissiveGreen);
			DynMat->SetVectorParameterValue(FName(TEXT("Emissive Color")), EmissiveGreen);
		}
	}
}

void AAsteroidSurvivorProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Move forward
	FVector Delta = GetActorForwardVector() * Speed * DeltaTime;
	AddActorWorldOffset(Delta, false);

	// Lifetime
	LifeTimer += DeltaTime;
	if (LifeTimer >= Lifetime)
	{
		Destroy();
	}
}

void AAsteroidSurvivorProjectile::OnOverlapBegin(UPrimitiveComponent* OverlappedComp,
                                                   AActor* OtherActor,
                                                   UPrimitiveComponent* OtherComp,
                                                   int32 OtherBodyIndex,
                                                   bool bFromSweep,
                                                   const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor != this && OtherActor != GetOwner())
	{
		if (AAsteroidSurvivorAsteroid* Asteroid = Cast<AAsteroidSurvivorAsteroid>(OtherActor))
		{
			Asteroid->TakeDamage_Asteroid(Damage);
			Destroy();
		}
	}
}
