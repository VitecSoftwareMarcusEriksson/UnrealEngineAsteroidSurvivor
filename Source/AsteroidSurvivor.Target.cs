// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class AsteroidSurvivorTarget : TargetRules
{
	public AsteroidSurvivorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V3;
		ExtraModuleNames.Add("AsteroidSurvivor");
	}
}
