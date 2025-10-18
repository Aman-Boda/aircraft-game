// Copyright Your Company Name, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "FighterJetPawn.generated.h"

// Forward declarations for component classes
class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class USceneComponent;
class UHealthComponent;
class UParticleSystem;
class USoundBase;
class UUserWidget;
class AMissile;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Physics")
	float LiftCoefficient;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Physics")
	float DragCoefficient;

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

	UPROPERTY(EditDefaultsOnly, Category = "Weapons")
	UParticleSystem* MuzzleFlashFX;

	UPROPERTY(EditDefaultsOnly, Category = "Weapons")
	USoundBase* FireSound;

	UPROPERTY(EditDefaultsOnly, Category = "Weapons")
	TSubclassOf<AMissile> MissileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapons")
	int32 MaxMissileAmmo;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapons")
	int32 CurrentMissileAmmo;

protected:
	// --- Input Handling ---
	void Throttle(float Value);
	void Pitch(float Value);
	void Roll(float Value);
	void Yaw(float Value);
	void GroundSteer(float Value);

	void StartFire();
	void StopFire();
	void FireWeapon();

	void FireMissile();

	// --- Internal Logic ---
	void ApplyAerodynamics(float DeltaTime);
	void CheckIfOnGround();
	void UpdateHUDVariables();
	void UpdateLockedTarget();

	UFUNCTION()
	void OnPawnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void HandlePawnDeath();

	bool bIsFiring;
	bool bIsOnGround;
	float CurrentThrottle;
	float PitchInput, RollInput, YawInput, GroundSteerInput;

	FTimerHandle FireRateTimerHandle;

	// --- UI ---
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> HUDWidgetClass;

	UPROPERTY()
	UUserWidget* HUDWidgetInstance;
};

