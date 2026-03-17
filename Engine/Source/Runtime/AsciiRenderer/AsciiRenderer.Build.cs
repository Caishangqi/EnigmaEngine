// AsciiRenderer.Build.cs -- Build rules for the AsciiRenderer module.
// AsciiRenderer implements the ASCII console rendering pipeline.
// Depends on RenderCore (interfaces + data types), Core, and ApplicationCore (window handle).

using EnigmaEngine;

public class AsciiRenderer : ModuleRules
{
	public AsciiRenderer()
	{
		PublicIncludePaths.Add("Public");
		PrivateIncludePaths.Add("Private");

		PublicDependencyModuleNames.Add("Core");
		PublicDependencyModuleNames.Add("RenderCore");
		PublicDependencyModuleNames.Add("ApplicationCore");
		PublicDependencyModuleNames.Add("Engine");
	}
}
