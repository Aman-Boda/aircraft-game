// Copyright Your Company Name, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AIAircraftPawn.generated.h"

class UHealthComponent;
class USceneComponent;
class UParticleSystem;
class USoundBase;

UENUM(BlueprintType)
enum class EAIState : uint8
{
	Seeking,
	Circling,
	Evading
};

UCLASS()
class FLIGHTSIM1_API AAIAircraftPawn : public APawn
{
	GENERATED_BODY()

public:
	AAIAircraftPawn();

protected:
	virtual void BeginPlay() override;

	// --- Components ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* AircraftMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UHealthComponent* HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* MuzzleLocation;

	// --- AI Properties ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float FlightSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float TurnSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float MaxSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float AvoidanceDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float CirclingOffsetDistance;

	// --- Weapon Properties ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
	float FireRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
	float WeaponRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
	float FireAngleThreshold;

	UPROPERTY(EditDefaultsOnly, Category = "Weapons")
	UParticleSystem* MuzzleFlashFX;

	UPROPERTY(EditDefaultsOnly, Category = "Weapons")
	USoundBase* FireSound;

	// --- AI State Machine ---
	EAIState CurrentAIState;
	FTimerHandle EvasionTimerHandle;
	FTimerHandle FireRateTimerHandle;

	void MoveAndTurn(float DeltaTime);
	void TryFireWeapon();
	void FireWeapon();

	// --- Evasion Logic ---
	UFUNCTION()
	void HandleTakeDamage(AActor* DamagedActor, float NewHealth);
	void BeginEvasion();
	void EndEvasion();
	void PerformEvasion(float DeltaTime);

public:
	virtual void Tick(float DeltaTime) override;
};

