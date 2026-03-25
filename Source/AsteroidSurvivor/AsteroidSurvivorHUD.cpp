// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsteroidSurvivorHUD.h"
#include "AsteroidSurvivorGameMode.h"
#include "AsteroidSurvivorShip.h"
#include "WaveManager.h"
#include "WeaponUpgradePickup.h"
#include "EnemyShipBase.h"
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

	// ── Global Timer (top centre) ───────────────────────────────────────────
	const AWaveManager* WM = GM->GetWaveManager();
	if (WM)
	{
		// Elapsed time
		FString TimerText = FString::Printf(TEXT("TIME: %s"), *FormatTime(WM->GetElapsedTime()));
		float TW_Timer, TH_Timer;
		GetTextSize(TimerText, TW_Timer, TH_Timer, HUDFont, 2.0f);
		DrawText(TimerText, FColor::White, (W - TW_Timer) * 0.5f, 10.0f, HUDFont, 2.0f);

		// Wave number
		FString WaveText = FString::Printf(TEXT("WAVE: %d"), WM->GetCurrentWave());
		float WW, WH;
		GetTextSize(WaveText, WW, WH, HUDFont, 1.2f);
		DrawText(WaveText, FColor(200, 200, 255), (W - WW) * 0.5f, 10.0f + TH_Timer + 2.0f, HUDFont, 1.2f);

		// Boss countdown
		float BossCountdown = WM->GetBossCountdown();
		if (BossCountdown > 0.0f && !WM->GetActiveBoss())
		{
			FString BossText = FString::Printf(TEXT("BOSS IN: %s"), *FormatTime(BossCountdown));
			float BW, BH;
			GetTextSize(BossText, BW, BH, HUDFont, 1.0f);
			DrawText(BossText, FColor(255, 100, 100),
			         (W - BW) * 0.5f, 10.0f + TH_Timer + WH + 4.0f, HUDFont, 1.0f);
		}
	}

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

	// ── Scrap counter ───────────────────────────────────────────────────────
	FString ScrapText = FString::Printf(TEXT("SCRAP: %d"), GM->GetCurrentScrap());
	DrawText(ScrapText, FColor(255, 200, 50), BarX, 142.0f, HUDFont, 1.2f);

	// ── Weapon arsenal (bottom-left) ────────────────────────────────────────
	if (Ship)
	{
		float WeaponY = H - 120.0f;
		FString BlasterText = FString::Printf(TEXT("Blaster Lv.%d"), Ship->GetBlasterLevel());
		DrawText(BlasterText, FColor(100, 255, 100), 20.0f, WeaponY, HUDFont, 0.9f);
		WeaponY += 20.0f;

		for (const auto& WeaponEntry : Ship->GetWeaponArsenal())
		{
			FString WeaponName = AWeaponUpgradePickup::GetWeaponDisplayName(WeaponEntry.Key);
			FString WeaponText = FString::Printf(TEXT("%s Lv.%d"), *WeaponName, WeaponEntry.Value);
			DrawText(WeaponText, FColor(150, 200, 255), 20.0f, WeaponY, HUDFont, 0.9f);
			WeaponY += 20.0f;
		}
	}

	// ── Boss health bar (top of screen, below timer) ────────────────────────
	if (WM)
	{
		AEnemyShipBase* Boss = WM->GetActiveBoss();
		if (Boss && IsValid(Boss) && Boss->GetMaxHealth() > 0.0f)
		{
			const float BossBarWidth = 400.0f;
			const float BossBarHeight = 20.0f;
			const float BossBarX = (W - BossBarWidth) * 0.5f;
			const float BossBarY = 80.0f;

			DrawText(TEXT("BOSS"), FColor(255, 50, 50),
			         BossBarX - 60.0f, BossBarY - 2.0f, HUDFont, 1.2f);

			const float BossFraction = Boss->GetCurrentHealth() / Boss->GetMaxHealth();
			DrawProgressBar(BossBarX, BossBarY, BossBarWidth, BossBarHeight,
			                BossFraction,
			                FLinearColor(1.0f, 0.15f, 0.3f),
			                FLinearColor(0.2f, 0.05f, 0.1f, 0.8f));

			FString BossHPText = FString::Printf(TEXT("%d / %d"),
				FMath::CeilToInt(Boss->GetCurrentHealth()),
				FMath::CeilToInt(Boss->GetMaxHealth()));
			DrawText(BossHPText, FColor::White, BossBarX + 5.0f, BossBarY - 2.0f, HUDFont, 0.9f);
		}
	}

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

FString AAsteroidSurvivorHUD::FormatTime(float Seconds)
{
	const int32 TotalSeconds = FMath::FloorToInt(Seconds);
	const int32 Minutes = TotalSeconds / 60;
	const int32 Secs = TotalSeconds % 60;
	return FString::Printf(TEXT("%02d:%02d"), Minutes, Secs);
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
