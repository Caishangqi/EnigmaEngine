using EnigmaEngine;

public class EnigmaGameTarget : TargetRules
{
    public EnigmaGameTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V1;

        // Engine entry point module
        ExtraModuleNames.Add("Launch");
    }
}
