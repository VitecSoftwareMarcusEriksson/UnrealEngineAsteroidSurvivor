// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsteroidSurvivorPlayerController.h"

AAsteroidSurvivorPlayerController::AAsteroidSurvivorPlayerController()
{
	bShowMouseCursor = false;
}

void AAsteroidSurvivorPlayerController::BeginPlay()
{
	Super::BeginPlay();

	SetInputMode(FInputModeGameOnly());
}
