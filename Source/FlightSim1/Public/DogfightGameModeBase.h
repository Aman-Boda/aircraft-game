// Copyright Your Company Name, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DogfightGameModeBase.generated.h"

class UUserWidget;

UCLASS()
class FLIGHTSIM1_API ADogfightGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADogfightGameModeBase();

	void EnemyDied();
	void PlayerDied();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	TSubclassOf<APawn> AIPawnClass;

	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	int32 NumberOfEnemiesToSpawn;

	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	float SpawnRadius;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> GameOverWidgetClass;

private:
	int32 LivingEnemies;

	void SpawnEnemies();
	void CheckWinCondition();
};

