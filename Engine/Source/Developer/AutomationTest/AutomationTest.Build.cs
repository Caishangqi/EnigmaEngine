// AutomationTest.Build.cs -- Build rules for Enigma automation test framework.
// Developer-only test authoring API and registry.

using EnigmaEngine;

public class AutomationTest : ModuleRules
{
    public AutomationTest(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.DeveloperTool;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencyModuleNames.Add("Core");
        PrivateTestDependencyModuleNames.Add("googletest");
    }
}
