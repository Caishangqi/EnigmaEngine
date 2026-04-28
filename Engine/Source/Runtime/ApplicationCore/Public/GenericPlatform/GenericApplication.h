// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "ApplicationCoreAPI.generated.h"

#include "GenericPlatform/GenericWindowDefinition.h"

#include <cstdint>

// -------------------------------------------------------------
// FGenericApplication
//
// Platform-agnostic application abstraction owning the OS
// message pump and window factory. Concrete implementations
// (e.g. FWindowsApplication) override virtual methods.
//
// Lifetime:
//   FGenericApplication::CreateApplication()  -- in PreInit
//   FGenericApplication::GetApplication()     -- global accessor
//   delete app                                -- in Exit
// -------------------------------------------------------------

namespace Enigma
{

class FGenericWindow;
class FGenericApplicationMessageHandler;

/// Abstract application interface.
class APPLICATIONCORE_API FGenericApplication
{
public:
    virtual ~FGenericApplication();

    /// Process pending OS messages for this frame.
    virtual void PumpMessages(float deltaTime) {}

    /// Process deferred events (called after PumpMessages).
    virtual void ProcessDeferredEvents(float deltaTime) {}

    /// Create a new platform window.
    virtual FGenericWindow* MakeWindow(const FWindowDefinition& definition) = 0;

    /// Destroy a window and release OS resources.
    virtual void DestroyWindow(FGenericWindow* window) = 0;

    /// Set the message handler for input/window events.
    void SetMessageHandler(FGenericApplicationMessageHandler* handler);

    /// Get the current message handler.
    FGenericApplicationMessageHandler* GetMessageHandler() const;

    /// Static factory: creates the correct platform Application.
    static FGenericApplication* CreateApplication();

    /// Global accessor.
    static FGenericApplication* GetApplication();

protected:
    FGenericApplicationMessageHandler* MessageHandler = nullptr;

private:
    static FGenericApplication* s_application;
};

} // namespace Enigma
