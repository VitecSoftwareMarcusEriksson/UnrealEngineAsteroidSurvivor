// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsteroidSurvivorHUD.h"
#include "AsteroidSurvivorGameMode.h"
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

	// ── Lives ────────────────────────────────────────────────────────────────
	FString LivesText = FString::Printf(TEXT("LIVES: %d"), GM->GetLives());
	DrawText(LivesText, FColor::White, 20.0f, 55.0f, HUDFont, 1.5f);

	// ── Wave ─────────────────────────────────────────────────────────────────
	FString WaveText = FString::Printf(TEXT("WAVE: %d"), GM->GetWave());
	// Right-align
	float WaveTextWidth, WaveTextHeight;
	GetTextSize(WaveText, WaveTextWidth, WaveTextHeight, HUDFont, 1.5f);
	DrawText(WaveText, FColor::White, W - WaveTextWidth - 20.0f, 20.0f, HUDFont, 1.5f);

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
