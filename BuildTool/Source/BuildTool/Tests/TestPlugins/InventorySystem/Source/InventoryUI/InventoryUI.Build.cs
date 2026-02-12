using EnigmaEngine;

public class InventoryUI : ModuleRules
{
    public InventoryUI(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencyModuleNames.Add("Core");
        PrivateDependencyModuleNames.Add("InventoryCore");
    }
}
