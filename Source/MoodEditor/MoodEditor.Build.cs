using UnrealBuildTool;

public class MoodEditor : ModuleRules
{
    public MoodEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore",
                "Mood"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
				"Slate",
                "SlateCore",
                "UnrealEd"
            }
        );
    }
}
