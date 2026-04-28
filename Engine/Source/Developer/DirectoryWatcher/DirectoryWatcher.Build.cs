// DirectoryWatcher.Build.cs -- Build rules for the DirectoryWatcher module.
// General-purpose file system monitoring service (Developer tool).

using EnigmaEngine;

public class DirectoryWatcher : ModuleRules
{
    public DirectoryWatcher(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.DeveloperTool;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencyModuleNames.Add("Core");
        PrivateTestDependencyModuleNames.Add("AutomationTest");
    }
}
