// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleInterface.h"

/**
 * Editor module that auto-generates default level maps when the project is
 * first opened in the Unreal Editor.  This ensures the game is playable out
 * of the box without requiring users to create maps manually.
 */
class FAsteroidSurvivorEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/** Create GameLevel and MainMenu maps if they do not exist on disk. */
	void EnsureDefaultMapsExist();

	/**
	 * Create a minimal empty .umap file at the given package path.
	 * @param PackageName  Long package name, e.g. "/Game/Maps/GameLevel"
	 */
	void CreateMinimalMap(const FString& PackageName);

	/** Create shared material assets if they do not exist on disk. */
	void EnsureDefaultMaterialsExist();

	/**
	 * Create M_SolidColor – a simple opaque material with a "Color" vector
	 * parameter driving Base Color and an "EmissiveColor" parameter driving
	 * Emissive Color.  Metallic is 0 and Roughness is 1 so objects display
	 * pure, reflection-free colour.
	 */
	void CreateSolidColorMaterial();
};
