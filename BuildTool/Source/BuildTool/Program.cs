using BuildTool.Commands;
using BuildTool.Models;

namespace BuildTool;

/// <summary>
/// CLI entry point for EnigmaEngine BuildTool.
/// Dispatches commands: build, clean, rebuild.
/// Usage: BuildTool &lt;command&gt; &lt;project-path&gt; [options]
/// </summary>
public static class Program
{
    private static readonly Dictionary<string, ICommand> Commands = new(StringComparer.OrdinalIgnoreCase)
    {
        ["build"] = new BuildCommand(),
        ["clean"] = new CleanCommand(),
        ["rebuild"] = new RebuildCommand(),
        ["generate-project-files"] = new GenerateProjectFilesCommand(),
        ["package"] = new PackageCommand(),
        ["create-module"] = new CreateModuleCommand(),
        ["create-plugin"] = new CreatePluginCommand(),
        ["create-project"] = new CreateProjectCommand(),
        ["remove-module"] = new RemoveModuleCommand(),
        ["remove-plugin"] = new RemovePluginCommand(),
    };

    public static int Main(string[] args)
    {
        if (args.Length == 0 || args[0] is "-h" or "--help")
        {
            PrintUsage();
            return 0;
        }

        // Internal test runner: BuildTool test [suite-name]
        if (args[0] is "test")
        {
            return RunTests(args.Length > 1 ? args[1] : null);
        }

        var commandName = args[0];

        if (!Commands.TryGetValue(commandName, out var command))
        {
            Console.Error.WriteLine($"Error: Unknown command '{commandName}'.");
            Console.Error.WriteLine();
            PrintUsage();
            return 1;
        }

        var options = ParseOptions(args);
        if (options is null)
        {
            return 1;
        }

        Console.WriteLine($"EnigmaEngine BuildTool v0.1.0");
        Console.WriteLine($"Command: {command.Name}");
        Console.WriteLine();

        var result = command.Execute(options);

        if (result.Success)
        {
            Console.WriteLine();
            Console.WriteLine($"=== {result.Message} ===");
            return 0;
        }

        Console.Error.WriteLine();
        Console.Error.WriteLine($"Error: {result.Message}");
        if (result.ErrorDetail is not null)
        {
            Console.Error.WriteLine($"Detail: {result.ErrorDetail}");
        }
        return 1;
    }

    private static BuildOptions? ParseOptions(string[] args)
    {
        // args[0] = command, args[1..] = project path (positional) or options
        var commandName = args[0];
        var projectPath = ".";
        var configuration = BuildConfiguration.Development;
        var platform = "Win64";
        string? outputDirectory = null;
        int optionStart = 1;

        // For generate-project-files, project path is optional (auto-search)
        bool projectPathOptional = commandName.Equals("generate-project-files", StringComparison.OrdinalIgnoreCase)
            || commandName.Equals("create-project", StringComparison.OrdinalIgnoreCase);

        // Check for positional project path (non-option second argument)
        if (args.Length >= 2 && !args[1].StartsWith('-'))
        {
            projectPath = args[1];
            optionStart = 2;
        }

        // Parse named options
        bool hasProjectOption = false;
        var extraArguments = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        for (int i = optionStart; i < args.Length; i++)
        {
            switch (args[i])
            {
                case "-c" or "--config" when i + 1 < args.Length:
                    if (!Enum.TryParse<BuildConfiguration>(args[++i], ignoreCase: true, out configuration))
                    {
                        Console.Error.WriteLine($"Error: Invalid configuration '{args[i]}'.");
                        Console.Error.WriteLine("Valid values: Debug, DebugGame, Development, Shipping, Test");
                        return null;
                    }
                    break;

                case "-p" or "--platform" when i + 1 < args.Length:
                    platform = args[++i];
                    break;

                case "-o" or "--output" when i + 1 < args.Length:
                    outputDirectory = args[++i];
                    break;

                case "--project" when i + 1 < args.Length:
                    projectPath = args[++i];
                    hasProjectOption = true;
                    break;

                default:
                    // Capture --key value pairs for command-specific arguments
                    if (args[i].StartsWith("--") && i + 1 < args.Length)
                    {
                        var key = args[i][2..];
                        extraArguments[key] = args[++i];
                    }
                    else
                    {
                        Console.Error.WriteLine($"Warning: Unknown option '{args[i]}', ignoring.");
                    }
                    break;
            }
        }

        // Validate: project path must be provided (positionally or via --project)
        if (!hasProjectOption && optionStart == 1 && !projectPathOptional)
        {
            Console.Error.WriteLine("Error: Project path is required.");
            Console.Error.WriteLine("Usage: BuildTool <command> <project-path> [options]");
            return null;
        }

        return new BuildOptions
        {
            ProjectPath = projectPath,
            Configuration = configuration,
            Platform = platform,
            OutputDirectory = outputDirectory,
            ExtraArguments = extraArguments,
        };
    }

    private static void PrintUsage()
    {
        Console.WriteLine("EnigmaEngine BuildTool v0.1.0");
        Console.WriteLine();
        Console.WriteLine("Usage: BuildTool <command> <project-path> [options]");
        Console.WriteLine();
        Console.WriteLine("Commands:");
        foreach (var cmd in Commands.Values)
        {
            Console.WriteLine($"  {cmd.Name,-24} {cmd.Description}");
        }
        Console.WriteLine($"  {"test",-24} Run internal test suites.");
        Console.WriteLine();
        Console.WriteLine("Options:");
        Console.WriteLine("  -c, --config <config>    Build configuration (Debug|DebugGame|Development|Shipping|Test)");
        Console.WriteLine("  -p, --platform <plat>    Target platform (default: Win64)");
        Console.WriteLine("  -o, --output <path>      Output directory for package command");
        Console.WriteLine("  -h, --help               Show this help message");
    }

    /// <summary>
    /// Run internal test suites. If suiteName is null, runs all suites.
    /// </summary>
    private static int RunTests(string? suiteName)
    {
        var suites = new Dictionary<string, Action>(StringComparer.OrdinalIgnoreCase)
        {
            ["module-parser"] = Tests.ModuleParserTest.Run,
            ["dependency-resolver"] = Tests.DependencyResolverTest.Run,
            ["cmake-generator"] = Tests.CMakeGeneratorTest.Run,
            ["plugin-parser"] = Tests.PluginParserTest.Run,
            ["plugin-scanner"] = Tests.PluginScannerTest.Run,
            ["thirdparty-scanner"] = Tests.ThirdPartyScannerTest.Run,
            ["nlohmann-json-integration"] = Tests.NlohmannJsonIntegrationTest.Run,
            ["phase1-integration"] = Tests.Phase1IntegrationTest.Run,
            ["phase6-plugin-integration"] = Tests.Phase6PluginIntegrationTest.Run,
            ["manifest-generator"] = Tests.ManifestGeneratorTest.Run,
            ["shipping-packager"] = Tests.ShippingPackagerTest.Run,
            ["phase8-config-integration"] = Tests.Phase8ConfigIntegrationTest.Run,
            ["enigma-arcade-project"] = Tests.EnigmaArcadeProjectTest.Run,
            ["guid-generator"] = Tests.GuidGeneratorTest.Run,
            ["module-api-header-generator"] = Tests.ModuleApiHeaderGeneratorTest.Run,
            ["vcxproj-generator"] = Tests.VcxprojGeneratorTest.Run,
            ["solution-generator"] = Tests.SolutionGeneratorTest.Run,
            ["generate-project-files"] = Tests.GenerateProjectFilesCommandTest.Run,
            ["project-scanner"] = Tests.ProjectScannerTest.Run,
            ["cmake-invoker"] = Tests.CMakeInvokerTest.Run,
            ["post-build-step"] = Tests.PostBuildStepTest.Run,
            ["build-pipeline"] = Tests.BuildPipelineTest.Run,
            ["clean-command"] = Tests.CleanCommandTest.Run,
            ["build-integration"] = Tests.BuildIntegrationTest.Run,
            ["shipped-build-integration"] = Tests.ShippedBuildIntegrationTest.Run,
            ["package-command"] = Tests.PackageCommandTest.Run,
            ["name-validator"] = Tests.NameValidatorTest.Run,
            ["template-engine"] = Tests.TemplateEngineTest.Run,
            ["scaffolding-rollback"] = Tests.ScaffoldingRollbackTest.Run,
            ["eproject-modifier"] = Tests.EprojectModifierTest.Run,
            ["target-file-modifier"] = Tests.TargetFileModifierTest.Run,
            ["dependency-checker"] = Tests.DependencyCheckerTest.Run,
            ["create-module-command"] = Tests.CreateModuleCommandTest.Run,
            ["create-plugin-command"] = Tests.CreatePluginCommandTest.Run,
            ["create-project-command"] = Tests.CreateProjectCommandTest.Run,
            ["remove-module-command"] = Tests.RemoveModuleCommandTest.Run,
            ["remove-plugin-command"] = Tests.RemovePluginCommandTest.Run,
            ["scaffolding-integration"] = Tests.ScaffoldingIntegrationTest.Run,
            ["scaffolding-build-integration"] = Tests.ScaffoldingBuildIntegrationTest.Run,
        };

        try
        {
            if (suiteName is not null)
            {
                if (!suites.TryGetValue(suiteName, out var suite))
                {
                    Console.Error.WriteLine($"Error: Unknown test suite '{suiteName}'.");
                    Console.Error.WriteLine($"Available: {string.Join(", ", suites.Keys)}");
                    return 1;
                }
                suite();
            }
            else
            {
                foreach (var (name, suite) in suites)
                {
                    Console.WriteLine($"\n--- Running: {name} ---\n");
                    suite();
                }
            }
            return 0;
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"\nTest failure: {ex.Message}");
            return 1;
        }
    }
}
