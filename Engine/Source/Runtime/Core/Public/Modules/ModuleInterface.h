// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

// -------------------------------------------------------------
// IModuleInterface (REQ-012)
//
// Base interface for all engine and game modules.
// This is a header-only interface -- all methods have inline
// default implementations. No CORE_API needed; each module
// gets its own vtable copy.
//
// Each DLL module implements this interface and registers itself
// via the IMPLEMENT_MODULE or IMPLEMENT_GAME_MODULE macro.
//
// Lifecycle:
//   1. DLL loaded by ModuleManager
//   2. CreateModule() called (exported C function)
//   3. StartupModule() called
//   4. ... module active ...
//   5. ShutdownModule() called
//   6. Module instance destroyed
//   7. DLL unloaded
// -------------------------------------------------------------

namespace Enigma
{

class IModuleInterface
{
public:
    virtual ~IModuleInterface() = default;

    /// Called immediately after the module DLL is loaded and the
    /// module instance is created. Use this for initialization
    /// that depends on other modules being available.
    virtual void StartupModule() {}

    /// Called before the module is unloaded. Clean up resources,
    /// unregister delegates, etc.
    virtual void ShutdownModule() {}

    /// Whether this module supports dynamic reloading (hot-reload).
    /// Modules that hold persistent state may return false.
    virtual bool SupportsDynamicReloading() { return true; }

    /// Whether this is a game module (as opposed to an engine module).
    /// Game modules have different lifetime semantics.
    virtual bool IsGameModule() const { return false; }
};

/// Default minimal implementation for engine modules.
class FDefaultModuleImpl : public IModuleInterface
{
};

/// Default minimal implementation for game modules.
/// Mirrors Unreal Engine's FDefaultGameModuleImpl.
class FDefaultGameModuleImpl : public FDefaultModuleImpl
{
public:
    bool IsGameModule() const override { return true; }
};

} // namespace Enigma
