// EnigmaEngine BuildTool test file
using EnigmaBuildTool;

public class TestGame : ModuleRules
{
    public TestGame(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "Engine"
            }
        );

        PublicDependencyModuleNames.Add("InputCore");

        PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        DynamicallyLoadedModuleNames.Add("OnlineSubsystem");

        // PrivateDependencyModuleNames.Add("CommentedOut");
    }
}
