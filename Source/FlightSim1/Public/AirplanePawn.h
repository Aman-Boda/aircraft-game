// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AirplanePawn.generated.h"

// Forward declare classes to improve compile times
class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

UCLASS()
class FLIGHTSIM1_API AAirplanePawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AAirplanePawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


	// --- COMPONENTS ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> AirframeMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> FollowCamera;


	// --- INPUT ---
	// Here we will link the Input Assets you created in the editor
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> IMC_FlightControls;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Throttle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Pitch;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Roll;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Yaw;


	// --- VARIABLES ---
	// These are the same variables from your Blueprint, exposed to the editor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Tuning")
	double EnginePower = 500000.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Tuning")
	double LiftCoefficient = 0.005;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Tuning")
	double DragCoefficient = 0.002;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Tuning")
	double ControlStrength = 950000000.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throttle")
	double ThrottleChangeSpeed = 0.2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throttle")
	double ThrottleRampSpeed = 0.5;

private:
	// Internal variables that the player doesn't need to change
	double TargetThrottle = 0.0;
	double CurrentThrottle = 0.0;
	double PitchInput = 0.0;
	double RollInput = 0.0;
	double YawInput = 0.0;
	double ThrottleInput = 0.0;

	// Functions to handle the input events
	void HandleThrottle(const FInputActionValue& Value);
	void HandlePitch(const FInputActionValue& Value);
	void HandleRoll(const FInputActionValue& Value);
	void HandleYaw(const FInputActionValue& Value);
	void ResetPitchRollYaw(const FInputActionValue& Value);
};
