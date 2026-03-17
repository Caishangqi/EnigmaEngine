using EnigmaEngine;

public class EnigmaArcadeTarget : TargetRules
{
    public EnigmaArcadeTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V1;

        // Primary module (same name as project, per REQ-018)
        ExtraModuleNames.Add("EnigmaArcade");

        // Runtime-loaded renderer implementation (loaded dynamically via FModuleManager)
        ExtraModuleNames.Add("AsciiRenderer");
    }
}
