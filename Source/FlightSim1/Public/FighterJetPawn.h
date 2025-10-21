// Copyright Your Company Name, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Blueprint/UserWidget.h"
#include "Particles/ParticleSystem.h"
#include "HealthComponent.h"
#include "Missile.h"
#include "FighterJetPawn.generated.h"

class USoundBase;

UCLASS()
class FLIGHTSIM1_API AFighterJetPawn : public APawn
{
	GENERATED_BODY()

public:
	AFighterJetPawn();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// --- Components ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* AircraftMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* MuzzleLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UHealthComponent* HealthComponent;

	// --- Flight Physics Properties ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Thrust")
	float MaxThrust;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Thrust")
	float ThrustAcceleration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Maneuvering")
	float PitchSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Maneuvering")
	float RollSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Maneuvering")
	float YawSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Maneuvering")
	float GroundSteerSpeed;

	// --- Advanced Aerodynamics ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Aerodynamics")
	float LiftCoefficient;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Aerodynamics")
	float DragCoefficient;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Aerodynamics")
	float InducedDragCoefficient;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Aerodynamics")
	float CriticalAngleOfAttack;

	// --- Factor to control how much roll induces a turn ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Maneuvering")
	float RollYawFactor;

	// --- Flaps and Ground Effect Parameters ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Aerodynamics|Flaps")
	float FlapsLiftMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Aerodynamics|Flaps")
	float FlapsDragMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Aerodynamics|GroundEffect")
	float GroundEffectAltitude;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Aerodynamics|GroundEffect")
	float GroundEffectLiftMultiplier;

	// --- HUD Variables ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD")
	float Airspeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD")
	float Altitude;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD")
	AActor* LockedTarget;

	// --- Weapon Properties ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
	float WeaponRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
	float FireRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
	int32 MaxMissileAmmo;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapons")
	int32 CurrentMissileAmmo;

	UPROPERTY(EditDefaultsOnly, Category = "Weapons")
	UParticleSystem* MuzzleFlashFX;

	UPROPERTY(EditDefaultsOnly, Category = "Weapons")
	USoundBase* FireSound;

	UPROPERTY(EditDefaultsOnly, Category = "Weapons")
	TSubclassOf<AMissile> MissileClass;

private:
	// --- Flight Control Inputs ---
	float PitchInput;
	float RollInput;
	float YawInput;
	float GroundSteerInput;
	float CurrentThrottle;

	// --- Internal State ---
	bool bIsOnGround;
	bool bIsFiring;
	bool bIsIncreasingThrottle;
	bool bIsDecreasingThrottle;
	bool bFlapsDeployed;

	FTimerHandle FireRateTimerHandle;

	// --- HUD ---
	UPROPERTY()
	TSubclassOf<UUserWidget> HUDWidgetClass;
	UPROPERTY()
	UUserWidget* HUDWidgetInstance;

	// --- Input Functions ---
	void PressThrottle();
	void ReleaseThrottle();
	void PressThrottleDecrease();
	void ReleaseThrottleDecrease();
	void Pitch(float Value);
	void Roll(float Value);
	void Yaw(float Value);
	void GroundSteer(float Value);
	void StartFire();
	void StopFire();
	void FireWeapon();
	void FireMissile();
	void ToggleFlaps();

	// --- Logic Functions ---
	void OnPawnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	UFUNCTION()
	void HandlePawnDeath();
	void UpdateHUDVariables();
	void UpdateLockedTarget();
	void UpdateThrottle(float DeltaTime);
	void CheckIfOnGround();
	void ApplyAerodynamics(float DeltaTime);
};

