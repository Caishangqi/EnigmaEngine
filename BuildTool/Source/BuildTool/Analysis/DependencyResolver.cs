using BuildTool.Models;

namespace BuildTool.Analysis;

/// <summary>
/// Resolves module dependencies and produces a topologically sorted build order.
/// Combines both Public and Private dependencies into a unified dependency graph.
/// </summary>
public sealed class DependencyResolver
{
    /// <summary>
    /// Result of dependency resolution.
    /// </summary>
    public sealed class ResolveResult
    {
        /// <summary>Whether resolution succeeded (no cycles, all deps resolved).</summary>
        public required bool Success { get; init; }

        /// <summary>
        /// Modules in topological build order (dependencies before dependents).
        /// Empty if resolution failed.
        /// </summary>
        public IReadOnlyList<string> BuildOrder { get; init; } = [];

        /// <summary>Error message if resolution failed.</summary>
        public string? Error { get; init; }

        /// <summary>Cycle detection result (if a cycle was found).</summary>
        public CycleDetector.CycleResult? CycleInfo { get; init; }

        /// <summary>The full adjacency list used for resolution.</summary>
        public IReadOnlyDictionary<string, List<string>> AdjacencyList { get; init; }
            = new Dictionary<string, List<string>>();
    }

    /// <summary>
    /// Build a dependency graph from parsed module rules and resolve build order.
    /// </summary>
    /// <param name="modules">All parsed module rules (keyed by module name).</param>
    /// <returns>A <see cref="ResolveResult"/> with build order or error info.</returns>
    public ResolveResult Resolve(IReadOnlyDictionary<string, ModuleRules> modules)
    {
        // Step 1: Build adjacency list (module -> its dependencies)
        var adjacency = BuildAdjacencyList(modules);

        // Step 2: Check for cycles
        var cycleResult = CycleDetector.Detect(adjacency);
        if (cycleResult.HasCycle)
        {
            return new ResolveResult
            {
                Success = false,
                Error = cycleResult.Description,
                CycleInfo = cycleResult,
                AdjacencyList = adjacency,
            };
        }

        // Step 3: Topological sort (Kahn's algorithm)
        var buildOrder = TopologicalSort(adjacency);

        return new ResolveResult
        {
            Success = true,
            BuildOrder = buildOrder,
            AdjacencyList = adjacency,
        };
    }

    /// <summary>
    /// Build an adjacency list from module rules.
    /// Merges Public and Private dependencies into a single edge list per module.
    /// </summary>
    private static Dictionary<string, List<string>> BuildAdjacencyList(
        IReadOnlyDictionary<string, ModuleRules> modules)
    {
        var adjacency = new Dictionary<string, List<string>>(modules.Count, StringComparer.Ordinal);

        // Ensure every module appears as a key (even with no dependencies)
        foreach (var (name, _) in modules)
        {
            adjacency[name] = [];
        }

        // Add edges: module depends on its Public + Private dependencies
        foreach (var (name, rules) in modules)
        {
            var deps = adjacency[name];

            foreach (var dep in rules.PublicDependencyModuleNames)
            {
                if (adjacency.ContainsKey(dep) && !deps.Contains(dep))
                {
                    deps.Add(dep);
                }
            }

            foreach (var dep in rules.PrivateDependencyModuleNames)
            {
                if (adjacency.ContainsKey(dep) && !deps.Contains(dep))
                {
                    deps.Add(dep);
                }
            }
        }

        return adjacency;
    }

    /// <summary>
    /// Kahn's algorithm for topological sort.
    /// Returns modules in build order (dependencies first).
    /// The adjacency list maps node -> [its dependencies].
    /// We reverse edges to get dep -> [dependents] for proper ordering.
    /// Assumes no cycles (caller must check first).
    /// </summary>
    private static List<string> TopologicalSort(Dictionary<string, List<string>> adjacency)
    {
        // Build reverse graph: dep -> [nodes that depend on it]
        // And compute in-degree as "number of dependencies a node has"
        var inDegree = new Dictionary<string, int>(adjacency.Count, StringComparer.Ordinal);
        var reversedAdj = new Dictionary<string, List<string>>(adjacency.Count, StringComparer.Ordinal);

        foreach (var node in adjacency.Keys)
        {
            inDegree[node] = 0;
            reversedAdj[node] = [];
        }

        foreach (var (node, deps) in adjacency)
        {
            inDegree[node] = deps.Count(d => adjacency.ContainsKey(d));
            foreach (var dep in deps)
            {
                if (reversedAdj.ContainsKey(dep))
                {
                    reversedAdj[dep].Add(node);
                }
            }
        }

        // Start with nodes that have zero dependencies (leaf modules like Core)
        var queue = new Queue<string>();
        foreach (var (node, degree) in inDegree)
        {
            if (degree == 0)
            {
                queue.Enqueue(node);
            }
        }

        var result = new List<string>(adjacency.Count);

        while (queue.Count > 0)
        {
            var node = queue.Dequeue();
            result.Add(node);

            // For each node that depends on the current node, decrement its in-degree
            foreach (var dependent in reversedAdj[node])
            {
                if (!inDegree.ContainsKey(dependent))
                    continue;

                inDegree[dependent]--;
                if (inDegree[dependent] == 0)
                {
                    queue.Enqueue(dependent);
                }
            }
        }

        return result;
    }
}
