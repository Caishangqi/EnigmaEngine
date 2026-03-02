// Minimal target file with no ExtraModuleNames
using EnigmaBuildTool;

public class EmptyTarget : TargetRules
{
    public EmptyTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
    }
}
