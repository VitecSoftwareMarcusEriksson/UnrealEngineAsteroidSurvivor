// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsteroidSurvivorShip.h"
#include "AsteroidSurvivorProjectile.h"
#include "AsteroidSurvivorAsteroid.h"
#include "AsteroidSurvivorGameMode.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Kismet/GameplayStatics.h"

AAsteroidSurvivorShip::AAsteroidSurvivorShip()
{
	PrimaryActorTick.bCanEverTick = true;

	// Invisible root keeps the actor's orientation in the XY play-plane.
	// The visual mesh is attached as a child with its own rotation so that
	// GetActorForwardVector() always lies in the XY plane (critical for
	// top-down movement and projectile direction).
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// Static mesh representing the ship hull
	ShipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
	ShipMesh->SetupAttachment(SceneRoot);
	ShipMesh->SetSimulatePhysics(false);

	// Set up overlap-based collision so the ship can move freely
	// and detect asteroid hits via overlap events.
	ShipMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ShipMesh->SetCollisionObjectType(ECC_Pawn);
	ShipMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	ShipMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	ShipMesh->SetGenerateOverlapEvents(true);

	ShipMesh->OnComponentBeginOverlap.AddDynamic(this, &AAsteroidSurvivorShip::OnShipOverlapBegin);

	// Assign a default visible mesh (cone shape ≈ simple ship silhouette)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultShipMesh(
		TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (DefaultShipMesh.Succeeded())
	{
		ShipMesh->SetStaticMesh(DefaultShipMesh.Object);
	}
	// Rotate so the cone tip points along +X (actor forward).
	// This is a visual-only rotation on the child mesh; it does NOT
	// affect the actor's forward vector because SceneRoot is the root.
	ShipMesh->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	ShipMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.7f));

	// Top-down camera rig – attached to the scene root so it follows
	// the actor position without being affected by mesh rotation.
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(SceneRoot);
	SpringArm->TargetArmLength = 1400.0f;
	SpringArm->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	SpringArm->bDoCollisionTest = false;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritYaw = false;
	SpringArm->bInheritRoll = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;
	Camera->SetProjectionMode(ECameraProjectionMode::Orthographic);
	Camera->OrthoWidth = 2048.0f;

	// Default projectile class so the ship can fire out of the box
	ProjectileClass = AAsteroidSurvivorProjectile::StaticClass();

	// Create default Enhanced Input actions and mapping context.
	// Blueprint subclasses can override these UPROPERTY fields.
	SetupDefaultInputActions();
}

void AAsteroidSurvivorShip::BeginPlay()
{
	Super::BeginPlay();

	// Register Enhanced Input mapping context.
	// For the initial default pawn, the controller is typically set by the
	// time BeginPlay runs. PossessedBy() handles the respawn case.
	RegisterInputMappingContext();
}

void AAsteroidSurvivorShip::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Register the mapping context now that we have a valid controller.
	// This is essential for respawned pawns where BeginPlay runs before
	// Possess, so GetController() was null in BeginPlay.
	RegisterInputMappingContext();
}

void AAsteroidSurvivorShip::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Apply drag (frame-rate independent).
	// Drag is defined as a per-frame retention factor at 60 FPS.
	// Normalise it so the behaviour is consistent at any frame rate.
	const float ReferenceFrameRate = 60.0f;
	Velocity *= FMath::Pow(Drag, DeltaTime * ReferenceFrameRate);

	// Clamp speed
	if (Velocity.SizeSquared() > MaxSpeed * MaxSpeed)
	{
		Velocity = Velocity.GetSafeNormal() * MaxSpeed;
	}

	// Move ship (no sweep – collision is handled via overlap events)
	FVector NewLocation = GetActorLocation() + Velocity * DeltaTime;
	SetActorLocation(NewLocation, false);

	// Auto-fire while button is held
	if (bFiring)
	{
		FireTimer -= DeltaTime;
		if (FireTimer <= 0.0f)
		{
			Fire();
			FireTimer = FireRate;
		}
	}

	// Invulnerability countdown and blinking
	if (bInvulnerable)
	{
		InvulnerabilityTimer -= DeltaTime;
		BlinkTimer -= DeltaTime;

		if (BlinkTimer <= 0.0f)
		{
			bBlinkVisible = !bBlinkVisible;
			if (ShipMesh)
			{
				ShipMesh->SetVisibility(bBlinkVisible);
			}
			BlinkTimer = BlinkInterval;
		}

		if (InvulnerabilityTimer <= 0.0f)
		{
			bInvulnerable = false;
			bBlinkVisible = true;
			if (ShipMesh)
			{
				ShipMesh->SetVisibility(true);
			}
		}
	}
}

void AAsteroidSurvivorShip::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAsteroidSurvivorShip::Move);
		}
		if (RotateAction)
		{
			EIC->BindAction(RotateAction, ETriggerEvent::Triggered, this, &AAsteroidSurvivorShip::Rotate);
		}
		if (FireAction)
		{
			EIC->BindAction(FireAction, ETriggerEvent::Started,  this, &AAsteroidSurvivorShip::StartFire);
			EIC->BindAction(FireAction, ETriggerEvent::Completed, this, &AAsteroidSurvivorShip::StopFire);
		}
	}
}

void AAsteroidSurvivorShip::Move(const FInputActionValue& Value)
{
	float ThrustInput = Value.Get<float>();
	FVector ForwardDir = GetActorForwardVector();
	Velocity += ForwardDir * ThrustInput * ThrustForce * GetWorld()->GetDeltaSeconds();
}

void AAsteroidSurvivorShip::Rotate(const FInputActionValue& Value)
{
	float RotInput = Value.Get<float>();
	FRotator NewRotation = GetActorRotation();
	NewRotation.Yaw += RotInput * RotationSpeed * GetWorld()->GetDeltaSeconds();
	SetActorRotation(NewRotation);
}

void AAsteroidSurvivorShip::StartFire()
{
	bFiring = true;
	FireTimer = 0.0f; // fire immediately on press
}

void AAsteroidSurvivorShip::StopFire()
{
	bFiring = false;
}

void AAsteroidSurvivorShip::Fire()
{
	if (!ProjectileClass)
	{
		return;
	}

	FVector MuzzleLocation = GetActorLocation() + GetActorRotation().RotateVector(MuzzleOffset);
	FRotator MuzzleRotation = GetActorRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	GetWorld()->SpawnActor<AAsteroidSurvivorProjectile>(
		ProjectileClass, MuzzleLocation, MuzzleRotation, SpawnParams);
}

void AAsteroidSurvivorShip::OnShipOverlapBegin(UPrimitiveComponent* OverlappedComp,
                                                AActor* OtherActor,
                                                UPrimitiveComponent* OtherComp,
                                                int32 OtherBodyIndex,
                                                bool bFromSweep,
                                                const FHitResult& SweepResult)
{
	if (bInvulnerable)
	{
		return;
	}

	AAsteroidSurvivorAsteroid* Asteroid = Cast<AAsteroidSurvivorAsteroid>(OtherActor);
	if (!Asteroid)
	{
		return;
	}

	// Destroy the asteroid on contact (no split, no score)
	Asteroid->Destroy();

	// Notify game mode to lose a life
	AAsteroidSurvivorGameMode* GM = Cast<AAsteroidSurvivorGameMode>(
	    UGameplayStatics::GetGameMode(this));
	if (GM)
	{
		GM->OnPlayerShipHit();

		if (GM->IsGameOver())
		{
			Destroy();
		}
		else
		{
			StartInvulnerability();
		}
	}
}

void AAsteroidSurvivorShip::StartInvulnerability()
{
	bInvulnerable = true;
	InvulnerabilityTimer = InvulnerabilityDuration;
	BlinkTimer = BlinkInterval;
	bBlinkVisible = true;
}

void AAsteroidSurvivorShip::RegisterInputMappingContext()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		    ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AAsteroidSurvivorShip::SetupDefaultInputActions()
{
	// Move (Axis1D: positive = forward thrust, negative = reverse)
	MoveAction = NewObject<UInputAction>(this, TEXT("IA_DefaultMove"));
	MoveAction->ValueType = EInputActionValueType::Axis1D;

	// Rotate (Axis1D: positive = turn right, negative = turn left)
	RotateAction = NewObject<UInputAction>(this, TEXT("IA_DefaultRotate"));
	RotateAction->ValueType = EInputActionValueType::Axis1D;

	// Fire (Boolean: pressed / released)
	FireAction = NewObject<UInputAction>(this, TEXT("IA_DefaultFire"));
	FireAction->ValueType = EInputActionValueType::Boolean;

	// Mapping context with default keyboard bindings
	DefaultMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Default"));

	// -- Movement (W / Up = forward, S / Down = reverse) --
	DefaultMappingContext->MapKey(MoveAction, EKeys::W);
	DefaultMappingContext->MapKey(MoveAction, EKeys::Up);

	{
		FEnhancedActionKeyMapping& Mapping = DefaultMappingContext->MapKey(MoveAction, EKeys::S);
		Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(DefaultMappingContext));
	}
	{
		FEnhancedActionKeyMapping& Mapping = DefaultMappingContext->MapKey(MoveAction, EKeys::Down);
		Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(DefaultMappingContext));
	}

	// -- Rotation (D / Right = turn right, A / Left = turn left) --
	DefaultMappingContext->MapKey(RotateAction, EKeys::D);
	DefaultMappingContext->MapKey(RotateAction, EKeys::Right);

	{
		FEnhancedActionKeyMapping& Mapping = DefaultMappingContext->MapKey(RotateAction, EKeys::A);
		Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(DefaultMappingContext));
	}
	{
		FEnhancedActionKeyMapping& Mapping = DefaultMappingContext->MapKey(RotateAction, EKeys::Left);
		Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(DefaultMappingContext));
	}

	// -- Fire (Space / Left mouse button) --
	DefaultMappingContext->MapKey(FireAction, EKeys::SpaceBar);
	DefaultMappingContext->MapKey(FireAction, EKeys::LeftMouseButton);
}
