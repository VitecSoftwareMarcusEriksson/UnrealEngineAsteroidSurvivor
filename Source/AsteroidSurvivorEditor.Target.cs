using UnrealBuildTool;
using System.Collections.Generic;

public class AsteroidSurvivorEditorTarget : TargetRules
{
    public AsteroidSurvivorEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;

        // Modern default build settings for UE 5.7
        DefaultBuildSettings = BuildSettingsVersion.V6;

        // Explicitly set C++ standard to match UE 5.7
        CppStandard = CppStandardVersion.Cpp20;

        // Add the main game module
        ExtraModuleNames.Add("AsteroidSurvivor");

        // Add the editor module that auto-generates default maps
        ExtraModuleNames.Add("AsteroidSurvivorEditor");
    }
}
