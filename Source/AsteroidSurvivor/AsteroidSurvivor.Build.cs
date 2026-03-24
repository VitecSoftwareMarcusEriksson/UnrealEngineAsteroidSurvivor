// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AsteroidSurvivor : ModuleRules
{
	public AsteroidSurvivor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"UMG"
		});
	}
}
