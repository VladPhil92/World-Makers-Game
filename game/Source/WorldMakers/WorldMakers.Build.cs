using UnrealBuildTool;

public class WorldMakers : ModuleRules
{
    public WorldMakers(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "PhysicsCore",
            "ProceduralMeshComponent",
            "Niagara",
            "UMG"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Slate",
            "SlateCore",
            "Json"
        });
    }
}
