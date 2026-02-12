// nlohmann_json.Build.cs -- Build rules for nlohmann/json (header-only).
// JSON for Modern C++ v3.12.0 -- https://github.com/nlohmann/json
// License: MIT (see LICENSE.MIT in this directory)
//
// This is a header-only ThirdParty module. No DLL is produced.
// Dependents simply add "nlohmann_json" to their dependency list
// and can then #include <nlohmann/json.hpp>.

using EnigmaEngine;

public class nlohmann_json : ModuleRules
{
    public nlohmann_json()
    {
        // Header-only: expose the include/ directory so dependents
        // can write #include <nlohmann/json.hpp>
        PublicIncludePaths.Add("include");
    }
}
