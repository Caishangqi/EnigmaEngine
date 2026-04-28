// ApplicationCore.Build.cs -- Build rules for the ApplicationCore module.
// ApplicationCore provides platform-agnostic application and window abstractions.

using EnigmaEngine;

public class ApplicationCore : ModuleRules
{
    public ApplicationCore()
    {
        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencyModuleNames.Add("Core");
        PrivateTestDependencyModuleNames.Add("AutomationTest");
    }
}
