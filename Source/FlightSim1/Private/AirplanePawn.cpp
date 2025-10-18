// Fill out your copyright notice in the Description page of Project Settings.

#include "AirplanePawn.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
AAirplanePawn::AAirplanePawn()
{
	// Set this pawn to call Tick() every frame.
	PrimaryActorTick.bCanEverTick = true;

	// Create the Airframe Mesh component
	AirframeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AirframeMesh"));
	RootComponent = AirframeMesh;
	AirframeMesh->SetSimulatePhysics(true);

	// Create the Spring Arm (Camera Boom)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(AirframeMesh);
	CameraBoom->TargetArmLength = 800.0f;
	CameraBoom->bDoCollisionTest = false; // Don't want camera to zoom in

	// Create the Camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom);
	FollowCamera->SetRelativeRotation(FRotator(-10.0f, 0.0f, 0.0f));
}

// Called when the game starts or when spawned
void AAirplanePawn::BeginPlay()
{
	Super::BeginPlay();

	// Add the Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(IMC_FlightControls, 0);
		}
	}
}

// Called every frame
void AAirplanePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// --- Throttle Logic (Sticky Throttle) ---
	const double ThrottleChange = ThrottleInput * ThrottleRampSpeed * DeltaTime;
	TargetThrottle = FMath::Clamp(TargetThrottle + ThrottleChange, 0.0, 1.0);

	// --- Smooth Throttle ---
	CurrentThrottle = FMath::FInterpTo(CurrentThrottle, TargetThrottle, DeltaTime, ThrottleChangeSpeed);

	// --- Physics Forces ---
	if (AirframeMesh)
	{
		// 1. THRUST
		const FVector ThrustForce = AirframeMesh->GetForwardVector() * CurrentThrottle * EnginePower;
		AirframeMesh->AddForce(ThrustForce);

		// 2. LIFT
		const FVector Velocity = AirframeMesh->GetPhysicsLinearVelocity();
		const double SpeedSquared = Velocity.SizeSquared();
		const FVector LiftDirection = AirframeMesh->GetUpVector();
		const FVector LiftForce = LiftDirection * SpeedSquared * LiftCoefficient;
		AirframeMesh->AddForce(LiftForce);

		// 3. DRAG
		const FVector DragForce = -Velocity * Velocity.Size() * DragCoefficient;
		AirframeMesh->AddForce(DragForce);

		// 4. CONTROL TORQUES (Pitch, Roll, Yaw)
		// Pitch
		const FVector PitchTorque = AirframeMesh->GetRightVector() * PitchInput * ControlStrength;
		AirframeMesh->AddTorqueInRadians(PitchTorque);

		// Roll
		const FVector RollTorque = AirframeMesh->GetForwardVector() * RollInput * ControlStrength;
		AirframeMesh->AddTorqueInRadians(RollTorque);

		// Yaw
		const FVector YawTorque = AirframeMesh->GetUpVector() * YawInput * ControlStrength;
		AirframeMesh->AddTorqueInRadians(YawTorque);
	}
}

// Called to bind functionality to input
void AAirplanePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Bind the input actions to our handler functions
		EnhancedInputComponent->BindAction(IA_Throttle, ETriggerEvent::Triggered, this, &AAirplanePawn::HandleThrottle);
		EnhancedInputComponent->BindAction(IA_Throttle, ETriggerEvent::Completed, this, &AAirplanePawn::HandleThrottle);

		EnhancedInputComponent->BindAction(IA_Pitch, ETriggerEvent::Triggered, this, &AAirplanePawn::HandlePitch);
		EnhancedInputComponent->BindAction(IA_Pitch, ETriggerEvent::Completed, this, &AAirplanePawn::ResetPitchRollYaw);

		EnhancedInputComponent->BindAction(IA_Roll, ETriggerEvent::Triggered, this, &AAirplanePawn::HandleRoll);
		EnhancedInputComponent->BindAction(IA_Roll, ETriggerEvent::Completed, this, &AAirplanePawn::ResetPitchRollYaw);

		EnhancedInputComponent->BindAction(IA_Yaw, ETriggerEvent::Triggered, this, &AAirplanePawn::HandleYaw);
		EnhancedInputComponent->BindAction(IA_Yaw, ETriggerEvent::Completed, this, &AAirplanePawn::ResetPitchRollYaw);
	}
}


// --- Input Handler Functions ---
void AAirplanePawn::HandleThrottle(const FInputActionValue& Value)
{
	// For the sticky throttle, we just store the input value (-1, 0, or 1)
	ThrottleInput = Value.Get<float>();
}

void AAirplanePawn::HandlePitch(const FInputActionValue& Value)
{
	PitchInput = Value.Get<float>();
}

void AAirplanePawn::HandleRoll(const FInputActionValue& Value)
{
	RollInput = Value.Get<float>();
}

void AAirplanePawn::HandleYaw(const FInputActionValue& Value)
{
	YawInput = Value.Get<float>();
}

void AAirplanePawn::ResetPitchRollYaw(const FInputActionValue& Value)
{
	// When pitch/roll/yaw keys are released, reset their inputs to 0
	PitchInput = 0.0;
	RollInput = 0.0;
	YawInput = 0.0;
}
