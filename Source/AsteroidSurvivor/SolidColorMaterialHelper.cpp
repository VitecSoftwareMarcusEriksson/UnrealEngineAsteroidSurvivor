// Copyright Epic Games, Inc. All Rights Reserved.

#include "SolidColorMaterialHelper.h"
#include "Materials/Material.h"

#if WITH_EDITORONLY_DATA
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionConstant.h"
#endif

TWeakObjectPtr<UMaterial> FSolidColorMaterialHelper::CachedMaterial;

/**
 * Check whether a material has the "Color" and "EmissiveColor" vector
 * parameters that our actors rely on.  Without these parameters,
 * SetVectorParameterValue() on dynamic instances silently fails and
 * every mesh renders as plain white.
 */
static bool HasExpectedParameters(const UMaterial* Mat)
{
	if (!Mat)
	{
		return false;
	}

	TArray<FMaterialParameterInfo> Infos;
	TArray<FGuid> Guids;
	Mat->GetAllVectorParameterInfo(Infos, Guids);

	bool bColor = false;
	bool bEmissive = false;
	for (const FMaterialParameterInfo& Info : Infos)
	{
		if (Info.Name == FName(TEXT("Color")))
		{
			bColor = true;
		}
		if (Info.Name == FName(TEXT("EmissiveColor")))
		{
			bEmissive = true;
		}
	}
	return bColor && bEmissive;
}

UMaterial* FSolidColorMaterialHelper::GetOrCreateMaterial()
{
	// Return cached material if still valid and has working parameters.
	if (CachedMaterial.IsValid() && HasExpectedParameters(CachedMaterial.Get()))
	{
		return CachedMaterial.Get();
	}

	// Try to load the material saved to disk by the Editor module.
	UMaterial* Mat = LoadObject<UMaterial>(
		nullptr, TEXT("/Game/Materials/M_SolidColor.M_SolidColor"));
	if (Mat && HasExpectedParameters(Mat))
	{
		CachedMaterial = Mat;
		return Mat;
	}

	if (Mat)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AsteroidSurvivor: M_SolidColor loaded from disk but is "
			     "missing expected vector parameters. Using runtime fallback."));
	}

#if WITH_EDITORONLY_DATA
	// ── Runtime fallback: create a transient material on the fly ──────────
	// This ensures colours, emissive glow, and specular highlights are
	// available even when the on-disk asset has not been generated yet or
	// is outdated.

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
	// A valid GUID is required for dynamic material instances to be able to
	// find and override this parameter at runtime.
	ColorParam->ExpressionGUID = FGuid::NewGuid();

	// "EmissiveColor" vector parameter → Emissive Color
	UMaterialExpressionVectorParameter* EmissiveParam =
		NewObject<UMaterialExpressionVectorParameter>(Mat);
	EmissiveParam->ParameterName = TEXT("EmissiveColor");
	EmissiveParam->DefaultValue = FLinearColor::Black;
	EmissiveParam->ExpressionGUID = FGuid::NewGuid();

	// Metallic constant – subtle reflectivity for 3-D depth
	UMaterialExpressionConstant* MetallicConst =
		NewObject<UMaterialExpressionConstant>(Mat);
	MetallicConst->R = 0.15f;

	// Roughness constant – moderate specular for visible highlights
	UMaterialExpressionConstant* RoughnessConst =
		NewObject<UMaterialExpressionConstant>(Mat);
	RoughnessConst->R = 0.35f;

	// Register expressions with the material's expression collection.
	Mat->GetExpressionCollection().AddExpression(ColorParam);
	Mat->GetExpressionCollection().AddExpression(EmissiveParam);
	Mat->GetExpressionCollection().AddExpression(MetallicConst);
	Mat->GetExpressionCollection().AddExpression(RoughnessConst);

	// Wire expressions to material output pins
	UMaterialEditorOnlyData* EditorData = Mat->GetEditorOnlyData();
	if (EditorData)
	{
		EditorData->BaseColor.Expression     = ColorParam;
		EditorData->BaseColor.OutputIndex     = 0;
		EditorData->EmissiveColor.Expression = EmissiveParam;
		EditorData->EmissiveColor.OutputIndex = 0;
		EditorData->Metallic.Expression      = MetallicConst;
		EditorData->Metallic.OutputIndex      = 0;
		EditorData->Roughness.Expression     = RoughnessConst;
		EditorData->Roughness.OutputIndex     = 0;
	}

	// Material settings
	Mat->BlendMode = BLEND_Opaque;
	Mat->SetShadingModel(MSM_DefaultLit);

	// Compile shaders
	Mat->PostEditChange();

	UE_LOG(LogTemp, Log,
		TEXT("AsteroidSurvivor: Created runtime M_SolidColor material "
		     "(params valid: %s)"),
		HasExpectedParameters(Mat) ? TEXT("yes") : TEXT("NO"));

	CachedMaterial = Mat;
	return Mat;
#else
	return nullptr;
#endif
}
