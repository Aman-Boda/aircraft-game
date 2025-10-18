// Copyright Your Company Name, Inc. All Rights Reserved.

#include "HealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "DogfightGameModeBase.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	MaxHealth = 100.0f;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
}

void UHealthComponent::TakeDamage(float Damage)
{
	if (IsDead()) return;

	CurrentHealth = FMath::Clamp(CurrentHealth - Damage, 0.0f, MaxHealth);

	OnHealthChanged.Broadcast(GetOwner(), CurrentHealth);

	if (IsDead())
	{
		Die();
	}
}

bool UHealthComponent::IsDead() const
{
	return CurrentHealth <= 0.0f;
}

void UHealthComponent::Die()
{
	// Notify the Game Mode
	AGameModeBase* GameMode = UGameplayStatics::GetGameMode(GetWorld());
	if (ADogfightGameModeBase* DogfightGameMode = Cast<ADogfightGameModeBase>(GameMode))
	{
		APawn* OwnerPawn = Cast<APawn>(GetOwner());
		if (OwnerPawn && OwnerPawn->IsPlayerControlled())
		{
			DogfightGameMode->PlayerDied();
		}
		else
		{
			DogfightGameMode->EnemyDied();
		}
	}

	OnDeath.Broadcast();

	AActor* Owner = GetOwner();
	if (Owner)
	{
		UStaticMeshComponent* MeshComponent = Owner->FindComponentByClass<UStaticMeshComponent>();
		if (MeshComponent)
		{
			MeshComponent->SetSimulatePhysics(false);
			MeshComponent->SetVisibility(false);
			MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		Owner->Destroy();
	}
}

