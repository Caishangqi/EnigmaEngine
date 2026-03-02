namespace BuildTool.Analysis;

/// <summary>
/// Detects circular dependencies in a directed graph using DFS.
/// Reports the full cycle path when a cycle is found.
/// </summary>
public static class CycleDetector
{
    /// <summary>
    /// Result of a cycle detection run.
    /// </summary>
    public sealed class CycleResult
    {
        /// <summary>Whether a cycle was detected.</summary>
        public bool HasCycle { get; init; }

        /// <summary>
        /// The cycle path if one was found (e.g. ["A", "B", "C", "A"]).
        /// Empty if no cycle exists.
        /// </summary>
        public IReadOnlyList<string> CyclePath { get; init; } = [];

        /// <summary>Human-readable cycle description.</summary>
        public string Description => HasCycle
            ? $"Circular dependency detected: {string.Join(" -> ", CyclePath)}"
            : "No circular dependencies found.";

        public static CycleResult None => new() { HasCycle = false };

        public static CycleResult Found(List<string> path) =>
            new() { HasCycle = true, CyclePath = path };
    }

    /// <summary>
    /// Check for cycles in a directed graph represented as an adjacency list.
    /// </summary>
    /// <param name="adjacency">
    /// Map from node name to its direct dependencies (outgoing edges).
    /// All nodes must appear as keys, even if they have no dependencies.
    /// </param>
    /// <returns>A <see cref="CycleResult"/> indicating whether a cycle exists.</returns>
    public static CycleResult Detect(IReadOnlyDictionary<string, List<string>> adjacency)
    {
        // Node states for DFS: 0 = unvisited, 1 = in current path, 2 = fully processed
        var state = new Dictionary<string, int>(adjacency.Count);
        foreach (var node in adjacency.Keys)
        {
            state[node] = 0;
        }

        // Parent tracking for path reconstruction
        var pathStack = new List<string>();

        foreach (var node in adjacency.Keys)
        {
            if (state[node] == 0)
            {
                var cycle = Dfs(node, adjacency, state, pathStack);
                if (cycle is not null)
                {
                    return CycleResult.Found(cycle);
                }
            }
        }

        return CycleResult.None;
    }

    /// <summary>
    /// DFS traversal that returns the cycle path if one is found, or null otherwise.
    /// </summary>
    private static List<string>? Dfs(
        string node,
        IReadOnlyDictionary<string, List<string>> adjacency,
        Dictionary<string, int> state,
        List<string> pathStack)
    {
        state[node] = 1; // Mark as in-progress
        pathStack.Add(node);

        if (adjacency.TryGetValue(node, out var neighbors))
        {
            foreach (var neighbor in neighbors)
            {
                if (!state.ContainsKey(neighbor))
                {
                    // Unknown node (external dependency not in graph) - skip
                    continue;
                }

                if (state[neighbor] == 1)
                {
                    // Found a back edge - extract cycle path
                    return ExtractCyclePath(pathStack, neighbor);
                }

                if (state[neighbor] == 0)
                {
                    var cycle = Dfs(neighbor, adjacency, state, pathStack);
                    if (cycle is not null)
                    {
                        return cycle;
                    }
                }
            }
        }

        pathStack.RemoveAt(pathStack.Count - 1);
        state[node] = 2; // Mark as fully processed
        return null;
    }

    /// <summary>
    /// Extract the cycle path from the DFS stack.
    /// Returns a list starting and ending with the cycle entry node.
    /// </summary>
    private static List<string> ExtractCyclePath(List<string> pathStack, string cycleStart)
    {
        var cycleIndex = pathStack.IndexOf(cycleStart);
        var cyclePath = new List<string>(pathStack.Count - cycleIndex + 1);
        for (int i = cycleIndex; i < pathStack.Count; i++)
        {
            cyclePath.Add(pathStack[i]);
        }
        cyclePath.Add(cycleStart); // Close the cycle
        return cyclePath;
    }
}
