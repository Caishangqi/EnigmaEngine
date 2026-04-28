// RenderCore.Build.cs -- Build rules for the RenderCore module.
// RenderCore defines renderer interfaces and ASCII data types.
// Depends on Core only.

using EnigmaEngine;

public class RenderCore : ModuleRules
{
	public RenderCore()
	{
		PublicIncludePaths.Add("Public");
		PrivateIncludePaths.Add("Private");

		PublicDependencyModuleNames.Add("Core");
		PrivateTestDependencyModuleNames.Add("AutomationTest");
	}
}
