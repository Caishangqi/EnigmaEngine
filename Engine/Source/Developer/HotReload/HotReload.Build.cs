// HotReload.Build.cs -- Build rules for the HotReload module.
// DLL hot-reload orchestration (Developer tool).

using EnigmaEngine;

public class HotReload : ModuleRules
{
    public HotReload(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.DeveloperTool;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencyModuleNames.Add("Core");
        PublicDependencyModuleNames.Add("DirectoryWatcher");

        // [TEST] Engine dependency for GameInstance recreation after hot-reload.
        // Remove when Editor exists and handles object reconstruction.
        PrivateDependencyModuleNames.Add("Engine");
        PrivateTestDependencyModuleNames.Add("AutomationTest");
    }
}
