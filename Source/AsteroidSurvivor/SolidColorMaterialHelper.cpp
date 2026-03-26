// Copyright Epic Games, Inc. All Rights Reserved.

#include "SolidColorMaterialHelper.h"
#include "Materials/Material.h"

#if WITH_EDITORONLY_DATA
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionConstant.h"
#endif

TWeakObjectPtr<UMaterial> FSolidColorMaterialHelper::CachedMaterial;

UMaterial* FSolidColorMaterialHelper::GetOrCreateMaterial()
{
	// Return cached material if still valid.
	if (CachedMaterial.IsValid())
	{
		return CachedMaterial.Get();
	}

	// Try to load the material saved to disk by the Editor module.
	UMaterial* Mat = LoadObject<UMaterial>(
		nullptr, TEXT("/Game/Materials/M_SolidColor.M_SolidColor"));
	if (Mat)
	{
		CachedMaterial = Mat;
		return Mat;
	}

#if WITH_EDITORONLY_DATA
	// ── Runtime fallback: create a transient material on the fly ──────────
	// This ensures colours, emissive glow, and specular highlights are
	// available even when the asset has not been generated yet.

	Mat = NewObject<UMaterial>(
		GetTransientPackage(), TEXT("M_SolidColor_Runtime"), RF_Transient);
	if (!Mat)
	{
		return nullptr;
	}

	// "Color" vector parameter → Base Color
	UMaterialExpressionVectorParameter* ColorParam =
		NewObject<UMaterialExpressionVectorParameter>(Mat);
	ColorParam->ParameterName = TEXT("Color");
	ColorParam->DefaultValue = FLinearColor::White;

	// "EmissiveColor" vector parameter → Emissive Color
	UMaterialExpressionVectorParameter* EmissiveParam =
		NewObject<UMaterialExpressionVectorParameter>(Mat);
	EmissiveParam->ParameterName = TEXT("EmissiveColor");
	EmissiveParam->DefaultValue = FLinearColor::Black;

	// Metallic constant – subtle reflectivity for 3-D depth
	UMaterialExpressionConstant* MetallicConst =
		NewObject<UMaterialExpressionConstant>(Mat);
	MetallicConst->R = 0.15f;

	// Roughness constant – moderate specular for visible highlights
	UMaterialExpressionConstant* RoughnessConst =
		NewObject<UMaterialExpressionConstant>(Mat);
	RoughnessConst->R = 0.35f;

	// Register expressions
	Mat->GetExpressionCollection().AddExpression(ColorParam);
	Mat->GetExpressionCollection().AddExpression(EmissiveParam);
	Mat->GetExpressionCollection().AddExpression(MetallicConst);
	Mat->GetExpressionCollection().AddExpression(RoughnessConst);

	// Wire expressions to material output pins
	UMaterialEditorOnlyData* EditorData = Mat->GetEditorOnlyData();
	EditorData->BaseColor.Expression     = ColorParam;
	EditorData->BaseColor.OutputIndex     = 0;
	EditorData->EmissiveColor.Expression = EmissiveParam;
	EditorData->EmissiveColor.OutputIndex = 0;
	EditorData->Metallic.Expression      = MetallicConst;
	EditorData->Metallic.OutputIndex      = 0;
	EditorData->Roughness.Expression     = RoughnessConst;
	EditorData->Roughness.OutputIndex     = 0;

	// Material settings
	Mat->BlendMode = BLEND_Opaque;
	Mat->SetShadingModel(MSM_DefaultLit);

	// Compile shaders
	Mat->PostEditChange();

	CachedMaterial = Mat;
	return Mat;
#else
	return nullptr;
#endif
}
