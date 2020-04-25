using UnrealBuildTool;

public class Mood : ModuleRules
{
    public Mood(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(
            new[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore",
                "LevelSequence",
                "MovieScene",
                "MovieSceneTracks"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new[]
            {
                "Slate",
                "SlateCore",
            }
        );
    }
}
