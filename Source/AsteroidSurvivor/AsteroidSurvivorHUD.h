// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "AsteroidSurvivorHUD.generated.h"

/**
 * HUD for Asteroid Survivor.
 * Draws score, lives, wave number and a Game Over message directly via Canvas.
 * For a polished UI, replace or supplement this with a UMG widget.
 */
UCLASS()
class ASTEROIDSURVIVOR_API AAsteroidSurvivorHUD : public AHUD
{
	GENERATED_BODY()

public:
	AAsteroidSurvivorHUD();

	virtual void DrawHUD() override;

protected:
	/** Font used for HUD text */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD")
	UFont* HUDFont = nullptr;
};
