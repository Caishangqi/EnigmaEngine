// NlohmannJsonIntegrationTest.Build.cs
// Depends on nlohmann_json ThirdParty header-only module.

using EnigmaEngine;

public class NlohmannJsonIntegrationTest : ModuleRules
{
    public NlohmannJsonIntegrationTest()
    {
        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencyModuleNames.Add("nlohmann_json");
    }
}
