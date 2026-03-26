// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsteroidSurvivorProjectile.h"
#include "AsteroidSurvivorAsteroid.h"
#include "AsteroidSurvivorGameMode.h"
#include "AsteroidSurvivorTrailParticle.h"
#include "EnemyShipBase.h"
#include "SolidColorMaterialHelper.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

AAsteroidSurvivorProjectile::AAsteroidSurvivorProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(12.0f);
	SetRootComponent(CollisionSphere);

	// Set up overlap-based collision so the projectile can move freely
	// and detect hits via overlap events.
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
	GlowLight->SetIntensity(40000.0f);
	GlowLight->SetLightColor(FLinearColor(0.2f, 1.0f, 0.1f));
	GlowLight->SetAttenuationRadius(600.0f);
	GlowLight->SetCastShadows(false);
}

void AAsteroidSurvivorProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Create a bright green material so the projectile is clearly visible.
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
			// Bright neon green for player projectiles – clearly distinct from red enemy shots.
			const FLinearColor BaseGreen(0.2f, 1.0f, 0.1f, 1.0f);
			const FLinearColor EmissiveGreen = BaseGreen * 6.0f;

			// BasicShapeMaterial uses "Color"
			DynMat->SetVectorParameterValue(FName(TEXT("Color")), BaseGreen);
			// Standard PBR materials use "BaseColor"
			DynMat->SetVectorParameterValue(FName(TEXT("BaseColor")), BaseGreen);
			// Emissive parameters (various naming conventions)
			DynMat->SetVectorParameterValue(FName(TEXT("EmissiveColor")), EmissiveGreen);
			DynMat->SetVectorParameterValue(FName(TEXT("Emissive Color")), EmissiveGreen);
		}
	}
}

void AAsteroidSurvivorProjectile::SetHomingMissile(bool bHoming)
{
	bIsHomingMissile = bHoming;

	if (!bHoming)
	{
		return;
	}

	// Scale up the missile slightly
	if (ProjectileMesh)
	{
		ProjectileMesh->SetRelativeScale3D(FVector(HomingMissileScale));

		// Apply orange-red color via dynamic material
		UMaterialInstanceDynamic* DynMat = Cast<UMaterialInstanceDynamic>(ProjectileMesh->GetMaterial(0));
		if (!DynMat)
		{
			DynMat = ProjectileMesh->CreateDynamicMaterialInstance(0);
		}
		if (DynMat)
		{
			const FLinearColor MissileBase(1.0f, 0.5f, 0.1f, 1.0f);
			const FLinearColor MissileEmissive = MissileBase * 6.0f;
			DynMat->SetVectorParameterValue(FName(TEXT("Color")), MissileBase);
			DynMat->SetVectorParameterValue(FName(TEXT("BaseColor")), MissileBase);
			DynMat->SetVectorParameterValue(FName(TEXT("EmissiveColor")), MissileEmissive);
			DynMat->SetVectorParameterValue(FName(TEXT("Emissive Color")), MissileEmissive);
		}
	}

	// Change glow light to orange-red
	if (GlowLight)
	{
		GlowLight->SetLightColor(FLinearColor(1.0f, 0.5f, 0.1f));
	}
}

void AAsteroidSurvivorProjectile::SetDamage(int32 NewDamage)
{
	Damage = NewDamage;
}

void AAsteroidSurvivorProjectile::SetSpeed(float NewSpeed)
{
	Speed = NewSpeed;
}

void AAsteroidSurvivorProjectile::SetScaleMultiplier(float Mult)
{
	if (ProjectileMesh)
	{
		ProjectileMesh->SetRelativeScale3D(FVector(0.25f * Mult));
	}
}

void AAsteroidSurvivorProjectile::SetExplosiveRounds(bool bHasExplosion, float InRadius)
{
	bIsExplosive = bHasExplosion;
	ExplosionRadius = InRadius;
}

void AAsteroidSurvivorProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Freeze movement during upgrade selection
	AAsteroidSurvivorGameMode* GM = Cast<AAsteroidSurvivorGameMode>(
		UGameplayStatics::GetGameMode(this));
	if (GM && GM->IsSelectingUpgrade())
	{
		return;
	}

	// Move forward
	FVector Delta = GetActorForwardVector() * Speed * DeltaTime;
	AddActorWorldOffset(Delta, true);

	// Smoke trail for homing missiles
	if (bIsHomingMissile)
	{
		HomingMissileTrailTimer -= DeltaTime;
		if (HomingMissileTrailTimer <= 0.0f)
		{
			// Spawn smoke trail behind the missile
			const FVector TrailOffset = GetActorForwardVector() * HomingMissileTrailOffset;
			const FVector TrailLocation = GetActorLocation() + TrailOffset;

			FActorSpawnParameters TrailSpawnParams;
			TrailSpawnParams.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			AAsteroidSurvivorTrailParticle* Trail = GetWorld()->SpawnActor<AAsteroidSurvivorTrailParticle>(
				AAsteroidSurvivorTrailParticle::StaticClass(),
				TrailLocation, FRotator::ZeroRotator, TrailSpawnParams);

			if (Trail)
			{
				static const FLinearColor MissileSmokeColor(0.6f, 0.6f, 0.6f, 1.0f);
				Trail->SetSmokeColor(MissileSmokeColor);
			}

			HomingMissileTrailTimer = HomingMissileTrailInterval;
		}
	}

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
	if (OtherActor && OtherActor != this && OtherActor != GetOwner()
		&& !OtherActor->IsA(AAsteroidSurvivorProjectile::StaticClass()))
	{
		// Explosive rounds: deal splash damage to nearby asteroids and enemies
		if (bIsExplosive && ExplosionRadius > 0.0f)
		{
			const FVector ExplosionLoc = GetActorLocation();
			// Splash deals half the projectile's damage, minimum 1
			const int32 SplashDamage = FMath::Max(1, Damage / 2);

			// Visual explosion effect – spawn trail particles in a ring
			const int32 ExplosionParticleCount = 8;
			for (int32 p = 0; p < ExplosionParticleCount; ++p)
			{
				const float Angle = (360.0f / ExplosionParticleCount) * p;
				FVector ParticleDir(
					FMath::Cos(FMath::DegreesToRadians(Angle)),
					FMath::Sin(FMath::DegreesToRadians(Angle)),
					0.0f);

				const FVector ParticleLoc = ExplosionLoc + ParticleDir * FMath::FRandRange(10.0f, 40.0f);

				FActorSpawnParameters ExpSpawnParams;
				ExpSpawnParams.SpawnCollisionHandlingOverride =
					ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

				AAsteroidSurvivorTrailParticle* ExpParticle =
					GetWorld()->SpawnActor<AAsteroidSurvivorTrailParticle>(
						AAsteroidSurvivorTrailParticle::StaticClass(),
						ParticleLoc, FRotator::ZeroRotator, ExpSpawnParams);
				if (ExpParticle)
				{
					// Bright orange-yellow explosion color
					static const FLinearColor ExplosionColor(1.0f, 0.6f, 0.1f, 1.0f);
					ExpParticle->SetSmokeColor(ExplosionColor);
				}
			}

			for (TActorIterator<AAsteroidSurvivorAsteroid> It(GetWorld()); It; ++It)
			{
				if (*It != OtherActor &&
					FVector::Dist(ExplosionLoc, (*It)->GetActorLocation()) <= ExplosionRadius)
				{
					(*It)->ApplySplashDamage(SplashDamage);
				}
			}

			for (TActorIterator<AEnemyShipBase> It(GetWorld()); It; ++It)
			{
				if (*It != OtherActor &&
					FVector::Dist(ExplosionLoc, (*It)->GetActorLocation()) <= ExplosionRadius)
				{
					(*It)->ApplyDamage(static_cast<float>(SplashDamage));
				}
			}
		}

		Destroy();
	}
}
