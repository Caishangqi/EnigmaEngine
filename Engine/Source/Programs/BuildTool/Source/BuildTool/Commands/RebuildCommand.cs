using BuildTool.Models;

namespace BuildTool.Commands;

/// <summary>
/// Performs a clean followed by a full build.
/// </summary>
public sealed class RebuildCommand : ICommand
{
    private readonly CleanCommand _cleanCommand = new();
    private readonly BuildCommand _buildCommand = new();

    public string Name => "rebuild";
    public string Description => "Clean and rebuild the specified project.";

    public BuildResult Execute(BuildOptions options)
    {
        Console.WriteLine($"[Rebuild] Project: {options.ProjectPath}");

        var cleanResult = _cleanCommand.Execute(options);
        if (!cleanResult.Success)
        {
            return BuildResult.Fail("Rebuild aborted: clean step failed.", cleanResult.ErrorDetail);
        }

        return _buildCommand.Execute(options);
    }
}
