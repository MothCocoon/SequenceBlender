using UnrealBuildTool;

public class SequenceBlender : ModuleRules
{
    public SequenceBlender(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "LevelSequence",
            "MovieScene",
            "MovieSceneTracks",
            "Slate",
            "SlateCore"
        });
    }
}
