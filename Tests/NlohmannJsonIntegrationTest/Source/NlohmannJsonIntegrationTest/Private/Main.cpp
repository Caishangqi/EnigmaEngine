// Copyright EnigmaEngine. All Rights Reserved.
//
// Task 5.3 Integration Test: nlohmann/json via BuildTool Pipeline
//
// This test validates the full BuildTool pipeline:
//   .eproject -> .Target.cs -> .Build.cs (with nlohmann_json dep)
//   -> DependencyResolver -> CMakeGenerator (INTERFACE target)
//   -> compile -> run -> verify JSON output
//
// Expected output (exact):
//   {"engine":"EnigmaEngine","phase":5,"features":["json","modules","thirdparty"],"config":{"version":"0.5.0","headerOnly":true}}

#include <nlohmann/json.hpp>
#include <iostream>
#include <string>

using json = nlohmann::json;

int main()
{
    json output;
    output["engine"] = "EnigmaEngine";
    output["phase"] = 5;
    output["features"] = {"json", "modules", "thirdparty"};
    output["config"]["version"] = "0.5.0";
    output["config"]["headerOnly"] = true;

    // dump(-1) produces compact single-line JSON
    std::cout << output.dump(-1) << std::endl;
    return 0;
}
