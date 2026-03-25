// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "AsteroidSurvivorHUD.generated.h"

/**
 * HUD for Asteroid Survivor.
 * Draws score, health bar, Thorium XP bar, level indicator, upgrade
 * selection screen, Game Over overlay, global timer, boss health bar,
 * scrap counter, and weapon arsenal info via Canvas.
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

private:
	/** Draws a filled progress bar on the Canvas. */
	void DrawProgressBar(float X, float Y, float Width, float Height,
	                     float Fraction, const FLinearColor& FillColor,
	                     const FLinearColor& BackColor);

	/** Formats seconds into MM:SS display string. */
	static FString FormatTime(float Seconds);
};
