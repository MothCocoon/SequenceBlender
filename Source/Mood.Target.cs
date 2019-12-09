using UnrealBuildTool;
using System.Collections.Generic;

public class MoodTarget : TargetRules
{
    public MoodTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        ExtraModuleNames.Add("Mood");

        DefaultBuildSettings = BuildSettingsVersion.V2;
    }
}
