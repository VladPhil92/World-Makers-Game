using UnrealBuildTool;
using System.Collections.Generic;

public class WorldMakersEditorTarget : TargetRules
{
    public WorldMakersEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        ExtraModuleNames.Add("WorldMakers");
    }
}
