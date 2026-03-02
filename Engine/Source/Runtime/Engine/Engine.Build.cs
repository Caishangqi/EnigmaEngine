// Engine.Build.cs -- Build rules for the Engine module.
// Engine depends on Core and provides the engine loop, GEngine, and game instance.

using EnigmaEngine;

public class Engine : ModuleRules
{
    public Engine()
    {
        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencyModuleNames.Add("Core");
        PublicDependencyModuleNames.Add("ApplicationCore");
        PublicDependencyModuleNames.Add("RenderCore");
    }
}
