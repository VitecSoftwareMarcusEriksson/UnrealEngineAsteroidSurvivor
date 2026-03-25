// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AsteroidSurvivorEditor : ModuleRules
{
	public AsteroidSurvivorEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd"
		});
	}
}
