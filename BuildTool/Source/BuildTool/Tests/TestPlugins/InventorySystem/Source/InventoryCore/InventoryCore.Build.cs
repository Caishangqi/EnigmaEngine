using EnigmaEngine;

public class InventoryCore : ModuleRules
{
    public InventoryCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencyModuleNames.Add("Core");
    }
}
