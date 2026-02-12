using EnigmaEngine;

public class DebugTools : ModuleRules
{
    public DebugTools(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicIncludePaths.Add("Public");

        PublicDependencyModuleNames.Add("Core");
        PublicDependencyModuleNames.Add("Engine");
    }
}
