// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsteroidSurvivorShip.h"
#include "AsteroidSurvivorProjectile.h"
#include "AsteroidSurvivorAsteroid.h"
#include "AsteroidSurvivorGameMode.h"
#include "AsteroidSurvivorTrailParticle.h"
#include "EnemyShipBase.h"
#include "EnemyProjectile.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

AAsteroidSurvivorShip::AAsteroidSurvivorShip()
{
	PrimaryActorTick.bCanEverTick = true;

	// Collision sphere as root – provides reliable overlap detection
	// and is the component moved by SetActorLocation.
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(40.0f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_Pawn);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	CollisionSphere->SetGenerateOverlapEvents(true);
	SetRootComponent(CollisionSphere);

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(
		this, &AAsteroidSurvivorShip::OnShipOverlapBegin);

	// Static mesh representing the ship hull (visual only, no collision)
	ShipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
	ShipMesh->SetupAttachment(CollisionSphere);
	ShipMesh->SetSimulatePhysics(false);
	ShipMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Assign a default visible mesh (cone shape ≈ simple ship silhouette)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultShipMesh(
		TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (DefaultShipMesh.Succeeded())
	{
		ShipMesh->SetStaticMesh(DefaultShipMesh.Object);
	}
	// Rotate so the cone tip points along +X (actor forward).
	// This is a visual-only rotation on the child mesh; it does NOT
	// affect the actor's forward vector because CollisionSphere is the root.
	ShipMesh->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	ShipMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.7f));

	// Shield visual mesh (translucent sphere around the ship)
	ShieldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShieldMesh"));
	ShieldMesh->SetupAttachment(CollisionSphere);
	ShieldMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ShieldSphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (ShieldSphereMesh.Succeeded())
	{
		ShieldMesh->SetStaticMesh(ShieldSphereMesh.Object);
	}
	ShieldMesh->SetRelativeScale3D(FVector(1.2f));
	ShieldMesh->SetVisibility(false);

	// Glow light for player ship visibility
	UPointLightComponent* ShipGlow = CreateDefaultSubobject<UPointLightComponent>(TEXT("ShipGlow"));
	ShipGlow->SetupAttachment(CollisionSphere);
	ShipGlow->SetIntensity(15000.0f);
	ShipGlow->SetLightColor(FLinearColor(0.3f, 0.8f, 1.0f));
	ShipGlow->SetAttenuationRadius(400.0f);
	ShipGlow->SetCastShadows(false);

	// Top-down camera rig – attached to the collision sphere root so it follows
	// the actor position without being affected by mesh rotation.
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(CollisionSphere);
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

	// Initialise health
	MaxHealth = BaseMaxHealth;
	CurrentHealth = MaxHealth;

	// Apply a bright metallic cyan ship material so the player stands out.
	// Use HDR emissive values and multiple parameter names for material compatibility.
	if (ShipMesh)
	{
		UMaterialInstanceDynamic* DynMat = ShipMesh->CreateDynamicMaterialInstance(0);
		if (DynMat)
		{
			const FLinearColor ShipBaseColor(0.1f, 0.7f, 1.0f, 1.0f);
			const FLinearColor ShipEmissive = ShipBaseColor * 2.0f;
			DynMat->SetVectorParameterValue(FName(TEXT("Color")), ShipBaseColor);
			DynMat->SetVectorParameterValue(FName(TEXT("BaseColor")), ShipBaseColor);
			DynMat->SetVectorParameterValue(FName(TEXT("EmissiveColor")), ShipEmissive);
			DynMat->SetVectorParameterValue(FName(TEXT("Emissive Color")), ShipEmissive);
		}
	}

	// Shield material – translucent cyan glow
	if (ShieldMesh)
	{
		UMaterialInstanceDynamic* ShieldMat = ShieldMesh->CreateDynamicMaterialInstance(0);
		if (ShieldMat)
		{
			const FLinearColor ShieldBaseColor(0.2f, 0.6f, 1.0f, 0.25f);
			const FLinearColor ShieldEmissive(0.1f, 0.3f, 0.5f, 0.25f);
			ShieldMat->SetVectorParameterValue(FName(TEXT("Color")), ShieldBaseColor);
			ShieldMat->SetVectorParameterValue(FName(TEXT("BaseColor")), ShieldBaseColor);
			ShieldMat->SetVectorParameterValue(FName(TEXT("EmissiveColor")), ShieldEmissive);
			ShieldMat->SetVectorParameterValue(FName(TEXT("Emissive Color")), ShieldEmissive);
		}
	}

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

	// Check if upgrade selection is active – skip gameplay updates
	AAsteroidSurvivorGameMode* GM = Cast<AAsteroidSurvivorGameMode>(
		UGameplayStatics::GetGameMode(this));
	const bool bUpgradeActive = GM && GM->IsSelectingUpgrade();

	if (!bUpgradeActive)
	{
		// Apply drag (frame-rate independent).
		// Drag is defined as a per-frame retention factor at 60 FPS.
		// Normalise it so the behaviour is consistent at any frame rate.
		const float ReferenceFrameRate = 60.0f;
		Velocity *= FMath::Pow(Drag, DeltaTime * ReferenceFrameRate);

		// Effective stats with upgrade multipliers
		const float EffectiveMaxSpeed = BaseMaxSpeed * SpeedMultiplier;

		// Clamp speed
		if (Velocity.SizeSquared() > EffectiveMaxSpeed * EffectiveMaxSpeed)
		{
			Velocity = Velocity.GetSafeNormal() * EffectiveMaxSpeed;
		}

		// Move ship with sweep to detect overlaps along the movement path
		FVector NewLocation = GetActorLocation() + Velocity * DeltaTime;
		SetActorLocation(NewLocation, true);

		// Spawn engine trail particles when moving fast enough
		const float SpeedSq = Velocity.SizeSquared();
		const float TrailSpeedThreshold = 50.0f;
		TrailSpawnTimer -= DeltaTime;
		if (SpeedSq > TrailSpeedThreshold * TrailSpeedThreshold && TrailSpawnTimer <= 0.0f)
		{
			// Spawn trail behind the ship (opposite of forward direction)
			const FVector TrailOffset = GetActorRotation().RotateVector(FVector(-60.0f, 0.0f, 0.0f));
			const FVector TrailLocation = GetActorLocation() + TrailOffset;

			FActorSpawnParameters TrailSpawnParams;
			TrailSpawnParams.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			GetWorld()->SpawnActor<AAsteroidSurvivorTrailParticle>(
				AAsteroidSurvivorTrailParticle::StaticClass(),
				TrailLocation, FRotator::ZeroRotator, TrailSpawnParams);

			TrailSpawnTimer = TrailSpawnInterval;
		}

		// Auto-fire while button is held
		if (bFiring)
		{
			const float EffectiveFireRate = BaseFireRate / FireRateMultiplier;
			FireTimer -= DeltaTime;
			if (FireTimer <= 0.0f)
			{
				Fire();
				FireTimer = EffectiveFireRate;
			}
		}

		// Tick down homing missile cooldown independently of main fire rate
		HomingMissileTimer -= DeltaTime;

		// Passive healing
		if (PassiveHealRate > 0.0f && CurrentHealth < MaxHealth && CurrentHealth > 0.0f)
		{
			CurrentHealth = FMath::Min(CurrentHealth + PassiveHealRate * DeltaTime, MaxHealth);
		}

		// Shield recharge
		if (bHasShieldUpgrade && !bShieldActive)
		{
			ShieldRechargeTimer -= DeltaTime;
			if (ShieldRechargeTimer <= 0.0f)
			{
				bShieldActive = true;
				UpdateShieldVisual();
			}
		}
	}

	// Invulnerability countdown and blinking (always tick, even during upgrade select)
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
		if (SelectUpgrade1Action)
		{
			EIC->BindAction(SelectUpgrade1Action, ETriggerEvent::Started, this, &AAsteroidSurvivorShip::OnSelectUpgrade1);
		}
		if (SelectUpgrade2Action)
		{
			EIC->BindAction(SelectUpgrade2Action, ETriggerEvent::Started, this, &AAsteroidSurvivorShip::OnSelectUpgrade2);
		}
		if (SelectUpgrade3Action)
		{
			EIC->BindAction(SelectUpgrade3Action, ETriggerEvent::Started, this, &AAsteroidSurvivorShip::OnSelectUpgrade3);
		}
	}
}

void AAsteroidSurvivorShip::Move(const FInputActionValue& Value)
{
	AAsteroidSurvivorGameMode* GM = Cast<AAsteroidSurvivorGameMode>(
		UGameplayStatics::GetGameMode(this));
	if (GM && GM->IsSelectingUpgrade())
	{
		return;
	}

	float ThrustInput = Value.Get<float>();
	FVector ForwardDir = GetActorForwardVector();
	const float EffectiveThrust = BaseThrustForce * SpeedMultiplier;
	Velocity += ForwardDir * ThrustInput * EffectiveThrust * GetWorld()->GetDeltaSeconds();
}

void AAsteroidSurvivorShip::Rotate(const FInputActionValue& Value)
{
	AAsteroidSurvivorGameMode* GM = Cast<AAsteroidSurvivorGameMode>(
		UGameplayStatics::GetGameMode(this));
	if (GM && GM->IsSelectingUpgrade())
	{
		return;
	}

	float RotInput = Value.Get<float>();
	FRotator NewRotation = GetActorRotation();
	const float EffectiveRotSpeed = BaseRotationSpeed * TurnRateMultiplier;
	NewRotation.Yaw += RotInput * EffectiveRotSpeed * GetWorld()->GetDeltaSeconds();
	SetActorRotation(NewRotation);
}

void AAsteroidSurvivorShip::StartFire()
{
	AAsteroidSurvivorGameMode* GM = Cast<AAsteroidSurvivorGameMode>(
		UGameplayStatics::GetGameMode(this));
	if (GM && GM->IsSelectingUpgrade())
	{
		return;
	}

	// Toggle firing on/off
	bFiring = !bFiring;
	if (bFiring)
	{
		FireTimer = 0.0f; // fire immediately on toggle-on
	}
}

void AAsteroidSurvivorShip::StopFire()
{
	// No-op: firing is now toggled by StartFire
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

	// Fire any extra weapons from the arsenal
	FireExtraWeapons();
}

void AAsteroidSurvivorShip::OnSelectUpgrade1()
{
	AAsteroidSurvivorGameMode* GM = Cast<AAsteroidSurvivorGameMode>(
		UGameplayStatics::GetGameMode(this));
	if (GM && GM->IsSelectingStatUpgrade())
	{
		GM->SelectUpgrade(0);
	}
	else if (GM && GM->IsSelectingWeaponUpgrade())
	{
		GM->SelectWeaponUpgrade(0);
	}
}

void AAsteroidSurvivorShip::OnSelectUpgrade2()
{
	AAsteroidSurvivorGameMode* GM = Cast<AAsteroidSurvivorGameMode>(
		UGameplayStatics::GetGameMode(this));
	if (GM && GM->IsSelectingStatUpgrade())
	{
		GM->SelectUpgrade(1);
	}
	else if (GM && GM->IsSelectingWeaponUpgrade())
	{
		GM->SelectWeaponUpgrade(1);
	}
}

void AAsteroidSurvivorShip::OnSelectUpgrade3()
{
	AAsteroidSurvivorGameMode* GM = Cast<AAsteroidSurvivorGameMode>(
		UGameplayStatics::GetGameMode(this));
	if (GM && GM->IsSelectingStatUpgrade())
	{
		GM->SelectUpgrade(2);
	}
	else if (GM && GM->IsSelectingWeaponUpgrade())
	{
		GM->SelectWeaponUpgrade(2);
	}
}

// ────────────────────────────────────────────────────────────────────────────
// Health & damage
// ────────────────────────────────────────────────────────────────────────────

bool AAsteroidSurvivorShip::ApplyDamageToShip(float DamageAmount)
{
	if (bInvulnerable)
	{
		return false;
	}

	// Shield absorbs the hit entirely
	if (bShieldActive)
	{
		bShieldActive = false;
		ShieldRechargeTimer = ShieldRechargeTime;
		UpdateShieldVisual();
		StartInvulnerability();
		return false;
	}

	CurrentHealth -= DamageAmount;
	if (CurrentHealth <= 0.0f)
	{
		CurrentHealth = 0.0f;
		return true; // Ship destroyed
	}

	StartInvulnerability();
	return false;
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

	if (!OtherActor || OtherActor == this)
	{
		return;
	}

	float Damage = 0.0f;
	bool bDestroyOther = false;

	// ── Asteroid collision ──────────────────────────────────────────────────
	AAsteroidSurvivorAsteroid* Asteroid = Cast<AAsteroidSurvivorAsteroid>(OtherActor);
	if (Asteroid)
	{
		Damage = Asteroid->GetDamageAmount();
		bDestroyOther = true;
	}

	// ── Enemy ship collision (contact damage) ───────────────────────────────
	AEnemyShipBase* EnemyShip = Cast<AEnemyShipBase>(OtherActor);
	if (EnemyShip)
	{
		Damage = EnemyShip->GetContactDamage();
		// Enemy ships are not destroyed on contact – they persist and the
		// player bounces off with invulnerability frames.
		bDestroyOther = false;
	}

	// ── Enemy projectile hit ────────────────────────────────────────────────
	AEnemyProjectile* EnemyProj = Cast<AEnemyProjectile>(OtherActor);
	if (EnemyProj)
	{
		Damage = EnemyProj->GetDamage();
		bDestroyOther = true;
	}

	// If no recognised damage source, ignore
	if (Damage <= 0.0f)
	{
		return;
	}

	if (bDestroyOther)
	{
		OtherActor->Destroy();
	}

	// Apply damage to ship
	const bool bDestroyed = ApplyDamageToShip(Damage);

	AAsteroidSurvivorGameMode* GM = Cast<AAsteroidSurvivorGameMode>(
	    UGameplayStatics::GetGameMode(this));
	if (GM && bDestroyed)
	{
		GM->OnPlayerShipDestroyed();
		Destroy();
	}
}

void AAsteroidSurvivorShip::StartInvulnerability()
{
	bInvulnerable = true;
	InvulnerabilityTimer = InvulnerabilityDuration;
	BlinkTimer = BlinkInterval;
	bBlinkVisible = true;
}

void AAsteroidSurvivorShip::UpdateShieldVisual()
{
	if (ShieldMesh)
	{
		ShieldMesh->SetVisibility(bShieldActive);
	}
}

// ────────────────────────────────────────────────────────────────────────────
// Upgrades
// ────────────────────────────────────────────────────────────────────────────

void AAsteroidSurvivorShip::UpgradeMaxHealth(float BonusHealth)
{
	MaxHealth += BonusHealth;
	CurrentHealth = MaxHealth; // full heal on upgrade
}

void AAsteroidSurvivorShip::UpgradeSpeed(float Multiplier)
{
	SpeedMultiplier *= Multiplier;
}

void AAsteroidSurvivorShip::UpgradeTurnRate(float Multiplier)
{
	TurnRateMultiplier *= Multiplier;
}

void AAsteroidSurvivorShip::UpgradePassiveHealing(float HealPerSecond)
{
	PassiveHealRate += HealPerSecond;
}

void AAsteroidSurvivorShip::UpgradeShield()
{
	bHasShieldUpgrade = true;
	bShieldActive = true;
	ShieldRechargeTimer = 0.0f;
	UpdateShieldVisual();
}

void AAsteroidSurvivorShip::UpgradeFireRate(float Multiplier)
{
	FireRateMultiplier *= Multiplier;
}

void AAsteroidSurvivorShip::UpgradeThoriumMagnet(float Multiplier)
{
	ThoriumMagnetMultiplier *= Multiplier;
}

// ────────────────────────────────────────────────────────────────────────────
// Weapon arsenal
// ────────────────────────────────────────────────────────────────────────────

void AAsteroidSurvivorShip::AddOrUpgradeWeapon(EWeaponType Type)
{
	// BlasterUpgrade directly enhances the base blaster level
	if (Type == EWeaponType::BlasterUpgrade)
	{
		BlasterLevel++;
		return;
	}

	if (int32* Level = WeaponArsenal.Find(Type))
	{
		// Stack the upgrade – no cap, all weapons are stackable
		*Level += 1;
	}
	else
	{
		// New weapon acquired at level 1
		WeaponArsenal.Add(Type, 1);
	}

	// Rapid Blaster stacks on top of existing fire rate multiplier
	if (Type == EWeaponType::RapidBlaster)
	{
		FireRateMultiplier *= 1.30f; // +30% fire rate per level
	}
}

void AAsteroidSurvivorShip::OnScrapCollected(int32 TotalScrap)
{
	// Every 5 scrap collected (doubling threshold), enhance the base blaster
	const int32 Threshold = 5;
	const int32 NewLevel = 1 + TotalScrap / Threshold;
	if (NewLevel > BlasterLevel)
	{
		BlasterLevel = NewLevel;
		// Each blaster level doesn't change the fire method directly;
		// FireExtraWeapons checks the level for additional projectiles.
	}
}

void AAsteroidSurvivorShip::FireExtraWeapons()
{
	if (!ProjectileClass)
	{
		return;
	}

	// Enhanced blaster: at level 2+, fire additional projectiles with slight angle offset
	if (BlasterLevel >= 2)
	{
		const float BlasterSpreadAngle = 8.0f; // Degrees between extra blaster shots
		const int32 ExtraShots = BlasterLevel - 1; // scales with level, no cap
		for (int32 i = 0; i < ExtraShots; ++i)
		{
			float AngleOffset = (i + 1) * BlasterSpreadAngle * ((i % 2 == 0) ? 1.0f : -1.0f);
			FRotator ShotRot = GetActorRotation();
			ShotRot.Yaw += AngleOffset;
			FVector ShotLoc = GetActorLocation() + ShotRot.RotateVector(MuzzleOffset);

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = this;
			SpawnParams.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			GetWorld()->SpawnActor<AAsteroidSurvivorProjectile>(
				ProjectileClass, ShotLoc, ShotRot, SpawnParams);
		}
	}

	// Fire weapons from the arsenal
	for (const auto& WeaponEntry : WeaponArsenal)
	{
		switch (WeaponEntry.Key)
		{
		case EWeaponType::SpreadShot:
			FireSpreadShot(WeaponEntry.Value);
			break;
		case EWeaponType::RearTurret:
			FireRearTurret(WeaponEntry.Value);
			break;
		case EWeaponType::HomingMissile:
			if (HomingMissileTimer <= 0.0f)
			{
				FireHomingMissile(WeaponEntry.Value);
				HomingMissileTimer = HomingMissileInterval;
			}
			break;
		case EWeaponType::RapidBlaster:
			// Handled passively via FireRateMultiplier in AddOrUpgradeWeapon()
			break;
		}
	}
}

void AAsteroidSurvivorShip::FireSpreadShot(int32 Level)
{
	// Spread shot fires additional projectiles in a fan pattern
	// Level 1: 2 extra (±15°), Level 2: 4 extra (±10°, ±20°), etc. – scales with level
	const int32 PairsCount = Level;
	const float SpreadBaseAngle = 15.0f;       // Starting angle at level 1
	const float SpreadLevelDecrement = 2.5f;   // Tightens spread per level
	const float AngleStep = FMath::Max(SpreadBaseAngle - (Level - 1) * SpreadLevelDecrement, 3.0f);

	for (int32 i = 1; i <= PairsCount; ++i)
	{
		float Angle = AngleStep * i;
		for (float Sign : {-1.0f, 1.0f})
		{
			FRotator ShotRot = GetActorRotation();
			ShotRot.Yaw += Angle * Sign;
			FVector ShotLoc = GetActorLocation() + ShotRot.RotateVector(MuzzleOffset);

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = this;
			SpawnParams.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			GetWorld()->SpawnActor<AAsteroidSurvivorProjectile>(
				ProjectileClass, ShotLoc, ShotRot, SpawnParams);
		}
	}
}

void AAsteroidSurvivorShip::FireRearTurret(int32 Level)
{
	// Rear turret fires projectile(s) backward – scales with level, no cap
	TArray<float> Angles;
	if (Level <= 1)
	{
		Angles = {0.0f};
	}
	else if (Level == 2)
	{
		Angles = {-10.0f, 10.0f};
	}
	else
	{
		// Level 3+: centre shot + symmetric pairs at ±15° spacing
		Angles.Add(0.0f);
		const int32 Pairs = (Level - 1) / 2 + 1; // 1 pair at L3, 2 at L5, etc.
		for (int32 i = 1; i <= Pairs; ++i)
		{
			Angles.Add(-15.0f * i);
			Angles.Add(15.0f * i);
		}
	}

	for (float Angle : Angles)
	{
		FRotator ShotRot = GetActorRotation();
		ShotRot.Yaw += 180.0f + Angle; // Backward
		FVector RearOffset(-120.0f, 0.0f, 0.0f);
		FVector ShotLoc = GetActorLocation() + GetActorRotation().RotateVector(RearOffset);

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;
		SpawnParams.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		GetWorld()->SpawnActor<AAsteroidSurvivorProjectile>(
			ProjectileClass, ShotLoc, ShotRot, SpawnParams);
	}
}

void AAsteroidSurvivorShip::FireHomingMissile(int32 Level)
{
	// Homing missile: fires toward the nearest enemy within a search radius.
	// Level determines the number of missiles (scales with level, no cap).
	// Note: actual homing behaviour is simulated by aiming at the target's position.
	// A true homing projectile would need a custom subclass; here we aim precisely.

	const int32 MissileCount = Level; // scales with level, no cap
	const float SearchRadius = 2000.0f;

	// Find nearest enemies
	TArray<AActor*> NearbyEnemies;
	for (TActorIterator<AEnemyShipBase> It(GetWorld()); It; ++It)
	{
		if (FVector::Dist(GetActorLocation(), (*It)->GetActorLocation()) <= SearchRadius)
		{
			NearbyEnemies.Add(*It);
		}
	}
	// Also consider asteroids as valid targets
	for (TActorIterator<AAsteroidSurvivorAsteroid> It(GetWorld()); It; ++It)
	{
		if (FVector::Dist(GetActorLocation(), (*It)->GetActorLocation()) <= SearchRadius)
		{
			NearbyEnemies.Add(*It);
		}
	}

	// Sort by distance (cache ship location to avoid redundant calls)
	const FVector ShipLoc = GetActorLocation();
	NearbyEnemies.Sort([ShipLoc](const AActor& A, const AActor& B)
	{
		return FVector::DistSquared(ShipLoc, A.GetActorLocation()) <
		       FVector::DistSquared(ShipLoc, B.GetActorLocation());
	});

	for (int32 i = 0; i < MissileCount && i < NearbyEnemies.Num(); ++i)
	{
		const FVector ToTarget = (NearbyEnemies[i]->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		FRotator ShotRot = ToTarget.Rotation();
		FVector ShotLoc = GetActorLocation() + ToTarget * 100.0f;

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;
		SpawnParams.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AAsteroidSurvivorProjectile* Missile = GetWorld()->SpawnActor<AAsteroidSurvivorProjectile>(
			ProjectileClass, ShotLoc, ShotRot, SpawnParams);
		if (Missile)
		{
			Missile->SetHomingMissile(true);
		}
	}

	// If no enemies nearby, fire forward
	if (NearbyEnemies.Num() == 0)
	{
		for (int32 i = 0; i < MissileCount; ++i)
		{
			FVector ShotLoc = GetActorLocation() + GetActorRotation().RotateVector(MuzzleOffset);
			FRotator ShotRot = GetActorRotation();

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = this;
			SpawnParams.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			AAsteroidSurvivorProjectile* Missile = GetWorld()->SpawnActor<AAsteroidSurvivorProjectile>(
				ProjectileClass, ShotLoc, ShotRot, SpawnParams);
			if (Missile)
			{
				Missile->SetHomingMissile(true);
			}
		}
	}
}

// ────────────────────────────────────────────────────────────────────────────
// Input setup
// ────────────────────────────────────────────────────────────────────────────

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

	// Upgrade selection actions (Boolean: pressed)
	SelectUpgrade1Action = NewObject<UInputAction>(this, TEXT("IA_SelectUpgrade1"));
	SelectUpgrade1Action->ValueType = EInputActionValueType::Boolean;

	SelectUpgrade2Action = NewObject<UInputAction>(this, TEXT("IA_SelectUpgrade2"));
	SelectUpgrade2Action->ValueType = EInputActionValueType::Boolean;

	SelectUpgrade3Action = NewObject<UInputAction>(this, TEXT("IA_SelectUpgrade3"));
	SelectUpgrade3Action->ValueType = EInputActionValueType::Boolean;

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

	// -- Upgrade selection (1, 2, 3 keys) --
	DefaultMappingContext->MapKey(SelectUpgrade1Action, EKeys::One);
	DefaultMappingContext->MapKey(SelectUpgrade2Action, EKeys::Two);
	DefaultMappingContext->MapKey(SelectUpgrade3Action, EKeys::Three);
}
