using EnigmaEngine;

public class ArcadeGameplay : ModuleRules
{
    public ArcadeGameplay(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencyModuleNames.Add("Core");
        PublicDependencyModuleNames.Add("Engine");
    }
}
