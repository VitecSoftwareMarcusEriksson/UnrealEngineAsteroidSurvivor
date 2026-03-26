// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsteroidSurvivorPlayerController.h"
#include "AsteroidSurvivorGameMode.h"
#include "Kismet/GameplayStatics.h"

AAsteroidSurvivorPlayerController::AAsteroidSurvivorPlayerController()
{
	bShowMouseCursor = false;
}

void AAsteroidSurvivorPlayerController::BeginPlay()
{
	Super::BeginPlay();

	SetInputMode(FInputModeGameOnly());
}

void AAsteroidSurvivorPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Bind the R key directly so it works without a project input mapping
	FInputKeyBinding RKeyBinding(FInputChord(EKeys::R), IE_Pressed);
	RKeyBinding.KeyDelegate.GetDelegateForManualSet().BindUObject(this, &AAsteroidSurvivorPlayerController::HandleRestart);
	InputComponent->KeyBindings.Add(RKeyBinding);
}

void AAsteroidSurvivorPlayerController::HandleRestart()
{
	AAsteroidSurvivorGameMode* GM = Cast<AAsteroidSurvivorGameMode>(
		UGameplayStatics::GetGameMode(this));
	if (GM && GM->IsGameOver())
	{
		UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetMapName()), true);
	}
}
