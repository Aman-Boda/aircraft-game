// Copyright Your Company Name, Inc. All Rights Reserved.

#include "DogfightGameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"

ADogfightGameModeBase::ADogfightGameModeBase()
{
	NumberOfEnemiesToSpawn = 3;
	SpawnRadius = 20000.0f;
	LivingEnemies = 0;
}

void ADogfightGameModeBase::BeginPlay()
{
	Super::BeginPlay();
	SpawnEnemies();
}

void ADogfightGameModeBase::SpawnEnemies()
{
	if (!AIPawnClass) return;

	for (int32 i = 0; i < NumberOfEnemiesToSpawn; ++i)
	{
		float Angle = FMath::FRandRange(0.0f, 360.0f);
		FVector SpawnLocation = FVector(SpawnRadius * FMath::Cos(Angle), SpawnRadius * FMath::Sin(Angle), 2000.0f);
		FRotator SpawnRotation = FRotator::ZeroRotator;
		GetWorld()->SpawnActor<APawn>(AIPawnClass, SpawnLocation, SpawnRotation);
	}
	LivingEnemies = NumberOfEnemiesToSpawn;
}

void ADogfightGameModeBase::EnemyDied()
{
	LivingEnemies--;
	CheckWinCondition();
}

// CORRECTED: The class name typo "ADogdigitGameModeBase" has been fixed.
void ADogfightGameModeBase::PlayerDied()
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Magenta, TEXT("Game Mode: PlayerDied() function was called!"));
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (PlayerController && GameOverWidgetClass)
	{
		PlayerController->bShowMouseCursor = true;
		PlayerController->bEnableClickEvents = true;
		PlayerController->bEnableMouseOverEvents = true;
		PlayerController->SetInputMode(FInputModeUIOnly());

		UUserWidget* GameOverWidget = CreateWidget<UUserWidget>(PlayerController, GameOverWidgetClass);
		if (GameOverWidget)
		{
			GameOverWidget->AddToViewport();
		}
	}
}

void ADogfightGameModeBase::CheckWinCondition()
{
	if (LivingEnemies <= 0)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("YOU WIN!"), true, FVector2D(3.f, 3.f));
		}
	}
}

