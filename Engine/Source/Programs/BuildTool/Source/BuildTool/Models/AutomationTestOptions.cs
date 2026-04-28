namespace BuildTool.Models;

/// <summary>
/// Parsed options for the automation-test command.
/// </summary>
public sealed class AutomationTestOptions
{
    public required string RootPath { get; init; }

    public bool EngineMode { get; init; }

    public bool List { get; init; }

    public bool Run { get; init; }

    public bool GenerateIde { get; init; }

    public AutomationTestProfile Profile { get; init; } = AutomationTestProfile.LocalFast;

    public string? Name { get; init; }

    public string? NamePrefix { get; init; }

    public string? Module { get; init; }

    public IReadOnlyList<string> Tags { get; init; } = [];

    public string? ReportDirectory { get; init; }

    public bool AllowEmpty { get; init; }

    public string ActionName
    {
        get
        {
            if (List) return "list";
            if (Run) return "run";
            if (GenerateIde) return "generate-ide";
            return "none";
        }
    }

    public static bool TryCreate(
        BuildOptions options,
        out AutomationTestOptions? automationOptions,
        out string? error)
    {
        automationOptions = null;

        if (!TryGetFlag(options, "engine", out bool engineMode, out error) ||
            !TryGetFlag(options, "list", out bool list, out error) ||
            !TryGetFlag(options, "run", out bool run, out error) ||
            !TryGetFlag(options, "generate-ide", out bool generateIde, out error) ||
            !TryGetFlag(options, "allow-empty", out bool allowEmpty, out error))
        {
            return false;
        }

        int actionCount = (list ? 1 : 0) + (run ? 1 : 0) + (generateIde ? 1 : 0);
        if (actionCount == 0)
        {
            error = "One action is required: --list, --run, or --generate-ide.";
            return false;
        }

        if (actionCount > 1)
        {
            error = "Only one action can be specified: --list, --run, or --generate-ide.";
            return false;
        }

        string profileName = options.ExtraArguments.GetValueOrDefault("profile", AutomationTestProfiles.DefaultName);
        if (string.IsNullOrWhiteSpace(profileName))
        {
            error = $"--profile requires a value. Valid values: {AutomationTestProfiles.ValidValues}.";
            return false;
        }

        if (!AutomationTestProfiles.TryParse(profileName, out var profile))
        {
            error = $"Invalid --profile '{profileName}'. Valid values: {AutomationTestProfiles.ValidValues}.";
            return false;
        }

        if (!TryGetOptionalValue(options, "name", out string? name, out error) ||
            !TryGetOptionalValue(options, "name-prefix", out string? namePrefix, out error) ||
            !TryGetOptionalValue(options, "module", out string? module, out error) ||
            !TryGetOptionalValue(options, "report", out string? reportDirectory, out error))
        {
            return false;
        }

        var tags = ParseTags(options.ExtraArguments.GetValueOrDefault("tag"));
        if (options.ExtraArguments.TryGetValue("tag", out var rawTags) && tags.Count == 0)
        {
            error = "--tag requires at least one non-empty value.";
            return false;
        }

        automationOptions = new AutomationTestOptions
        {
            RootPath = options.ProjectPath,
            EngineMode = engineMode,
            List = list,
            Run = run,
            GenerateIde = generateIde,
            Profile = profile,
            Name = name,
            NamePrefix = namePrefix,
            Module = module,
            Tags = tags,
            ReportDirectory = reportDirectory,
            AllowEmpty = allowEmpty,
        };
        error = null;
        return true;
    }

    private static bool TryGetOptionalValue(
        BuildOptions options,
        string key,
        out string? value,
        out string? error)
    {
        value = null;
        error = null;

        if (!options.ExtraArguments.TryGetValue(key, out var rawValue))
        {
            return true;
        }

        if (string.IsNullOrWhiteSpace(rawValue))
        {
            error = $"--{key} requires a value.";
            return false;
        }

        value = rawValue.Trim();
        return true;
    }

    private static bool TryGetFlag(
        BuildOptions options,
        string key,
        out bool value,
        out string? error)
    {
        value = false;
        error = null;

        if (!options.ExtraArguments.TryGetValue(key, out var rawValue))
        {
            return true;
        }

        if (string.IsNullOrWhiteSpace(rawValue))
        {
            value = true;
            return true;
        }

        if (bool.TryParse(rawValue, out value))
        {
            return true;
        }

        error = $"--{key} is a flag. Use --{key}, --{key} true, or --{key} false.";
        return false;
    }

    private static List<string> ParseTags(string? rawValue)
    {
        if (string.IsNullOrWhiteSpace(rawValue))
        {
            return [];
        }

        return rawValue
            .Split([';', ','], StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries)
            .Where(tag => !string.IsNullOrWhiteSpace(tag))
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .ToList();
    }
}
