// Core.Build.cs -- Build rules for the Core module.
// Core is the lowest-level engine module with zero external dependencies.

using EnigmaEngine;

public class Core : ModuleRules
{
    public Core()
    {
        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        // Core has no module dependencies -- it is the foundation.
    }
}
