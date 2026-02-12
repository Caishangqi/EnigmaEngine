// Copyright EnigmaEngine. All Rights Reserved.
//
// Task 5.1 Test: nlohmann/json ThirdParty Integration
//
// Validates:
//   [1]  #include <nlohmann/json.hpp> compiles successfully
//   [2]  nlohmann::json object default-constructs as null
//   [3]  JSON object creation with operator[]
//   [4]  JSON array creation with push_back
//   [5]  JSON serialization to string (dump)
//   [6]  JSON deserialization from string (parse)
//   [7]  Nested object access
//   [8]  Type checking (is_string, is_number, is_array, is_object)
//   [9]  Value extraction with get<T>()
//   [10] Round-trip: serialize -> parse -> compare

#include <nlohmann/json.hpp>

#include <cstdio>
#include <cmath>
#include <string>

using json = nlohmann::json;

static int g_passed = 0;
static int g_failed = 0;

static void Check(bool cond, const char* name)
{
    if (cond) { std::printf("  [PASS] %s\n", name); ++g_passed; }
    else      { std::printf("  [FAIL] %s\n", name); ++g_failed; }
}

int main()
{
    std::printf("=== Task 5.1: nlohmann/json ThirdParty Integration Test ===\n\n");

    // [1] Include compiles
    Check(true, "[1]  #include <nlohmann/json.hpp> compiles successfully");

    // [2] Default construct is null
    json j;
    Check(j.is_null(), "[2]  Default-constructed json is null");

    // [3] Object creation
    json obj;
    obj["name"] = "EnigmaEngine";
    obj["version"] = 5;
    obj["active"] = true;
    Check(obj.is_object() && obj.size() == 3,
        "[3]  JSON object creation with operator[]");

    // [4] Array creation
    json arr = json::array();
    arr.push_back(1);
    arr.push_back("two");
    arr.push_back(3.0);
    Check(arr.is_array() && arr.size() == 3,
        "[4]  JSON array creation with push_back");

    // [5] Serialization
    std::string serialized = obj.dump();
    Check(!serialized.empty() && serialized.find("EnigmaEngine") != std::string::npos,
        "[5]  JSON serialization to string (dump)");

    // [6] Deserialization
    json parsed = json::parse(R"({"engine":"Enigma","phase":5})");
    Check(parsed.is_object() && parsed["phase"] == 5,
        "[6]  JSON deserialization from string (parse)");

    // [7] Nested object
    json nested;
    nested["config"]["render"]["width"] = 1920;
    nested["config"]["render"]["height"] = 1080;
    Check(nested["config"]["render"]["width"] == 1920
       && nested["config"]["render"]["height"] == 1080,
        "[7]  Nested object access");

    // [8] Type checking
    json mixed;
    mixed["str"] = "hello";
    mixed["num"] = 42;
    mixed["arr"] = json::array({1, 2, 3});
    mixed["obj"] = json::object({{"k", "v"}});
    Check(mixed["str"].is_string()
       && mixed["num"].is_number()
       && mixed["arr"].is_array()
       && mixed["obj"].is_object(),
        "[8]  Type checking (is_string, is_number, is_array, is_object)");

    // [9] Value extraction
    std::string s = mixed["str"].get<std::string>();
    int n = mixed["num"].get<int>();
    Check(s == "hello" && n == 42,
        "[9]  Value extraction with get<T>()");

    // [10] Round-trip
    json original;
    original["pi"] = 3.14159;
    original["items"] = {"a", "b", "c"};
    original["nested"]["flag"] = true;

    std::string wire = original.dump();
    json restored = json::parse(wire);
    Check(restored == original,
        "[10] Round-trip: serialize -> parse -> compare");

    // ---- Summary ----
    std::printf("\n=== %d/%d tests passed ===\n",
        g_passed, g_passed + g_failed);

    return g_failed > 0 ? 1 : 0;
}
