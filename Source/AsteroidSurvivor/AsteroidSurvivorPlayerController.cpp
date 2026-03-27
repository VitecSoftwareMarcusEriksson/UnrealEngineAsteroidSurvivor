// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsteroidSurvivorPlayerController.h"
#include "AsteroidSurvivorGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

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

	// Bind the Escape key to quit the game
	FInputKeyBinding EscKeyBinding(FInputChord(EKeys::Escape), IE_Pressed);
	EscKeyBinding.KeyDelegate.GetDelegateForManualSet().BindUObject(this, &AAsteroidSurvivorPlayerController::HandleQuit);
	InputComponent->KeyBindings.Add(EscKeyBinding);
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

void AAsteroidSurvivorPlayerController::HandleQuit()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}
