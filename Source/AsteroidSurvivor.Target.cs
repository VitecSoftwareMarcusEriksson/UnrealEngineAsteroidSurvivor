// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class AsteroidSurvivorTarget : TargetRules
{
	public AsteroidSurvivorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		CppStandard = CppStandardVersion.Cpp20;
		ExtraModuleNames.Add("AsteroidSurvivor");
	}
}
