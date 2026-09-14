using UnrealBuildTool;

public class WorldMakers : ModuleRules
{
    public WorldMakers(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // The runtime sources use module-root-qualified includes such as
        // Adventure/... and Mission/.... Make the module root explicit so
        // UnrealBuildTool resolves those headers consistently on UE 5.8.x.
        PublicIncludePaths.Add(ModuleDirectory);

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
            "Json",
            "HTTP"
        });
    }
}
