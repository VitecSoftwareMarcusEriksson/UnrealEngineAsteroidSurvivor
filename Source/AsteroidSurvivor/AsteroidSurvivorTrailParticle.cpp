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
			// Bright orange/fire color for engine trail
			const FLinearColor TrailBaseColor(1.0f, 0.4f, 0.05f, 1.0f);
			const FLinearColor TrailEmissive = TrailBaseColor * 2.0f;
			DynMat->SetVectorParameterValue(FName(TEXT("Color")), TrailBaseColor);
			DynMat->SetVectorParameterValue(FName(TEXT("BaseColor")), TrailBaseColor);
			DynMat->SetVectorParameterValue(FName(TEXT("EmissiveColor")), TrailEmissive);
			DynMat->SetVectorParameterValue(FName(TEXT("Emissive Color")), TrailEmissive);
			DynMat->SetScalarParameterValue(FName(TEXT("Metallic")), 0.0f);
			DynMat->SetScalarParameterValue(FName(TEXT("Roughness")), 1.0f);
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

void AAsteroidSurvivorTrailParticle::SetSmokeColor(const FLinearColor& BaseColor)
{
	if (ParticleMesh)
	{
		UMaterialInstanceDynamic* DynMat = Cast<UMaterialInstanceDynamic>(ParticleMesh->GetMaterial(0));
		if (!DynMat)
		{
			DynMat = ParticleMesh->CreateDynamicMaterialInstance(0);
		}
		if (DynMat)
		{
			const FLinearColor SmokeEmissive = BaseColor * 2.0f;
			DynMat->SetVectorParameterValue(FName(TEXT("Color")), BaseColor);
			DynMat->SetVectorParameterValue(FName(TEXT("BaseColor")), BaseColor);
			DynMat->SetVectorParameterValue(FName(TEXT("EmissiveColor")), SmokeEmissive);
			DynMat->SetVectorParameterValue(FName(TEXT("Emissive Color")), SmokeEmissive);
			DynMat->SetScalarParameterValue(FName(TEXT("Metallic")), 0.0f);
			DynMat->SetScalarParameterValue(FName(TEXT("Roughness")), 1.0f);
		}
	}

	if (TrailLight)
	{
		TrailLight->SetLightColor(BaseColor);
	}
}
