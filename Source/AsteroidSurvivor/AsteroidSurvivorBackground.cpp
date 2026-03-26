// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsteroidSurvivorBackground.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"

AAsteroidSurvivorBackground::AAsteroidSurvivorBackground()
{
	PrimaryActorTick.bCanEverTick = true;

	// Root scene component
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Load shared star mesh
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	// Override with M_SolidColor for reliable per-instance colour
	static ConstructorHelpers::FObjectFinder<UMaterial> SolidColorMat(
		TEXT("/Game/Materials/M_SolidColor.M_SolidColor"));

	auto InitISM = [&](UInstancedStaticMeshComponent* ISM)
	{
		ISM->SetupAttachment(Root);
		ISM->SetAbsolute(true, true, true);
		ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ISM->SetCastShadow(false);
		if (SphereMesh.Succeeded())
		{
			ISM->SetStaticMesh(SphereMesh.Object);
		}
		if (SolidColorMat.Succeeded())
		{
			ISM->SetMaterial(0, SolidColorMat.Object);
		}
	};

	// Far stars – many tiny stars, barely scroll
	FarStarsISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FarStars"));
	InitISM(FarStarsISM);

	// Mid stars – medium count, moderate scroll
	MidStarsISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("MidStars"));
	InitISM(MidStarsISM);

	// Near stars – fewer, larger stars, scroll the fastest
	NearStarsISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("NearStars"));
	InitISM(NearStarsISM);

	// Space dust – tiny particles with subtle drift
	SpaceDustISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("SpaceDust"));
	InitISM(SpaceDustISM);
}

void AAsteroidSurvivorBackground::BeginPlay()
{
	Super::BeginPlay();

	// Star layers with increasing size and decreasing count for parallax depth
	CreateStarLayer(FarStarsISM, 300, 0.02f, 0.05f);
	CreateStarLayer(MidStarsISM, 150, 0.04f, 0.09f);
	CreateStarLayer(NearStarsISM, 50, 0.08f, 0.16f);

	// Space dust – many tiny colored particles for atmosphere
	CreateSpaceDustLayer(SpaceDustISM, 200);

	// Color the star layers with subtle tints for visual variety
	ApplyLayerColor(FarStarsISM, FLinearColor(0.6f, 0.6f, 1.0f, 1.0f));
	ApplyLayerColor(MidStarsISM, FLinearColor(0.8f, 0.8f, 1.0f, 1.0f));
	ApplyLayerColor(NearStarsISM, FLinearColor(1.0f, 0.9f, 0.8f, 1.0f));
	ApplyLayerColor(SpaceDustISM, FLinearColor(0.4f, 0.6f, 1.0f, 1.0f));
}

void AAsteroidSurvivorBackground::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	APawn* Ship = UGameplayStatics::GetPlayerPawn(this, 0);
	FVector ShipPos = Ship ? Ship->GetActorLocation() : FVector::ZeroVector;

	UpdateLayerPosition(FarStarsISM, FarParallax, ShipPos, -500.0f);
	UpdateLayerPosition(MidStarsISM, MidParallax, ShipPos, -300.0f);
	UpdateLayerPosition(NearStarsISM, NearParallax, ShipPos, -100.0f);

	// Space dust gets a subtle animated drift on top of parallax scrolling
	DustDriftTime += DeltaTime;
	FVector DustAnimOffset(
		FMath::Sin(DustDriftTime * DustDriftFrequencyX) * DustDriftSpeed,
		FMath::Cos(DustDriftTime * DustDriftFrequencyY) * DustDriftSpeed,
		0.0f
	);
	UpdateLayerPosition(SpaceDustISM, DustParallax, ShipPos + DustAnimOffset, -400.0f);
}

void AAsteroidSurvivorBackground::CreateStarLayer(UInstancedStaticMeshComponent* ISM, int32 NumStars, float MinScale, float MaxScale)
{
	if (!ISM)
	{
		return;
	}

	const float HalfField = FieldSize * 0.5f;

	// Generate base star positions
	TArray<FTransform> BaseStars;
	BaseStars.Reserve(NumStars);

	for (int32 i = 0; i < NumStars; i++)
	{
		FVector Pos(
			FMath::FRandRange(-HalfField, HalfField),
			FMath::FRandRange(-HalfField, HalfField),
			0.0f
		);

		float Scale = FMath::FRandRange(MinScale, MaxScale);

		FTransform InstanceTransform;
		InstanceTransform.SetLocation(Pos);
		InstanceTransform.SetScale3D(FVector(Scale));
		BaseStars.Add(InstanceTransform);
	}

	// Tile 3x3 for seamless wrapping when the ship moves far
	for (int32 dx = -1; dx <= 1; dx++)
	{
		for (int32 dy = -1; dy <= 1; dy++)
		{
			FVector Offset(dx * FieldSize, dy * FieldSize, 0.0f);
			for (const FTransform& T : BaseStars)
			{
				FTransform TiledT = T;
				TiledT.SetLocation(T.GetLocation() + Offset);
				ISM->AddInstance(TiledT, false);
			}
		}
	}
}

void AAsteroidSurvivorBackground::CreateSpaceDustLayer(UInstancedStaticMeshComponent* ISM, int32 NumParticles)
{
	if (!ISM)
	{
		return;
	}

	const float HalfField = FieldSize * 0.5f;

	TArray<FTransform> BaseDust;
	BaseDust.Reserve(NumParticles);

	for (int32 i = 0; i < NumParticles; i++)
	{
		FVector Pos(
			FMath::FRandRange(-HalfField, HalfField),
			FMath::FRandRange(-HalfField, HalfField),
			0.0f
		);

		// Vary dust particle sizes - mostly tiny with some larger wisps
		float Scale = FMath::FRandRange(0.01f, 0.04f);
		if (FMath::FRand() < 0.15f)
		{
			Scale = FMath::FRandRange(0.04f, 0.08f);
		}

		// Elongate some particles for a wispy dust streak effect
		FVector ScaleVec(Scale);
		if (FMath::FRand() < 0.3f)
		{
			float Elongation = FMath::FRandRange(1.5f, 3.0f);
			ScaleVec.X *= Elongation;
		}

		FTransform InstanceTransform;
		InstanceTransform.SetLocation(Pos);
		InstanceTransform.SetScale3D(ScaleVec);
		// Random rotation for variety
		InstanceTransform.SetRotation(FQuat(FRotator(
			FMath::FRandRange(0.0f, 360.0f),
			FMath::FRandRange(0.0f, 360.0f),
			0.0f)));
		BaseDust.Add(InstanceTransform);
	}

	// Tile 3x3 for seamless wrapping
	for (int32 dx = -1; dx <= 1; dx++)
	{
		for (int32 dy = -1; dy <= 1; dy++)
		{
			FVector Offset(dx * FieldSize, dy * FieldSize, 0.0f);
			for (const FTransform& T : BaseDust)
			{
				FTransform TiledT = T;
				TiledT.SetLocation(T.GetLocation() + Offset);
				ISM->AddInstance(TiledT, false);
			}
		}
	}
}

void AAsteroidSurvivorBackground::ApplyLayerColor(UInstancedStaticMeshComponent* ISM, const FLinearColor& Color)
{
	if (!ISM)
	{
		return;
	}

	UMaterialInstanceDynamic* DynMat = ISM->CreateDynamicMaterialInstance(0);
	if (DynMat)
	{
		const FLinearColor EmissiveColor = Color * 2.0f;
		DynMat->SetVectorParameterValue(FName(TEXT("Color")), Color);
		DynMat->SetVectorParameterValue(FName(TEXT("BaseColor")), Color);
		DynMat->SetVectorParameterValue(FName(TEXT("EmissiveColor")), EmissiveColor);
		DynMat->SetVectorParameterValue(FName(TEXT("Emissive Color")), EmissiveColor);
	}
}

/** Wraps a value into the range [-Size/2, Size/2] for seamless parallax tiling */
static float WrapParallaxValue(float Value, float Size)
{
	float HalfSize = Size * 0.5f;
	float Wrapped = FMath::Fmod(Value + HalfSize, Size);
	if (Wrapped < 0.0f)
	{
		Wrapped += Size;
	}
	return Wrapped - HalfSize;
}

void AAsteroidSurvivorBackground::UpdateLayerPosition(UInstancedStaticMeshComponent* ISM, float ParallaxFactor, const FVector& ShipPos, float ZOffset)
{
	if (!ISM)
	{
		return;
	}

	FVector ParallaxOffset;
	ParallaxOffset.X = WrapParallaxValue(ShipPos.X * ParallaxFactor, FieldSize);
	ParallaxOffset.Y = WrapParallaxValue(ShipPos.Y * ParallaxFactor, FieldSize);
	ParallaxOffset.Z = 0.0f;

	FVector LayerPos = ShipPos - ParallaxOffset;
	LayerPos.Z = ZOffset;

	ISM->SetWorldLocation(LayerPos);
}
