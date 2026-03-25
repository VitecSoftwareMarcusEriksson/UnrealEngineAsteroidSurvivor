// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsteroidSurvivorTrailParticle.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AAsteroidSurvivorTrailParticle::AAsteroidSurvivorTrailParticle()
{
	PrimaryActorTick.bCanEverTick = true;

	ParticleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ParticleMesh"));
	ParticleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRootComponent(ParticleMesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshAsset(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshAsset.Succeeded())
	{
		ParticleMesh->SetStaticMesh(SphereMeshAsset.Object);
	}
	ParticleMesh->SetRelativeScale3D(FVector(InitialScale));

	TrailLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("TrailLight"));
	TrailLight->SetupAttachment(ParticleMesh);
	TrailLight->SetIntensity(InitialLightIntensity);
	TrailLight->SetLightColor(FLinearColor(1.0f, 0.4f, 0.05f));
	TrailLight->SetAttenuationRadius(120.0f);
	TrailLight->SetCastShadows(false);
}

void AAsteroidSurvivorTrailParticle::BeginPlay()
{
	Super::BeginPlay();

	if (ParticleMesh)
	{
		UMaterialInstanceDynamic* DynMat = ParticleMesh->CreateDynamicMaterialInstance(0);
		if (DynMat)
		{
			// Bright orange/fire emissive color
			const FLinearColor TrailColor(4.0f, 1.5f, 0.2f, 1.0f);
			DynMat->SetVectorParameterValue(FName(TEXT("Color")), TrailColor);
			DynMat->SetVectorParameterValue(FName(TEXT("BaseColor")), TrailColor);
			DynMat->SetVectorParameterValue(FName(TEXT("EmissiveColor")), TrailColor);
			DynMat->SetVectorParameterValue(FName(TEXT("Emissive Color")), TrailColor);
		}
	}
}

void AAsteroidSurvivorTrailParticle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	LifeTimer += DeltaTime;

	if (LifeTimer >= Lifetime)
	{
		Destroy();
		return;
	}

	// Normalized age 0→1
	const float Alpha = LifeTimer / Lifetime;

	// Shrink over time
	const float Scale = InitialScale * (1.0f - Alpha);
	if (ParticleMesh)
	{
		ParticleMesh->SetRelativeScale3D(FVector(FMath::Max(Scale, 0.01f)));
	}

	// Fade light intensity
	if (TrailLight)
	{
		TrailLight->SetIntensity(InitialLightIntensity * (1.0f - Alpha));
	}
}
