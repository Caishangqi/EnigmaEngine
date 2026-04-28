// AutomationTestRunner.Build.cs -- Build rules for the standalone automation test runner.

using EnigmaEngine;

public class AutomationTestRunner : ModuleRules
{
    public AutomationTestRunner(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.DeveloperTool;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PrivateDependencyModuleNames.Add("AutomationTest");
        PrivateDependencyModuleNames.Add("nlohmann_json");
        PrivateTestDependencyModuleNames.Add("googletest");
    }
}
