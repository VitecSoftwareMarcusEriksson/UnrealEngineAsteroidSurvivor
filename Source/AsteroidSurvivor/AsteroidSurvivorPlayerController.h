// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AsteroidSurvivorPlayerController.generated.h"

/**
 * Player controller for Asteroid Survivor.
 * Hides the hardware cursor and ensures the game is always in game-input mode.
 */
UCLASS()
class ASTEROIDSURVIVOR_API AAsteroidSurvivorPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAsteroidSurvivorPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	/** Restarts the current level when the game is over. */
	void HandleRestart();

	/** Exits the game when the Escape key is pressed. */
	void HandleQuit();
};
