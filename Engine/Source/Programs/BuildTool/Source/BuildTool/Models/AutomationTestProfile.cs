namespace BuildTool.Models;

/// <summary>
/// Named automation test execution profiles.
/// </summary>
public enum AutomationTestProfile
{
    /// <summary>Fast local loop: unit and smoke tests only.</summary>
    LocalFast,

    /// <summary>Standard CI profile: unit, smoke, and selected integration tests.</summary>
    CiStandard,

    /// <summary>All non-performance tests.</summary>
    AllNonPerf,

    /// <summary>Performance tests only.</summary>
    Perf
}

/// <summary>
/// Helpers for parsing and printing automation test profile names.
/// </summary>
public static class AutomationTestProfiles
{
    public const string DefaultName = "local-fast";

    public static string ValidValues => "local-fast, ci-standard, all-non-perf, perf";

    public static bool TryParse(string value, out AutomationTestProfile profile)
    {
        switch (value.Trim().ToLowerInvariant())
        {
            case "local-fast":
                profile = AutomationTestProfile.LocalFast;
                return true;
            case "ci-standard":
                profile = AutomationTestProfile.CiStandard;
                return true;
            case "all-non-perf":
                profile = AutomationTestProfile.AllNonPerf;
                return true;
            case "perf":
                profile = AutomationTestProfile.Perf;
                return true;
            default:
                profile = AutomationTestProfile.LocalFast;
                return false;
        }
    }

    public static string ToCliName(AutomationTestProfile profile) =>
        profile switch
        {
            AutomationTestProfile.LocalFast => "local-fast",
            AutomationTestProfile.CiStandard => "ci-standard",
            AutomationTestProfile.AllNonPerf => "all-non-perf",
            AutomationTestProfile.Perf => "perf",
            _ => throw new ArgumentOutOfRangeException(nameof(profile), profile, null),
        };
}
