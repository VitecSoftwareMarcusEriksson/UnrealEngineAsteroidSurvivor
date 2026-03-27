// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleInterface.h"
#include "Delegates/IDelegateInstance.h"

/**
 * Editor module that auto-generates default level maps and material assets
 * when the project is first opened in the Unreal Editor.  This ensures the
 * game is playable out of the box without requiring users to create assets
 * manually.
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

	/**
	 * Deferred callback – runs after the engine is fully initialised so that
	 * material creation does not trigger "UpdateValidators before
	 * RegisterBlueprintValidators" warnings.
	 */
	void OnPostEngineInit();

	/** Create shared material assets if they do not exist on disk. */
	void EnsureDefaultMaterialsExist();

	/**
	 * Create M_SolidColor – a simple opaque material with a "Color" vector
	 * parameter driving Base Color and an "EmissiveColor" parameter driving
	 * Emissive Color.  Metallic is 0.15 and Roughness is 0.35 for specular
	 * highlights that give objects a visible 3-D shape.
	 */
	void CreateSolidColorMaterial();

	/**
	 * Create M_SolidColor_Translucent – a translucent variant of M_SolidColor
	 * with an additional "Opacity" scalar parameter for semi-transparent
	 * effects like shields.
	 */
	void CreateSolidColorTranslucentMaterial();

	/** Handle for the OnPostEngineInit delegate so it can be unregistered. */
	FDelegateHandle PostEngineInitHandle;
};
