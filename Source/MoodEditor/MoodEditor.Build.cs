using UnrealBuildTool;

public class MoodEditor : ModuleRules
{
    public MoodEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(
            new[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore",
                "Mood"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new[]
            {
				"Slate",
                "SlateCore",
                "UnrealEd"
            }
        );
    }
}
