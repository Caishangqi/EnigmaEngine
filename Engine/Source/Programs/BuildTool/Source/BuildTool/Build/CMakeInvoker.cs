// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Build;

using System.Diagnostics;

/// <summary>
/// Invokes CMake configure and build commands via System.Diagnostics.Process.
/// Resolves CMake executable path from PATH, then well-known installation locations.
/// All methods return ProcessResult - no exceptions are thrown on failure.
/// </summary>
public sealed class CMakeInvoker
{
    /// <summary>Result of a CMake process invocation.</summary>
    public sealed class ProcessResult
    {
        /// <summary>Process exit code (0 = success).</summary>
        public required int ExitCode { get; init; }

        /// <summary>Whether the process exited with code 0.</summary>
        public bool Success => ExitCode == 0;

        /// <summary>Combined stdout + stderr output.</summary>
        public string Output { get; init; } = string.Empty;

        /// <summary>Error message if the process could not be started.</summary>
        public string? Error { get; init; }

        public static ProcessResult Ok(string output) =>
            new() { ExitCode = 0, Output = output };

        public static ProcessResult Fail(int exitCode, string output, string? error = null) =>
            new() { ExitCode = exitCode, Output = output, Error = error };
    }

    private readonly string _cmakePath;

    /// <summary>Resolved path to the CMake executable.</summary>
    public string CmakePath => _cmakePath;

    /// <summary>
    /// Creates a new CMakeInvoker, resolving the CMake executable path.
    /// Searches PATH first, then well-known installation directories.
    /// </summary>
    /// <param name="cmakePathOverride">Optional explicit path to cmake executable.</param>
    public CMakeInvoker(string? cmakePathOverride = null)
    {
        _cmakePath = cmakePathOverride ?? ResolveCmakePath();
    }

    /// <summary>
    /// Run CMake configure: <c>cmake -S {sourceDir} -B {buildDir} -G {generator} [-D key=value ...]</c>.
    /// </summary>
    /// <param name="sourceDir">Path to the directory containing CMakeLists.txt.</param>
    /// <param name="buildDir">Path to the build output directory.</param>
    /// <param name="generator">CMake generator name (e.g. "Visual Studio 17 2022", "Ninja").</param>
    /// <param name="defines">Optional dictionary of -D cache entries.</param>
    /// <returns>ProcessResult with exit code and output.</returns>
    public ProcessResult Configure(string sourceDir, string buildDir,
        string generator, Dictionary<string, string>? defines = null)
    {
        var args = new List<string>
        {
            "-S", QuotePath(sourceDir),
            "-B", QuotePath(buildDir),
            "-G", QuotePath(generator),
        };

        if (defines is not null)
        {
            foreach (var (key, value) in defines)
                args.Add($"-D{key}={value}");
        }

        Console.WriteLine($"[CMakeInvoker] Configure: cmake {string.Join(' ', args)}");
        return RunProcess(_cmakePath, string.Join(' ', args), sourceDir);
    }

    /// <summary>
    /// Run CMake build: <c>cmake --build {buildDir} --config {config} [-j {parallelism}]</c>.
    /// </summary>
    /// <param name="buildDir">Path to the build directory (same as Configure's buildDir).</param>
    /// <param name="config">Build configuration (e.g. "Development", "DebugGame").</param>
    /// <param name="parallelism">Optional number of parallel jobs. 0 or null = CMake default.</param>
    /// <returns>ProcessResult with exit code and output.</returns>
    public ProcessResult Build(string buildDir, string config, int? parallelism = null)
    {
        var args = new List<string>
        {
            "--build", QuotePath(buildDir),
            "--config", config,
        };

        if (parallelism is > 0)
            args.Add($"-j {parallelism.Value}");

        Console.WriteLine($"[CMakeInvoker] Build: cmake {string.Join(' ', args)}");
        return RunProcess(_cmakePath, string.Join(' ', args), buildDir);
    }

    /// <summary>
    /// Run a process synchronously, forwarding stdout/stderr line-by-line in real-time.
    /// </summary>
    private static ProcessResult RunProcess(string fileName, string arguments, string workingDir)
    {
        var psi = new ProcessStartInfo
        {
            FileName = fileName,
            Arguments = arguments,
            WorkingDirectory = workingDir,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            UseShellExecute = false,
            CreateNoWindow = true,
        };

        Process proc;
        try
        {
            proc = Process.Start(psi)!;
        }
        catch (Exception ex)
        {
            return ProcessResult.Fail(-1, string.Empty,
                $"Failed to start process '{fileName}': {ex.Message}");
        }

        using (proc)
        {
            var output = new System.Text.StringBuilder();

            // Forward stdout line-by-line
            proc.OutputDataReceived += (_, e) =>
            {
                if (e.Data is not null)
                {
                    Console.WriteLine(e.Data);
                    lock (output) { output.AppendLine(e.Data); }
                }
            };

            // Forward stderr line-by-line
            proc.ErrorDataReceived += (_, e) =>
            {
                if (e.Data is not null)
                {
                    Console.Error.WriteLine(e.Data);
                    lock (output) { output.AppendLine(e.Data); }
                }
            };

            proc.BeginOutputReadLine();
            proc.BeginErrorReadLine();
            proc.WaitForExit();

            string outputStr = output.ToString();
            return proc.ExitCode == 0
                ? ProcessResult.Ok(outputStr)
                : ProcessResult.Fail(proc.ExitCode, outputStr);
        }
    }

    /// <summary>Quote a path if it contains spaces.</summary>
    private static string QuotePath(string path)
    {
        return path.Contains(' ') ? $"\"{path}\"" : path;
    }

    /// <summary>
    /// Resolve CMake executable path: search PATH first, then well-known locations.
    /// </summary>
    private static string ResolveCmakePath()
    {
        // 1. Try PATH via "where" (Windows) or "which" (Unix)
        string whichCmd = OperatingSystem.IsWindows() ? "where" : "which";
        try
        {
            var psi = new ProcessStartInfo
            {
                FileName = whichCmd,
                Arguments = "cmake",
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true,
            };

            using var proc = Process.Start(psi)!;
            string result = proc.StandardOutput.ReadLine() ?? string.Empty;
            proc.WaitForExit(5_000);

            if (proc.ExitCode == 0 && !string.IsNullOrWhiteSpace(result) && File.Exists(result.Trim()))
                return result.Trim();
        }
        catch { /* Fall through to well-known locations */ }

        // 2. Well-known installation directories
        string[] wellKnownPaths = OperatingSystem.IsWindows()
            ? [
                @"C:\Program Files\CMake\bin\cmake.exe",
                @"C:\Program Files (x86)\CMake\bin\cmake.exe",
                @"C:\Tools\CMake\bin\cmake.exe",
              ]
            : [
                "/usr/bin/cmake",
                "/usr/local/bin/cmake",
                "/opt/homebrew/bin/cmake",
                "/snap/bin/cmake",
              ];

        foreach (var path in wellKnownPaths)
        {
            if (File.Exists(path))
                return path;
        }

        // 3. Fallback - assume "cmake" is on PATH (will fail at invocation if not)
        Console.WriteLine("[CMakeInvoker] Warning: cmake not found in PATH or well-known locations. " +
            "Using 'cmake' and hoping for the best.");
        return "cmake";
    }
}
