// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsteroidSurvivorEditorModule.h"
#include "Modules/ModuleManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Engine/World.h"
#include "HAL/PlatformFileManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionConstant.h"

void FAsteroidSurvivorEditorModule::StartupModule()
{
	if (!IsRunningCommandlet() && !IsRunningGame())
	{
		EnsureDefaultMapsExist();
		EnsureDefaultMaterialsExist();
	}
}

void FAsteroidSurvivorEditorModule::ShutdownModule()
{
}

void FAsteroidSurvivorEditorModule::EnsureDefaultMapsExist()
{
	// Make sure the Content/Maps directory exists on disk
	const FString MapsDir = FPaths::ProjectContentDir() / TEXT("Maps");
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.CreateDirectoryTree(*MapsDir);

	auto CreateIfMissing = [this, &PlatformFile](const TCHAR* PackageName)
	{
		const FString FilePath = FPackageName::LongPackageNameToFilename(
			PackageName, FPackageName::GetMapPackageExtension());

		if (!PlatformFile.FileExists(*FilePath))
		{
			CreateMinimalMap(PackageName);
		}
	};

	CreateIfMissing(TEXT("/Game/Maps/GameLevel"));
	CreateIfMissing(TEXT("/Game/Maps/MainMenu"));
}

void FAsteroidSurvivorEditorModule::CreateMinimalMap(const FString& PackageName)
{
	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AsteroidSurvivor: Failed to create package for '%s'"), *PackageName);
		return;
	}

	const FName WorldName = *FPackageName::GetShortName(PackageName);
	UWorld* World = UWorld::CreateWorld(EWorldType::None, /*bInformEngineOfWorld=*/false,
		WorldName, Package);
	if (!World)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AsteroidSurvivor: Failed to create world for '%s'"), *PackageName);
		return;
	}

	World->SetFlags(RF_Public | RF_Standalone);

	const FString FilePath = FPackageName::LongPackageNameToFilename(
		PackageName, FPackageName::GetMapPackageExtension());

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Standalone;

	const bool bSuccess = UPackage::SavePackage(
		Package, World, *FilePath, SaveArgs);

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log,
			TEXT("AsteroidSurvivor: Created default map '%s'"), *PackageName);
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AsteroidSurvivor: Failed to save map '%s'"),
			*PackageName);
	}

	World->DestroyWorld(/*bInformEngineOfWorld=*/false);
}

// ────────────────────────────────────────────────────────────────────────────
// Material asset generation
// ────────────────────────────────────────────────────────────────────────────

void FAsteroidSurvivorEditorModule::EnsureDefaultMaterialsExist()
{
	const FString MaterialsDir = FPaths::ProjectContentDir() / TEXT("Materials");
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.CreateDirectoryTree(*MaterialsDir);

	const FString PackageName = TEXT("/Game/Materials/M_SolidColor");
	const FString FilePath = FPackageName::LongPackageNameToFilename(
		PackageName, FPackageName::GetAssetPackageExtension());

	if (!PlatformFile.FileExists(*FilePath))
	{
		CreateSolidColorMaterial();
	}
}

void FAsteroidSurvivorEditorModule::CreateSolidColorMaterial()
{
	const FString PackageName = TEXT("/Game/Materials/M_SolidColor");

	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AsteroidSurvivor: Failed to create package for M_SolidColor"));
		return;
	}

	UMaterial* Material = NewObject<UMaterial>(
		Package, TEXT("M_SolidColor"), RF_Public | RF_Standalone);
	if (!Material)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AsteroidSurvivor: Failed to create M_SolidColor material"));
		return;
	}

	// ── "Color" vector parameter → Base Color ────────────────────────────
	UMaterialExpressionVectorParameter* ColorParam =
		NewObject<UMaterialExpressionVectorParameter>(Material);
	ColorParam->ParameterName = TEXT("Color");
	ColorParam->DefaultValue = FLinearColor::White;

	// ── "EmissiveColor" vector parameter → Emissive Color ────────────────
	UMaterialExpressionVectorParameter* EmissiveParam =
		NewObject<UMaterialExpressionVectorParameter>(Material);
	EmissiveParam->ParameterName = TEXT("EmissiveColor");
	EmissiveParam->DefaultValue = FLinearColor::Black;

	// ── Metallic constant = 0 ────────────────────────────────────────────
	UMaterialExpressionConstant* MetallicConst =
		NewObject<UMaterialExpressionConstant>(Material);
	MetallicConst->R = 0.0f;

	// ── Roughness constant = 1 ───────────────────────────────────────────
	UMaterialExpressionConstant* RoughnessConst =
		NewObject<UMaterialExpressionConstant>(Material);
	RoughnessConst->R = 1.0f;

	// Register expressions with the material
	Material->GetExpressionCollection().AddExpression(ColorParam);
	Material->GetExpressionCollection().AddExpression(EmissiveParam);
	Material->GetExpressionCollection().AddExpression(MetallicConst);
	Material->GetExpressionCollection().AddExpression(RoughnessConst);

	// Wire expressions to material output pins
	UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData();
	EditorData->BaseColor.Expression = ColorParam;
	EditorData->BaseColor.OutputIndex = 0;
	EditorData->EmissiveColor.Expression = EmissiveParam;
	EditorData->EmissiveColor.OutputIndex = 0;
	EditorData->Metallic.Expression = MetallicConst;
	EditorData->Metallic.OutputIndex = 0;
	EditorData->Roughness.Expression = RoughnessConst;
	EditorData->Roughness.OutputIndex = 0;

	// Material settings – Opaque, Default Lit, no reflections
	Material->BlendMode = BLEND_Opaque;
	Material->SetShadingModel(MSM_DefaultLit);

	// Trigger material compilation
	Material->PreEditChange(nullptr);
	Material->PostEditChange();

	// Save to disk
	const FString FilePath = FPackageName::LongPackageNameToFilename(
		PackageName, FPackageName::GetAssetPackageExtension());

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Standalone;

	const bool bSuccess = UPackage::SavePackage(
		Package, Material, *FilePath, SaveArgs);

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log,
			TEXT("AsteroidSurvivor: Created M_SolidColor material"));
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AsteroidSurvivor: Failed to save M_SolidColor material"));
	}
}

IMPLEMENT_MODULE(FAsteroidSurvivorEditorModule, AsteroidSurvivorEditor)
