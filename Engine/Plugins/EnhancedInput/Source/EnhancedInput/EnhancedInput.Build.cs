// EnhancedInput.Build.cs -- Build rules for the EnhancedInput plugin module.
// Provides Enhanced Input system: actions, mapping contexts, modifiers, triggers.

using EnigmaEngine;

public class EnhancedInput : ModuleRules
{
    public EnhancedInput(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencyModuleNames.Add("Core");
        PublicDependencyModuleNames.Add("Engine");
        PublicDependencyModuleNames.Add("ApplicationCore");
    }
}
