// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UMaterial;

/**
 * Static helper that ensures the M_SolidColor material is available at
 * runtime.  All actors that need a solid-coloured emissive material should
 * call GetOrCreateMaterial() instead of LoadObject directly.
 *
 * The function first tries to load the material from disk (created by the
 * Editor module).  If the asset is unavailable – e.g. on a fresh clone
 * before the Editor has been opened – it creates an equivalent transient
 * material at runtime (editor builds only) so that colours, 3-D shading,
 * and glow effects work out of the box.
 */
class FSolidColorMaterialHelper
{
public:
	/**
	 * Return the shared M_SolidColor material, creating it on the fly if
	 * necessary.  Returns nullptr only in non-editor (shipping) builds
	 * when the asset does not exist on disk.
	 */
	static UMaterial* GetOrCreateMaterial();

	/**
	 * Return a translucent variant of M_SolidColor with an "Opacity"
	 * scalar parameter.  Used for semi-transparent effects like shields.
	 * Returns nullptr in non-editor builds when the asset does not exist
	 * on disk.
	 */
	static UMaterial* GetOrCreateTranslucentMaterial();

private:
	/** Weak cache so we only create the material once per session. */
	static TWeakObjectPtr<UMaterial> CachedMaterial;

	/** Weak cache for the translucent variant. */
	static TWeakObjectPtr<UMaterial> CachedTranslucentMaterial;
};
