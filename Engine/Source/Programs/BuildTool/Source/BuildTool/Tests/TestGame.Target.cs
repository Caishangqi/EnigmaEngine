// EnigmaEngine BuildTool test file
using EnigmaBuildTool;

public class TestGameTarget : TargetRules
{
    public TestGameTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V2;
        ExtraModuleNames.Add("TestGame");
        ExtraModuleNames.Add("TestGameplay");
        ExtraModuleNames.Add("CoreUtils");
    }
}
