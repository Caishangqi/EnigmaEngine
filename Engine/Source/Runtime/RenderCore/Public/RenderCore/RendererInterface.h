// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file RendererInterface.h
/// @brief Generic renderer module interface - lifecycle only.
/// Implementations: FAsciiRendererModule (ASCII), future FDX12RendererModule.
/// Draw commands live on renderer-specific sub-interfaces.

#include "RenderCoreAPI.generated.h"
#include "Modules/ModuleInterface.h"

#include <cstdint>

namespace Enigma
{

// Forward declarations
struct FSceneView;
class FGenericWindow;

/// Generic renderer module interface - lifecycle only.
/// Implementations: FAsciiRendererModule (ASCII), future FDX12RendererModule.
/// Draw commands live on renderer-specific sub-interfaces (IAsciiRendererModule, etc.).
class IRendererModule : public IModuleInterface
{
public:
	// --- Frame lifecycle ---

	/// Begin a new rendering frame. Clears buffers and resets state.
	virtual void BeginFrame() = 0;

	/// End the current frame. Rasterizes and presents to screen.
	virtual void EndFrame() = 0;

	// --- Initialization ---

	/// Bind the renderer to a window for output.
	virtual void Initialize(FGenericWindow* renderTarget) = 0;

	// --- View ---

	/// Set the active camera/scene view for the current frame.
	virtual void SetActiveView(const FSceneView& view) = 0;

	// --- Queries ---

	/// Returns the width of the frame buffer in cells/pixels.
	virtual int32_t GetFrameBufferWidth() const = 0;

	/// Returns the height of the frame buffer in cells/pixels.
	virtual int32_t GetFrameBufferHeight() const = 0;
};

} // namespace Enigma
