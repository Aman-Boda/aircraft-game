// Copyright Your Company Name, Inc. All Rights Reserved.

#include "FighterJetPawn.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Sound/SoundBase.h"
#include "Particles/ParticleSystem.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "HealthComponent.h"
#include "AIAircraftPawn.h"
#include "Missile.h"

// Sets default values
AFighterJetPawn::AFighterJetPawn()
{
	// Set this pawn to call Tick() every frame.
	PrimaryActorTick.bCanEverTick = true;

	// --- Component Initialization ---
	AircraftMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AircraftMesh"));
	RootComponent = AircraftMesh;
	AircraftMesh->SetSimulatePhysics(true);
	AircraftMesh->SetEnableGravity(true);
	AircraftMesh->SetMassOverrideInKg(NAME_None, 15000.0f, true);
	AircraftMesh->SetAngularDamping(2.0f);
	AircraftMesh->SetLinearDamping(0.5f);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 17000.0f;
	SpringArm->SocketOffset = FVector(-700.0f, 0.0f, 700.0f);
	SpringArm->SetRelativeRotation(FRotator(-45.0f, 0.0f, 0.0f));
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 5.0f;
	SpringArm->bInheritPitch = true;
	SpringArm->bInheritYaw = true;
	SpringArm->bInheritRoll = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

	MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	MuzzleLocation->SetupAttachment(AircraftMesh);

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

	// --- Default Physics Values ---
	// --- CHANGE: Increased MaxThrust by 35% ---
	MaxThrust = 17550000.0f;
	ThrustAcceleration = 0.5f;
	PitchSpeed = 30.0f;
	// --- CHANGE: Significantly increased RollSpeed for responsive rolling ---
	RollSpeed = 80.0f;
	YawSpeed = 10.0f;
	GroundSteerSpeed = 80.0f;

	// --- ADVANCED PHYSICS DEFAULTS (Re-tuned for better feel) ---
	LiftCoefficient = 0.2f;
	DragCoefficient = 0.022f;
	InducedDragCoefficient = 0.06f;
	CriticalAngleOfAttack = 15.0f;

	// Coordinated turn default value
	RollYawFactor = 15.0f;

	// --- Flaps and Ground Effect Defaults ---
	FlapsLiftMultiplier = 1.75f;
	FlapsDragMultiplier = 2.0f;
	GroundEffectAltitude = 1500.0f; // 15 meters
	GroundEffectLiftMultiplier = 1.5f;

	// --- Weapon Properties ---
	WeaponRange = 50000.0f;
	FireRate = 0.1f;
	MaxMissileAmmo = 10;
	CurrentMissileAmmo = MaxMissileAmmo;

	// --- Initial State ---
	CurrentThrottle = 0.0f;
	PitchInput = 0.0f;
	RollInput = 0.0f;
	YawInput = 0.0f;
	GroundSteerInput = 0.0f;
	bIsOnGround = false;
	bIsFiring = false;
	bIsIncreasingThrottle = false;
	bIsDecreasingThrottle = false;
	bFlapsDeployed = false;
	LockedTarget = nullptr;

	// --- Find HUD Widget ---
	static ConstructorHelpers::FClassFinder<UUserWidget> HUDClassFinder(TEXT("/Game/Blueprints/WBP_FighterHUD"));
	if (HUDClassFinder.Succeeded())
	{
		HUDWidgetClass = HUDClassFinder.Class;
	}
}

// Called when the game starts or when spawned
void AFighterJetPawn::BeginPlay()
{
	Super::BeginPlay();

	// Add OnHit delegate
	AircraftMesh->OnComponentHit.AddDynamic(this, &AFighterJetPawn::OnPawnHit);

	// Add OnDeath delegate from Health Component
	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddDynamic(this, &AFighterJetPawn::HandlePawnDeath);
	}

	// Create and display HUD
	if (HUDWidgetClass)
	{
		HUDWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), HUDWidgetClass);
		if (HUDWidgetInstance)
		{
			HUDWidgetInstance->AddToViewport();
		}
	}
}

// Called every frame
void AFighterJetPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HealthComponent && HealthComponent->IsDead()) return;

	CheckIfOnGround();
	UpdateThrottle(DeltaTime);
	ApplyAerodynamics(DeltaTime);
	UpdateHUDVariables();
	UpdateLockedTarget();
}

// Called to bind functionality to input
void AFighterJetPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Throttle", IE_Pressed, this, &AFighterJetPawn::PressThrottle);
	PlayerInputComponent->BindAction("Throttle", IE_Released, this, &AFighterJetPawn::ReleaseThrottle);

	PlayerInputComponent->BindAction("ThrottleDecrease", IE_Pressed, this, &AFighterJetPawn::PressThrottleDecrease);
	PlayerInputComponent->BindAction("ThrottleDecrease", IE_Released, this, &AFighterJetPawn::ReleaseThrottleDecrease);

	PlayerInputComponent->BindAction("ToggleFlaps", IE_Pressed, this, &AFighterJetPawn::ToggleFlaps);

	PlayerInputComponent->BindAxis("Pitch", this, &AFighterJetPawn::Pitch);
	PlayerInputComponent->BindAxis("Roll", this, &AFighterJetPawn::Roll);
	PlayerInputComponent->BindAxis("Yaw", this, &AFighterJetPawn::Yaw);
	PlayerInputComponent->BindAxis("GroundSteer", this, &AFighterJetPawn::GroundSteer);

	PlayerInputComponent->BindAction("FireWeapon", IE_Pressed, this, &AFighterJetPawn::StartFire);
	PlayerInputComponent->BindAction("FireWeapon", IE_Released, this, &AFighterJetPawn::StopFire);

	PlayerInputComponent->BindAction("FireMissile", IE_Pressed, this, &AFighterJetPawn::FireMissile);
}

void AFighterJetPawn::OnPawnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (HealthComponent)
	{
		float ImpactSpeed = NormalImpulse.Size() / (AircraftMesh->GetMass());
		if (ImpactSpeed > 1000.0f)
		{
			HealthComponent->TakeDamage(50.0f);
		}
	}
}

void AFighterJetPawn::HandlePawnDeath()
{
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
}

void AFighterJetPawn::UpdateHUDVariables()
{
	Airspeed = AircraftMesh->GetPhysicsLinearVelocity().Size() * 0.036; // Convert cm/s to km/h
	Altitude = GetActorLocation().Z / 100.0f; // Convert cm to m
}

void AFighterJetPawn::UpdateLockedTarget()
{
	LockedTarget = nullptr;
	float BestTargetScore = -1.0f;
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAIAircraftPawn::StaticClass(), FoundActors);
	FVector MyLocation = GetActorLocation();
	FVector MyForward = GetActorForwardVector();

	for (AActor* PotentialTarget : FoundActors)
	{
		if (PotentialTarget)
		{
			FVector DirectionToTarget = (PotentialTarget->GetActorLocation() - MyLocation).GetSafeNormal();
			float DotProduct = FVector::DotProduct(MyForward, DirectionToTarget);

			if (DotProduct > 0.8f)
			{
				if (DotProduct > BestTargetScore)
				{
					BestTargetScore = DotProduct;
					LockedTarget = PotentialTarget;
				}
			}
		}
	}
}

void AFighterJetPawn::UpdateThrottle(float DeltaTime)
{
	if (bIsIncreasingThrottle)
	{
		CurrentThrottle = FMath::Clamp(CurrentThrottle + ThrustAcceleration * DeltaTime, 0.0f, 1.0f);
	}
	else if (bIsDecreasingThrottle)
	{
		CurrentThrottle = FMath::Clamp(CurrentThrottle - ThrustAcceleration * DeltaTime, 0.0f, 1.0f);
	}
}

void AFighterJetPawn::PressThrottle() { bIsIncreasingThrottle = true; }
void AFighterJetPawn::ReleaseThrottle() { bIsIncreasingThrottle = false; }
void AFighterJetPawn::PressThrottleDecrease() { bIsDecreasingThrottle = true; }
void AFighterJetPawn::ReleaseThrottleDecrease() { bIsDecreasingThrottle = false; }

void AFighterJetPawn::ToggleFlaps()
{
	bFlapsDeployed = !bFlapsDeployed;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, bFlapsDeployed ? TEXT("Flaps Deployed") : TEXT("Flaps Retracted"));
	}
}

void AFighterJetPawn::Pitch(float Value) { PitchInput = Value; }
void AFighterJetPawn::Roll(float Value) { RollInput = Value; }
void AFighterJetPawn::Yaw(float Value) { YawInput = Value; }
void AFighterJetPawn::GroundSteer(float Value) { GroundSteerInput = Value; }

void AFighterJetPawn::StartFire()
{
	bIsFiring = true;
	GetWorldTimerManager().SetTimer(FireRateTimerHandle, this, &AFighterJetPawn::FireWeapon, FireRate, true, 0.0f);
}

void AFighterJetPawn::StopFire()
{
	bIsFiring = false;
	GetWorldTimerManager().ClearTimer(FireRateTimerHandle);
}

void AFighterJetPawn::FireWeapon()
{
	FHitResult HitResult;
	FVector Start = MuzzleLocation->GetComponentLocation();
	FVector End = Start + (GetActorForwardVector() * WeaponRange);
	GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility);

	if (HitResult.GetActor())
	{
		UHealthComponent* HitHealthComponent = HitResult.GetActor()->FindComponentByClass<UHealthComponent>();
		if (HitHealthComponent)
		{
			HitHealthComponent->TakeDamage(10.f);
		}
	}

	if (MuzzleFlashFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlashFX, MuzzleLocation->GetComponentTransform());
	}
	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}
}

void AFighterJetPawn::FireMissile()
{
	if (CurrentMissileAmmo > 0)
	{
		if (MissileClass && LockedTarget)
		{
			FVector SpawnLocation = MuzzleLocation->GetComponentLocation();
			FRotator SpawnRotation = GetActorRotation();
			AMissile* NewMissile = GetWorld()->SpawnActor<AMissile>(MissileClass, SpawnLocation, SpawnRotation);

			if (NewMissile)
			{
				NewMissile->SetTarget(LockedTarget);
			}

			CurrentMissileAmmo--;
		}
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("OUT OF MISSILES"));
		}
	}
}

void AFighterJetPawn::CheckIfOnGround()
{
	if (!AircraftMesh) return;
	FVector Start = AircraftMesh->GetComponentLocation();
	FVector End = Start - (GetActorUpVector() * 300.0f);
	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);
	bIsOnGround = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, CollisionParams);
}

void AFighterJetPawn::ApplyAerodynamics(float DeltaTime)
{
	if (!AircraftMesh) return;

	FVector ForwardVector = AircraftMesh->GetForwardVector();
	FVector UpVector = AircraftMesh->GetUpVector();
	FVector RightVector = AircraftMesh->GetRightVector();

	FVector ThrustForce = ForwardVector * CurrentThrottle * MaxThrust;
	AircraftMesh->AddForce(ThrustForce);

	if (bIsOnGround)
	{
		AircraftMesh->AddTorqueInDegrees(UpVector * GroundSteerInput * GroundSteerSpeed, NAME_None, true);
	}
	else
	{
		FVector Velocity = AircraftMesh->GetPhysicsLinearVelocity();
		float LocalAirspeed = Velocity.Size();
		if (LocalAirspeed < 1.0f) return;

		FVector VelocityNormal = Velocity.GetSafeNormal();

		float AngleOfAttackDegrees = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(VelocityNormal, ForwardVector)));

		float CurrentLiftCoefficient = 0;
		if (AngleOfAttackDegrees < CriticalAngleOfAttack)
		{
			CurrentLiftCoefficient = (AngleOfAttackDegrees / CriticalAngleOfAttack) * LiftCoefficient;
		}
		else
		{
			CurrentLiftCoefficient = 0.01f;
		}

		float FinalLiftCoefficient = CurrentLiftCoefficient;
		float FinalDragCoefficient = DragCoefficient;

		if (bFlapsDeployed)
		{
			FinalLiftCoefficient *= FlapsLiftMultiplier;
			FinalDragCoefficient *= FlapsDragMultiplier;
		}
		if (Altitude < GroundEffectAltitude)
		{
			FinalLiftCoefficient *= GroundEffectLiftMultiplier;
		}

		FVector LiftDirection = FVector::CrossProduct(VelocityNormal, RightVector).GetSafeNormal();
		FVector LiftForce = LiftDirection * LocalAirspeed * LocalAirspeed * FinalLiftCoefficient;
		AircraftMesh->AddForce(LiftForce);

		FVector ParasiticDragForce = -VelocityNormal * LocalAirspeed * LocalAirspeed * FinalDragCoefficient;
		FVector InducedDragForce = -VelocityNormal * LocalAirspeed * LocalAirspeed * (CurrentLiftCoefficient * CurrentLiftCoefficient * InducedDragCoefficient);
		AircraftMesh->AddForce(ParasiticDragForce + InducedDragForce);

		float ControlEffectiveness = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 15000.f), FVector2D(0.1f, 1.f), LocalAirspeed);

		AircraftMesh->AddTorqueInDegrees(RightVector * PitchInput * PitchSpeed * ControlEffectiveness, NAME_None, true);

		// Apply the standard roll torque
		AircraftMesh->AddTorqueInDegrees(ForwardVector * RollInput * RollSpeed * ControlEffectiveness, NAME_None, true);
		// Apply a small amount of yaw in the same direction as the roll to make the turn feel natural
		float CoordinatedYaw = RollInput * RollYawFactor * YawSpeed * ControlEffectiveness;
		AircraftMesh->AddTorqueInDegrees(UpVector * CoordinatedYaw, NAME_None, true);

		// Apply the standard yaw torque from player input
		AircraftMesh->AddTorqueInDegrees(UpVector * YawInput * YawSpeed * ControlEffectiveness, NAME_None, true);
	}
}

