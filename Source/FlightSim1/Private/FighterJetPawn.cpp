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
	AircraftMesh->SetMassOverrideInKg(NAME_None, 15000.0f, true);
	AircraftMesh->SetAngularDamping(0.5f);
	AircraftMesh->SetLinearDamping(0.1f);

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
	MaxThrust = 100000000.0f;
	ThrustAcceleration = 0.5f;
	PitchSpeed = 30.0f;
	RollSpeed = 50.0f;
	YawSpeed = 10.0f;
	GroundSteerSpeed = 80.0f;
	LiftCoefficient = 0.1f;
	DragCoefficient = 0.005f;

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
	ApplyAerodynamics(DeltaTime);
	UpdateHUDVariables();
	UpdateLockedTarget();
}

// Called to bind functionality to input
void AFighterJetPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("Throttle", this, &AFighterJetPawn::Throttle);
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
		// Apply damage on hard landings/crashes
		float ImpactSpeed = NormalImpulse.Size() / (AircraftMesh->GetMass());
		if (ImpactSpeed > 1000.0f) // Threshold for damage
		{
			HealthComponent->TakeDamage(50.0f);
		}
	}
}

void AFighterJetPawn::HandlePawnDeath()
{
	// Hide and disable the pawn
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

			// Check if target is in front of us
			if (DotProduct > 0.8f) // Within a cone in front of the player
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

void AFighterJetPawn::Throttle(float Value)
{
	CurrentThrottle = FMath::Clamp(CurrentThrottle + Value * ThrustAcceleration * GetWorld()->GetDeltaSeconds(), 0.0f, 1.0f);
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
	FVector End = Start - FVector(0.0f, 0.0f, 300.0f);
	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);

	bIsOnGround = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, CollisionParams);
}

void AFighterJetPawn::ApplyAerodynamics(float DeltaTime)
{
	if (!AircraftMesh) return;

	FVector Velocity = AircraftMesh->GetPhysicsLinearVelocity();
	float AirspeedSq = Velocity.SizeSquared();

	// Thrust
	FVector ThrustForce = AircraftMesh->GetForwardVector() * CurrentThrottle * MaxThrust;
	AircraftMesh->AddForce(ThrustForce);

	// Drag
	FVector DragForce = -Velocity.GetSafeNormal() * AirspeedSq * DragCoefficient;
	AircraftMesh->AddForce(DragForce);

	// Lift
	if (!bIsOnGround)
	{
		FVector LiftDirection = FVector::CrossProduct(Velocity.GetSafeNormal(), AircraftMesh->GetRightVector()).GetSafeNormal();
		FVector LiftForce = LiftDirection * AirspeedSq * LiftCoefficient;
		AircraftMesh->AddForce(LiftForce);
	}

	// Control Torques
	FVector RightVector = AircraftMesh->GetRightVector();
	FVector UpVector = AircraftMesh->GetUpVector();
	FVector ForwardVector = AircraftMesh->GetForwardVector();

	AircraftMesh->AddTorqueInDegrees(RightVector * PitchInput * PitchSpeed, NAME_None, true);
	AircraftMesh->AddTorqueInDegrees(ForwardVector * RollInput * RollSpeed, NAME_None, true);

	if (bIsOnGround)
	{
		AircraftMesh->AddTorqueInDegrees(UpVector * GroundSteerInput * GroundSteerSpeed, NAME_None, true);
	}
	else
	{
		AircraftMesh->AddTorqueInDegrees(UpVector * YawInput * YawSpeed, NAME_None, true);
	}
}

