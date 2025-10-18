// Copyright Your Company Name, Inc. All Rights Reserved.

#include "Missile.h"
#include "Components/StaticMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "HealthComponent.h"
#include "Particles/ParticleSystem.h"

// Sets default values
AMissile::AMissile()
{
	// Set this actor to call Tick() every frame.
	PrimaryActorTick.bCanEverTick = true;

	MissileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MissileMesh"));
	RootComponent = MissileMesh;
	MissileMesh->SetCollisionProfileName(TEXT("BlockAll"));

	TrailEffect = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("TrailEffect"));
	TrailEffect->SetupAttachment(MissileMesh);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 8000.0f;
	ProjectileMovement->MaxSpeed = 20000.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bIsHomingProjectile = true;
	ProjectileMovement->HomingAccelerationMagnitude = 15000.0f;

	Damage = 100.0f;
	TargetActor = nullptr;
}

// Called when the game starts or when spawned
void AMissile::BeginPlay()
{
	Super::BeginPlay();
	MissileMesh->OnComponentHit.AddDynamic(this, &AMissile::OnMissileHit);
}

void AMissile::OnMissileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor && OtherActor != this)
	{
		UHealthComponent* HealthComponent = OtherActor->FindComponentByClass<UHealthComponent>();
		if (HealthComponent)
		{
			HealthComponent->TakeDamage(Damage);
		}
	}

	if (ExplosionEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation(), GetActorRotation());
	}

	Destroy();
}


// Called every frame
void AMissile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (TargetActor && ProjectileMovement)
	{
		ProjectileMovement->HomingTargetComponent = TargetActor->GetRootComponent();
	}
}

void AMissile::SetTarget(AActor* NewTarget)
{
	TargetActor = NewTarget;
}

