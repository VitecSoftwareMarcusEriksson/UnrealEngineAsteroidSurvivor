// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsteroidSurvivorEditorModule.h"
#include "Modules/ModuleManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/CoreDelegates.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Engine/World.h"
#include "HAL/PlatformFileManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionConstant.h"

void FAsteroidSurvivorEditorModule::StartupModule()
{
	if (!IsRunningGame())
	{
		// Always ensure materials exist on disk – including during cook
		// commandlets – so that the cooker can pick up the assets listed in
		// DirectoriesToAlwaysCook.  Without this, command-line packaging
		// (e.g. BuildCookRun) on a fresh clone would produce a package with
		// no M_SolidColor material, causing all colours to be missing at
		// runtime because the shipping-build fallback cannot create
		// materials dynamically.
		EnsureDefaultMaterialsExist();
	}

	if (!IsRunningCommandlet() && !IsRunningGame())
	{
		EnsureDefaultMapsExist();

		// Also register the delegate as a belt-and-suspenders fallback.
		PostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddRaw(
			this, &FAsteroidSurvivorEditorModule::OnPostEngineInit);
	}
}

void FAsteroidSurvivorEditorModule::ShutdownModule()
{
	if (PostEngineInitHandle.IsValid())
	{
		FCoreDelegates::OnPostEngineInit.Remove(PostEngineInitHandle);
	}
}

void FAsteroidSurvivorEditorModule::OnPostEngineInit()
{
	EnsureDefaultMaterialsExist();
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

	if (PlatformFile.FileExists(*FilePath))
	{
		// Validate that the existing material has its expressions properly wired
		// and uses the current material properties (non-flat roughness/metallic).
		UMaterial* Existing = LoadObject<UMaterial>(nullptr,
			TEXT("/Game/Materials/M_SolidColor.M_SolidColor"));
		if (Existing)
		{
			UMaterialEditorOnlyData* EditorData = Existing->GetEditorOnlyData();
			bool bIsValid = EditorData
				&& EditorData->BaseColor.Expression
				&& EditorData->EmissiveColor.Expression;

			// Also check that roughness is not the old flat value (1.0).
			// Older versions created the material with Roughness = 1 and
			// Metallic = 0, which made every object look completely flat.
			if (bIsValid && EditorData->Roughness.Expression)
			{
				if (const UMaterialExpressionConstant* RConst =
						Cast<UMaterialExpressionConstant>(EditorData->Roughness.Expression))
				{
					if (FMath::IsNearlyEqual(RConst->R, 1.0f, 0.01f))
					{
						bIsValid = false; // outdated flat material
					}
				}
			}

			// Verify that the vector parameters are actually registered and
			// discoverable by dynamic material instances.  Without valid
			// ExpressionGUIDs the parameters exist in the graph but
			// SetVectorParameterValue() on DMIs silently fails.
			if (bIsValid)
			{
				TArray<FMaterialParameterInfo> ParamInfos;
				TArray<FGuid> ParamGuids;
				Existing->GetAllVectorParameterInfo(ParamInfos, ParamGuids);

				bool bHasColor = false;
				bool bHasEmissive = false;
				for (const FMaterialParameterInfo& Info : ParamInfos)
				{
					if (Info.Name == FName(TEXT("Color")))
					{
						bHasColor = true;
					}
					if (Info.Name == FName(TEXT("EmissiveColor")))
					{
						bHasEmissive = true;
					}
				}

				if (!bHasColor || !bHasEmissive)
				{
					UE_LOG(LogTemp, Warning,
						TEXT("AsteroidSurvivor: M_SolidColor on disk is missing "
						     "registered vector parameters (Color=%d, "
						     "EmissiveColor=%d). Regenerating."),
						bHasColor, bHasEmissive);
					bIsValid = false;
				}
			}

			if (bIsValid)
			{
				return; // Material exists and is valid
			}

			// Material is broken or outdated – move the old object out of the
			// way so we can recreate it with the same name.
			Existing->Rename(nullptr, GetTransientPackage(),
				REN_DontCreateRedirectors | REN_NonTransactional);
		}
		PlatformFile.DeleteFile(*FilePath);
	}

	CreateSolidColorMaterial();

	// ── Translucent variant ──────────────────────────────────────────────
	const FString TranslucentPackageName = TEXT("/Game/Materials/M_SolidColor_Translucent");
	const FString TranslucentFilePath = FPackageName::LongPackageNameToFilename(
		TranslucentPackageName, FPackageName::GetAssetPackageExtension());

	bool bNeedTranslucent = !PlatformFile.FileExists(*TranslucentFilePath);
	if (!bNeedTranslucent)
	{
		UMaterial* ExistingTrans = LoadObject<UMaterial>(nullptr,
			TEXT("/Game/Materials/M_SolidColor_Translucent.M_SolidColor_Translucent"));
		if (!ExistingTrans || ExistingTrans->BlendMode != BLEND_Translucent)
		{
			if (ExistingTrans)
			{
				ExistingTrans->Rename(nullptr, GetTransientPackage(),
					REN_DontCreateRedirectors | REN_NonTransactional);
			}
			PlatformFile.DeleteFile(*TranslucentFilePath);
			bNeedTranslucent = true;
		}
	}

	if (bNeedTranslucent)
	{
		CreateSolidColorTranslucentMaterial();
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
	ColorParam->ExpressionGUID = FGuid::NewGuid();

	// ── "EmissiveColor" vector parameter → Emissive Color ────────────────
	UMaterialExpressionVectorParameter* EmissiveParam =
		NewObject<UMaterialExpressionVectorParameter>(Material);
	EmissiveParam->ParameterName = TEXT("EmissiveColor");
	EmissiveParam->DefaultValue = FLinearColor::Black;
	EmissiveParam->ExpressionGUID = FGuid::NewGuid();

	// ── Metallic constant – subtle reflectivity for 3-D depth ───────────
	UMaterialExpressionConstant* MetallicConst =
		NewObject<UMaterialExpressionConstant>(Material);
	MetallicConst->R = 0.15f;

	// ── Roughness constant – moderate specular for visible highlights ────
	UMaterialExpressionConstant* RoughnessConst =
		NewObject<UMaterialExpressionConstant>(Material);
	RoughnessConst->R = 0.35f;

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

	// Material settings – Opaque, Default Lit, moderate specular for 3-D depth
	Material->BlendMode = BLEND_Opaque;
	Material->SetShadingModel(MSM_DefaultLit);

	// Compile the material's shaders.  Do NOT call PreEditChange(nullptr) on
	// a freshly created material – that can corrupt the material's internal
	// state before PostEditChange has a chance to compile it properly.
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

void FAsteroidSurvivorEditorModule::CreateSolidColorTranslucentMaterial()
{
	const FString PackageName = TEXT("/Game/Materials/M_SolidColor_Translucent");

	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AsteroidSurvivor: Failed to create package for M_SolidColor_Translucent"));
		return;
	}

	UMaterial* Material = NewObject<UMaterial>(
		Package, TEXT("M_SolidColor_Translucent"), RF_Public | RF_Standalone);
	if (!Material)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AsteroidSurvivor: Failed to create M_SolidColor_Translucent material"));
		return;
	}

	// "Color" vector parameter → Base Color
	UMaterialExpressionVectorParameter* ColorParam =
		NewObject<UMaterialExpressionVectorParameter>(Material);
	ColorParam->ParameterName = TEXT("Color");
	ColorParam->DefaultValue = FLinearColor::White;
	ColorParam->ExpressionGUID = FGuid::NewGuid();

	// "EmissiveColor" vector parameter → Emissive Color
	UMaterialExpressionVectorParameter* EmissiveParam =
		NewObject<UMaterialExpressionVectorParameter>(Material);
	EmissiveParam->ParameterName = TEXT("EmissiveColor");
	EmissiveParam->DefaultValue = FLinearColor::Black;
	EmissiveParam->ExpressionGUID = FGuid::NewGuid();

	// "Opacity" scalar parameter → Opacity pin
	UMaterialExpressionScalarParameter* OpacityParam =
		NewObject<UMaterialExpressionScalarParameter>(Material);
	OpacityParam->ParameterName = TEXT("Opacity");
	OpacityParam->DefaultValue = 0.3f;
	OpacityParam->ExpressionGUID = FGuid::NewGuid();

	// Metallic constant
	UMaterialExpressionConstant* MetallicConst =
		NewObject<UMaterialExpressionConstant>(Material);
	MetallicConst->R = 0.15f;

	// Roughness constant
	UMaterialExpressionConstant* RoughnessConst =
		NewObject<UMaterialExpressionConstant>(Material);
	RoughnessConst->R = 0.35f;

	// Register expressions
	Material->GetExpressionCollection().AddExpression(ColorParam);
	Material->GetExpressionCollection().AddExpression(EmissiveParam);
	Material->GetExpressionCollection().AddExpression(OpacityParam);
	Material->GetExpressionCollection().AddExpression(MetallicConst);
	Material->GetExpressionCollection().AddExpression(RoughnessConst);

	// Wire expressions to material output pins
	UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData();
	EditorData->BaseColor.Expression     = ColorParam;
	EditorData->BaseColor.OutputIndex     = 0;
	EditorData->EmissiveColor.Expression = EmissiveParam;
	EditorData->EmissiveColor.OutputIndex = 0;
	EditorData->Opacity.Expression       = OpacityParam;
	EditorData->Opacity.OutputIndex       = 0;
	EditorData->Metallic.Expression      = MetallicConst;
	EditorData->Metallic.OutputIndex      = 0;
	EditorData->Roughness.Expression     = RoughnessConst;
	EditorData->Roughness.OutputIndex     = 0;

	// Translucent blend mode
	Material->BlendMode = BLEND_Translucent;
	Material->SetShadingModel(MSM_DefaultLit);

	// Compile shaders
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
			TEXT("AsteroidSurvivor: Created M_SolidColor_Translucent material"));
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AsteroidSurvivor: Failed to save M_SolidColor_Translucent material"));
	}
}

IMPLEMENT_MODULE(FAsteroidSurvivorEditorModule, AsteroidSurvivorEditor)
