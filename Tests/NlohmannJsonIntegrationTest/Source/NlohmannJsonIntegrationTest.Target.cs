using EnigmaEngine;

public class NlohmannJsonIntegrationTestTarget : TargetRules
{
    public NlohmannJsonIntegrationTestTarget()
    {
        Type = TargetType.Game;
        DefaultBuildSettings = "V1";
        ExtraModuleNames.Add("NlohmannJsonIntegrationTest");
    }
}
