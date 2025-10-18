// Copyright Your Company Name, Inc. All Rights Reserved.

#include "AIAircraftPawn.h"
#include "HealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Kismet/KismetMathLibrary.h"
#include "Sound/SoundBase.h"
#include "Particles/ParticleSystem.h"

// Sets default values
AAIAircraftPawn::AAIAircraftPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	AircraftMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AircraftMesh"));
	RootComponent = AircraftMesh;
	AircraftMesh->SetSimulatePhysics(true);

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

	MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	MuzzleLocation->SetupAttachment(AircraftMesh);

	FlightSpeed = 5000.0f;
	TurnSpeed = 50.0f;
	FireRate = 0.5f;
	WeaponRange = 30000.0f;
	FireAngleThreshold = 0.98f;
	AvoidanceDistance = 10000.0f;
	MaxSpeed = 8000.0f;
	CirclingOffsetDistance = 5000.0f;

	CurrentAIState = EAIState::Seeking;
}

// Called when the game starts or when spawned
void AAIAircraftPawn::BeginPlay()
{
	Super::BeginPlay();

	if (HealthComponent)
	{
		HealthComponent->OnHealthChanged.AddDynamic(this, &AAIAircraftPawn::HandleTakeDamage);
	}
}

// Called every frame
void AAIAircraftPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HealthComponent && HealthComponent->IsDead()) return;

	if (CurrentAIState == EAIState::Seeking || CurrentAIState == EAIState::Circling)
	{
		MoveAndTurn(DeltaTime);
		TryFireWeapon();
	}
	else if (CurrentAIState == EAIState::Evading)
	{
		PerformEvasion(DeltaTime);
	}
}

void AAIAircraftPawn::MoveAndTurn(float DeltaTime)
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!PlayerPawn) return;

	float DistanceToPlayer = FVector::Dist(PlayerPawn->GetActorLocation(), GetActorLocation());

	FVector TargetLocation;
	if (DistanceToPlayer < AvoidanceDistance)
	{
		CurrentAIState = EAIState::Circling;
		FVector RightVector = GetActorRightVector();
		TargetLocation = PlayerPawn->GetActorLocation() + (RightVector * CirclingOffsetDistance);
	}
	else
	{
		CurrentAIState = EAIState::Seeking;
		TargetLocation = PlayerPawn->GetActorLocation();
	}

	FVector DirectionToTarget = (TargetLocation - GetActorLocation()).GetSafeNormal();
	FRotator TargetRotation = FMath::RInterpTo(GetActorRotation(), DirectionToTarget.Rotation(), DeltaTime, TurnSpeed * 0.1f);
	SetActorRotation(TargetRotation);

	FVector CurrentVelocity = AircraftMesh->GetPhysicsLinearVelocity();
	if (CurrentVelocity.Size() < MaxSpeed)
	{
		AircraftMesh->AddForce(GetActorForwardVector() * FlightSpeed * 100.0f);
	}
}

void AAIAircraftPawn::TryFireWeapon()
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!PlayerPawn) return;

	FVector DirectionToPlayer = (PlayerPawn->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	float DotProduct = FVector::DotProduct(GetActorForwardVector(), DirectionToPlayer);

	if (DotProduct > FireAngleThreshold)
	{
		GetWorldTimerManager().SetTimer(FireRateTimerHandle, this, &AAIAircraftPawn::FireWeapon, FireRate, true, 0.0f);
	}
	else
	{
		GetWorldTimerManager().ClearTimer(FireRateTimerHandle);
	}
}

void AAIAircraftPawn::FireWeapon()
{
	FHitResult HitResult;
	FVector Start = MuzzleLocation->GetComponentLocation();
	FVector End = Start + (GetActorForwardVector() * WeaponRange);
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);

	GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, CollisionParams);

	if (HitResult.GetActor())
	{
		UHealthComponent* HitHealthComponent = HitResult.GetActor()->FindComponentByClass<UHealthComponent>();
		if (HitHealthComponent)
		{
			HitHealthComponent->TakeDamage(10.0f);
		}
	}

	if (MuzzleFlashFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlashFX, MuzzleLocation->GetComponentLocation());
	}

	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}
}

void AAIAircraftPawn::HandleTakeDamage(AActor* DamagedActor, float NewHealth)
{
	if (CurrentAIState != EAIState::Evading)
	{
		BeginEvasion();
	}
}

void AAIAircraftPawn::BeginEvasion()
{
	CurrentAIState = EAIState::Evading;
	GetWorldTimerManager().SetTimer(EvasionTimerHandle, this, &AAIAircraftPawn::EndEvasion, 2.0f, false);
}

void AAIAircraftPawn::EndEvasion()
{
	CurrentAIState = EAIState::Seeking;
}

void AAIAircraftPawn::PerformEvasion(float DeltaTime)
{
	FRotator EvasionRotation = GetActorRotation() + FRotator(0.0f, 90.0f, 0.0f);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), EvasionRotation, DeltaTime, TurnSpeed * 0.2f));

	FVector CurrentVelocity = AircraftMesh->GetPhysicsLinearVelocity();
	if (CurrentVelocity.Size() < MaxSpeed)
	{
		AircraftMesh->AddForce(GetActorForwardVector() * FlightSpeed * 100.0f);
	}
}

