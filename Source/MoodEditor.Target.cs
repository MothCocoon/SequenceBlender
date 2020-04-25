using UnrealBuildTool;

public class MoodEditorTarget : TargetRules
{
    public MoodEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        ExtraModuleNames.Add("MoodEditor");

        DefaultBuildSettings = BuildSettingsVersion.V2;
    }
}
