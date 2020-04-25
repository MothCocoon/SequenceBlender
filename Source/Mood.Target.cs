using UnrealBuildTool;

public class MoodTarget : TargetRules
{
    public MoodTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        ExtraModuleNames.Add("Mood");

        DefaultBuildSettings = BuildSettingsVersion.V2;
    }
}
