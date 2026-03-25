// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsteroidSurvivorHUD.h"
#include "AsteroidSurvivorGameMode.h"
#include "AsteroidSurvivorShip.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AAsteroidSurvivorHUD::AAsteroidSurvivorHUD()
{
	// Load a default engine font for the HUD
	static ConstructorHelpers::FObjectFinder<UFont> DefaultFont(
		TEXT("/Engine/EngineFonts/RobotoDistanceField.RobotoDistanceField"));
	if (DefaultFont.Succeeded())
	{
		HUDFont = DefaultFont.Object;
	}
}

void AAsteroidSurvivorHUD::DrawHUD()
{
	Super::DrawHUD();

	const AAsteroidSurvivorGameMode* GM = Cast<AAsteroidSurvivorGameMode>(
		UGameplayStatics::GetGameMode(GetWorld()));

	if (!GM || !Canvas)
	{
		return;
	}

	const float W = Canvas->SizeX;
	const float H = Canvas->SizeY;

	// ── Score ────────────────────────────────────────────────────────────────
	FString ScoreText = FString::Printf(TEXT("SCORE: %d"), GM->GetScore());
	DrawText(ScoreText, FColor::White, 20.0f, 20.0f, HUDFont, 1.5f);

	// ── Level ───────────────────────────────────────────────────────────────
	FString LevelText = FString::Printf(TEXT("LEVEL: %d"), GM->GetCurrentLevel());
	DrawText(LevelText, FColor::Cyan, 20.0f, 55.0f, HUDFont, 1.5f);

	// ── Health bar ──────────────────────────────────────────────────────────
	const AAsteroidSurvivorShip* Ship = Cast<AAsteroidSurvivorShip>(
		UGameplayStatics::GetPlayerPawn(GetWorld(), 0));

	const float BarX = 20.0f;
	const float BarWidth = 200.0f;
	const float BarHeight = 16.0f;

	DrawText(TEXT("HP"), FColor::White, BarX, 88.0f, HUDFont, 1.0f);

	float HealthFraction = 0.0f;
	if (Ship && Ship->GetMaxHealth() > 0.0f)
	{
		HealthFraction = Ship->GetCurrentHealth() / Ship->GetMaxHealth();
	}

	// Color transitions from green → yellow → red based on health
	FLinearColor HealthColor;
	if (HealthFraction > 0.5f)
	{
		HealthColor = FLinearColor::LerpUsingHSV(
			FLinearColor(1.0f, 1.0f, 0.0f), FLinearColor(0.0f, 1.0f, 0.0f),
			(HealthFraction - 0.5f) * 2.0f);
	}
	else
	{
		HealthColor = FLinearColor::LerpUsingHSV(
			FLinearColor(1.0f, 0.0f, 0.0f), FLinearColor(1.0f, 1.0f, 0.0f),
			HealthFraction * 2.0f);
	}

	DrawProgressBar(BarX + 30.0f, 90.0f, BarWidth, BarHeight, HealthFraction,
	                HealthColor, FLinearColor(0.15f, 0.15f, 0.15f, 0.8f));

	// Health text on the bar
	if (Ship)
	{
		FString HPText = FString::Printf(TEXT("%d / %d"),
			FMath::CeilToInt(Ship->GetCurrentHealth()),
			FMath::CeilToInt(Ship->GetMaxHealth()));
		DrawText(HPText, FColor::White, BarX + 35.0f, 88.0f, HUDFont, 0.8f);
	}

	// Shield indicator
	if (Ship && Ship->IsShieldActive())
	{
		DrawText(TEXT("[SHIELD]"), FColor::Cyan, BarX + BarWidth + 40.0f, 88.0f, HUDFont, 1.0f);
	}

	// ── Thorium XP bar ──────────────────────────────────────────────────────
	DrawText(TEXT("XP"), FColor(100, 220, 255), BarX, 114.0f, HUDFont, 1.0f);

	float XPFraction = 0.0f;
	if (GM->GetThoriumForNextLevel() > 0)
	{
		XPFraction = static_cast<float>(GM->GetCurrentThorium()) /
		             static_cast<float>(GM->GetThoriumForNextLevel());
	}
	XPFraction = FMath::Clamp(XPFraction, 0.0f, 1.0f);

	DrawProgressBar(BarX + 30.0f, 116.0f, BarWidth, BarHeight, XPFraction,
	                FLinearColor(0.2f, 0.8f, 1.0f), FLinearColor(0.1f, 0.1f, 0.2f, 0.8f));

	FString XPText = FString::Printf(TEXT("%d / %d"),
		GM->GetCurrentThorium(), GM->GetThoriumForNextLevel());
	DrawText(XPText, FColor::White, BarX + 35.0f, 114.0f, HUDFont, 0.8f);

	// ── Upgrade selection overlay ───────────────────────────────────────────
	if (GM->IsSelectingUpgrade())
	{
		// Semi-transparent backdrop
		DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.6f), 0.0f, 0.0f, W, H);

		// Title
		FString Title = TEXT("LEVEL UP! Choose an Upgrade:");
		float TW, TH;
		GetTextSize(Title, TW, TH, HUDFont, 2.0f);
		DrawText(Title, FColor::Yellow, (W - TW) * 0.5f, H * 0.25f, HUDFont, 2.0f);

		const TArray<FUpgradeOption>& Options = GM->GetCurrentUpgradeOptions();
		const float OptionStartY = H * 0.38f;
		const float OptionSpacing = 70.0f;

		for (int32 i = 0; i < Options.Num(); ++i)
		{
			const FUpgradeOption& Opt = Options[i];

			// Option number + name
			FString OptText = FString::Printf(TEXT("[%d]  %s"), i + 1, *Opt.Name);
			float OW, OH;
			GetTextSize(OptText, OW, OH, HUDFont, 1.8f);
			DrawText(OptText, FColor::White,
			         (W - OW) * 0.5f, OptionStartY + i * OptionSpacing,
			         HUDFont, 1.8f);

			// Description
			float DW, DH;
			GetTextSize(Opt.Description, DW, DH, HUDFont, 1.2f);
			DrawText(Opt.Description, FColor(180, 180, 180),
			         (W - DW) * 0.5f, OptionStartY + i * OptionSpacing + OH + 4.0f,
			         HUDFont, 1.2f);
		}

		// Instruction
		FString Hint = TEXT("Press 1, 2 or 3 to select");
		float HW, HH;
		GetTextSize(Hint, HW, HH, HUDFont, 1.2f);
		DrawText(Hint, FColor(255, 255, 100),
		         (W - HW) * 0.5f, OptionStartY + Options.Num() * OptionSpacing + 30.0f,
		         HUDFont, 1.2f);
	}

	// ── Game Over overlay ────────────────────────────────────────────────────
	if (GM->IsGameOver())
	{
		FString GOText = TEXT("GAME OVER");
		float GOWidth, GOHeight;
		GetTextSize(GOText, GOWidth, GOHeight, HUDFont, 3.0f);
		DrawText(GOText, FColor::Red,
		         (W - GOWidth) * 0.5f, (H - GOHeight) * 0.5f,
		         HUDFont, 3.0f);

		FString RestartText = TEXT("Press R to Restart");
		float RW, RH;
		GetTextSize(RestartText, RW, RH, HUDFont, 1.5f);
		DrawText(RestartText, FColor::Yellow,
		         (W - RW) * 0.5f, (H - GOHeight) * 0.5f + GOHeight + 20.0f,
		         HUDFont, 1.5f);
	}
}

void AAsteroidSurvivorHUD::DrawProgressBar(float X, float Y, float Width, float Height,
                                            float Fraction, const FLinearColor& FillColor,
                                            const FLinearColor& BackColor)
{
	Fraction = FMath::Clamp(Fraction, 0.0f, 1.0f);

	// Background
	DrawRect(BackColor, X, Y, Width, Height);

	// Fill
	if (Fraction > 0.0f)
	{
		DrawRect(FillColor, X, Y, Width * Fraction, Height);
	}

	// Thin border (draw 4 edges)
	const FLinearColor BorderColor(0.5f, 0.5f, 0.5f, 0.8f);
	const float BorderThickness = 1.0f;
	DrawRect(BorderColor, X, Y, Width, BorderThickness);                           // Top
	DrawRect(BorderColor, X, Y + Height - BorderThickness, Width, BorderThickness); // Bottom
	DrawRect(BorderColor, X, Y, BorderThickness, Height);                           // Left
	DrawRect(BorderColor, X + Width - BorderThickness, Y, BorderThickness, Height); // Right
}
