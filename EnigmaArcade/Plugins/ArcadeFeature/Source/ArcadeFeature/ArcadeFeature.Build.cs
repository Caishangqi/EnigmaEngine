using EnigmaEngine;

public class ArcadeFeature : ModuleRules
{
    public ArcadeFeature(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencyModuleNames.Add("Core");
        PublicDependencyModuleNames.Add("Engine");
    }
}
