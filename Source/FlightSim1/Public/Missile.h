// Copyright Your Company Name, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Missile.generated.h"

class UStaticMeshComponent;
class UParticleSystemComponent;
class UProjectileMovementComponent;
class UParticleSystem;

UCLASS()
class FLIGHTSIM1_API AMissile : public AActor
{
	GENERATED_BODY()

public:
	AMissile();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MissileMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UParticleSystemComponent* TrailEffect;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float Damage;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	UParticleSystem* ExplosionEffect;

	UFUNCTION()
	void OnMissileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY()
	AActor* TargetActor;

public:
	virtual void Tick(float DeltaTime) override;

	void SetTarget(AActor* NewTarget);
};

