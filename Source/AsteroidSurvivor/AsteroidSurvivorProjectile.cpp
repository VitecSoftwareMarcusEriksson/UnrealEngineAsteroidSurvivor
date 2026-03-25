// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsteroidSurvivorProjectile.h"
#include "AsteroidSurvivorAsteroid.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "UObject/ConstructorHelpers.h"

AAsteroidSurvivorProjectile::AAsteroidSurvivorProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(12.0f);
	CollisionSphere->SetCollisionProfileName(TEXT("Projectile"));
	SetRootComponent(CollisionSphere);

	CollisionSphere->OnComponentHit.AddDynamic(this, &AAsteroidSurvivorProjectile::OnHit);
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

	// Bright green glow so the projectile is easy to see
	GlowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("GlowLight"));
	GlowLight->SetupAttachment(RootComponent);
	GlowLight->SetIntensity(3000.0f);
	GlowLight->SetLightColor(FLinearColor(0.0f, 1.0f, 0.3f));
	GlowLight->SetAttenuationRadius(150.0f);
	GlowLight->SetCastShadows(false);
}

void AAsteroidSurvivorProjectile::BeginPlay()
{
	Super::BeginPlay();
}

void AAsteroidSurvivorProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Move forward
	FVector Delta = GetActorForwardVector() * Speed * DeltaTime;
	AddActorWorldOffset(Delta, true);

	// Lifetime
	LifeTimer += DeltaTime;
	if (LifeTimer >= Lifetime)
	{
		Destroy();
	}
}

void AAsteroidSurvivorProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
                                         UPrimitiveComponent* OtherComp,
                                         FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor && OtherActor != this && OtherActor != GetOwner())
	{
		if (AAsteroidSurvivorAsteroid* Asteroid = Cast<AAsteroidSurvivorAsteroid>(OtherActor))
		{
			Asteroid->TakeDamage_Asteroid(Damage);
		}
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
		}
		Destroy();
	}
}
