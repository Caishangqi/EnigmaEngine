using EnigmaEngine;

public class EnigmaArcade : ModuleRules
{
    public EnigmaArcade(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencyModuleNames.Add("Core");
        PublicDependencyModuleNames.Add("Engine");
        PublicDependencyModuleNames.Add("ApplicationCore");
        PublicDependencyModuleNames.Add("RenderCore");

        PrivateDependencyModuleNames.Add("nlohmann_json");
        PrivateDependencyModuleNames.Add("ArcadeGameplay");
    }
}
