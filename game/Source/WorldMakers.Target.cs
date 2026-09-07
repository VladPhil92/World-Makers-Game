using UnrealBuildTool;
using System.Collections.Generic;

public class WorldMakersTarget : TargetRules
{
    public WorldMakersTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        ExtraModuleNames.Add("WorldMakers");
    }
}
