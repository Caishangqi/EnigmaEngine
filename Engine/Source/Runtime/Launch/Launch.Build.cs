// Copyright EnigmaEngine. All Rights Reserved.

public class Launch : ModuleRules
{
    public Launch()
    {
        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencyModuleNames.Add("Core");
        PublicDependencyModuleNames.Add("Engine");
    }
}
