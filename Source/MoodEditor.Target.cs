using UnrealBuildTool;
using System.Collections.Generic;

public class MoodEditorTarget : TargetRules
{
    public MoodEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        ExtraModuleNames.Add("MoodEditor");
    }
}
