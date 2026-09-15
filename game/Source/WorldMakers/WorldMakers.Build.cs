using UnrealBuildTool;

public class WorldMakers : ModuleRules
{
    public WorldMakers(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // World Makers contains many intentionally file-local anonymous-namespace
        // helpers. Keep the module on non-unity compilation so each .cpp remains
        // an independent translation unit, matching standard C++ semantics and
        // preventing Unreal unity aggregation from creating false redefinitions.
        // This also makes missing include dependencies surface deterministically.
        bUseUnity = false;

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
            "EnhancedInput",
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
