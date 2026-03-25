// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsteroidSurvivorEditorModule.h"
#include "Modules/ModuleManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Engine/World.h"
#include "HAL/PlatformFileManager.h"

void FAsteroidSurvivorEditorModule::StartupModule()
{
	if (!IsRunningCommandlet() && !IsRunningGame())
	{
		EnsureDefaultMapsExist();
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

IMPLEMENT_MODULE(FAsteroidSurvivorEditorModule, AsteroidSurvivorEditor)
