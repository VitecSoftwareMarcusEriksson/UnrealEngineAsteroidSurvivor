// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsteroidSurvivorBackground.h"
#include "Components/InstancedStaticMeshComponent.h"
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

	// Far stars – many tiny stars, barely scroll
	FarStarsISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FarStars"));
	FarStarsISM->SetupAttachment(Root);
	FarStarsISM->SetAbsolute(true, true, true);
	FarStarsISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FarStarsISM->SetCastShadow(false);
	if (SphereMesh.Succeeded())
	{
		FarStarsISM->SetStaticMesh(SphereMesh.Object);
	}

	// Mid stars – medium count, moderate scroll
	MidStarsISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("MidStars"));
	MidStarsISM->SetupAttachment(Root);
	MidStarsISM->SetAbsolute(true, true, true);
	MidStarsISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MidStarsISM->SetCastShadow(false);
	if (SphereMesh.Succeeded())
	{
		MidStarsISM->SetStaticMesh(SphereMesh.Object);
	}

	// Near stars – fewer, larger stars, scroll the fastest
	NearStarsISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("NearStars"));
	NearStarsISM->SetupAttachment(Root);
	NearStarsISM->SetAbsolute(true, true, true);
	NearStarsISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NearStarsISM->SetCastShadow(false);
	if (SphereMesh.Succeeded())
	{
		NearStarsISM->SetStaticMesh(SphereMesh.Object);
	}
}

void AAsteroidSurvivorBackground::BeginPlay()
{
	Super::BeginPlay();

	CreateStarLayer(FarStarsISM, 300, 0.02f, 0.05f);
	CreateStarLayer(MidStarsISM, 150, 0.04f, 0.09f);
	CreateStarLayer(NearStarsISM, 50, 0.08f, 0.16f);
}

void AAsteroidSurvivorBackground::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	APawn* Ship = UGameplayStatics::GetPlayerPawn(this, 0);
	FVector ShipPos = Ship ? Ship->GetActorLocation() : FVector::ZeroVector;

	UpdateLayerPosition(FarStarsISM, FarParallax, ShipPos, -500.0f);
	UpdateLayerPosition(MidStarsISM, MidParallax, ShipPos, -300.0f);
	UpdateLayerPosition(NearStarsISM, NearParallax, ShipPos, -100.0f);
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

void AAsteroidSurvivorBackground::UpdateLayerPosition(UInstancedStaticMeshComponent* ISM, float ParallaxFactor, const FVector& ShipPos, float ZOffset)
{
	if (!ISM)
	{
		return;
	}

	// Wrap the parallax offset within the field size for seamless tiling
	auto WrapValue = [](float Value, float Size) -> float
	{
		float HalfSize = Size * 0.5f;
		float Wrapped = FMath::Fmod(Value + HalfSize, Size);
		if (Wrapped < 0.0f)
		{
			Wrapped += Size;
		}
		return Wrapped - HalfSize;
	};

	FVector ParallaxOffset;
	ParallaxOffset.X = WrapValue(ShipPos.X * ParallaxFactor, FieldSize);
	ParallaxOffset.Y = WrapValue(ShipPos.Y * ParallaxFactor, FieldSize);
	ParallaxOffset.Z = 0.0f;

	FVector LayerPos = ShipPos - ParallaxOffset;
	LayerPos.Z = ZOffset;

	ISM->SetWorldLocation(LayerPos);
}
