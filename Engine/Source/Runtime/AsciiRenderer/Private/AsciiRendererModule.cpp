// Copyright EnigmaEngine. All Rights Reserved.

/// @file AsciiRendererModule.cpp
/// @brief FAsciiRendererModule - concrete IAsciiRendererModule implementation.
/// Owns back buffer, rasterizer, render backend, scene view, and blend state.

#include "RenderCore/AsciiRendererInterface.h"
#include "RenderCore/AsciiCell.h"
#include "RenderCore/AsciiSprite.h"
#include "AsciiRenderer/AsciiRenderBackend.h"
#include "SceneView/SceneView.h"
#include "GenericPlatform/GenericWindow.h"
#include "Modules/ModuleMacros.h"
#include "Misc/AssertionMacros.h"

#include "AsciiBackBuffer.h"
#include "AsciiRasterizer.h"
#include "VTConsoleBackend.h"
#include "ClassicConsoleBackend.h"

#include <memory>
#include <cstdio>

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#undef DrawText
#endif

namespace Enigma
{

/// Concrete ASCII renderer module.
/// Registered as "Renderer" via IMPLEMENT_MODULE so GetRendererModule() finds it.
class FAsciiRendererModule : public IAsciiRendererModule
{
public:
	// --- IModuleInterface ---
	void StartupModule() override {}
	void ShutdownModule() override;

	// --- IRendererModule (generic lifecycle) ---
	void Initialize(FGenericWindow* renderTarget) override;
	void BeginFrame() override;
	void EndFrame() override;
	void SetActiveView(const FSceneView& view) override;
	int32_t GetFrameBufferWidth() const override;
	int32_t GetFrameBufferHeight() const override;

	// --- IAsciiRendererModule (ASCII draw + blend) ---
	void DrawCell(int32_t worldX, int32_t worldY, int32_t zOrder,
	              FAsciiCell cell) override;
	void DrawSprite(int32_t worldX, int32_t worldY, int32_t zOrder,
	                const FAsciiSprite& sprite) override;
	void DrawText(int32_t worldX, int32_t worldY, int32_t zOrder,
	              const char* text, FColor fg, FColor bg) override;
	void FillRect(int32_t worldX, int32_t worldY, int32_t width,
	              int32_t height, int32_t zOrder, FAsciiCell cell) override;
	void DrawBox(int32_t worldX, int32_t worldY, int32_t width,
	             int32_t height, int32_t zOrder, FColor fg, FColor bg) override;
	void SetBlendState(const FAsciiBlendState& state) override;

private:
	/// Create and initialize the rendering backend (Auto/Classic/VT).
	void createBackend(EAsciiRenderBackendType type, void* consoleOutputHandle);

	/// Flip world Y (Y-up) to screen Y (Y-down) for single-cell commands.
	int32_t flipY(int32_t worldY) const
	{
		return m_backBuffer ? (m_backBuffer->GetHeight() - 1 - worldY) : worldY;
	}

	/// Flip world Y for multi-row commands (height > 1).
	/// In Y-up convention, (x, y) is the bottom-left; this converts to
	/// screen-space top-left for the rasterizer.
	int32_t flipY(int32_t worldY, int32_t height) const
	{
		return m_backBuffer ? (m_backBuffer->GetHeight() - worldY - height) : worldY;
	}

	FGenericWindow*                       m_renderTarget = nullptr;
	std::unique_ptr<FAsciiBackBuffer>     m_backBuffer;
	std::unique_ptr<FRasterizer>          m_rasterizer;
	std::unique_ptr<IAsciiRenderBackend>  m_backend;
	FSceneView                            m_activeView;
	FAsciiBlendState                      m_blendState = FAsciiBlendState::Opaque();
	bool                                  m_bInitialized = false;
	bool                                  m_bFrameInProgress = false;
#ifdef _WIN32
	HANDLE                                m_screenBuffer = INVALID_HANDLE_VALUE;
	bool                                  m_bOwnConsole = false;
	bool                                  m_bDedicatedBuffer = false;
#endif
};

// ---------------------------------------------------------------
// IModuleInterface
// ---------------------------------------------------------------
void FAsciiRendererModule::ShutdownModule()
{
	if (m_backend)
	{
		m_backend->Shutdown();
		m_backend.reset();
	}
	m_rasterizer.reset();
	m_backBuffer.reset();
	m_renderTarget = nullptr;
	m_bInitialized = false;
	m_bFrameInProgress = false;

#ifdef _WIN32
	if (m_screenBuffer != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(m_screenBuffer);
		m_screenBuffer = INVALID_HANDLE_VALUE;
	}
	m_bDedicatedBuffer = false;
	if (m_bOwnConsole)
	{
		::FreeConsole();
		m_bOwnConsole = false;
	}
#endif
}

// ---------------------------------------------------------------
// IRendererModule - Initialize
// ---------------------------------------------------------------
void FAsciiRendererModule::Initialize(FGenericWindow* renderTarget)
{
	checkf(renderTarget != nullptr,
		"FAsciiRendererModule::Initialize called with null renderTarget");

	m_renderTarget = renderTarget;

	// Allocate back buffer from window dimensions.
	m_backBuffer = std::make_unique<FAsciiBackBuffer>();
	m_backBuffer->Allocate(renderTarget->GetWidth(), renderTarget->GetHeight());

	// Create rasterizer.
	m_rasterizer = std::make_unique<FRasterizer>();

	// Create backend with Auto detection.
	// The renderer uses its own console screen buffer, completely separate
	// from stdout/stderr so ENIGMA_LOG output never interferes with rendering.
#ifdef _WIN32
	// Ensure the process has a console.  When launched from an IDE the
	// process stdout is a pipe - AllocConsole creates a real console window.
	if (::GetConsoleWindow() == nullptr)
	{
		::AllocConsole();
		m_bOwnConsole = true;
	}

	SHORT w = static_cast<SHORT>(renderTarget->GetWidth());
	SHORT h = static_cast<SHORT>(renderTarget->GetHeight());

	// Strategy 1: Dedicated screen buffer (legacy conhost.exe).
	// Cleanly separates renderer output from stdout/log output.
	// Windows Terminal does NOT support SetConsoleActiveScreenBuffer,
	// so this path only succeeds under the legacy console host.
	m_screenBuffer = ::CreateConsoleScreenBuffer(
		GENERIC_READ | GENERIC_WRITE,
		0,        // not shared
		nullptr,  // default security
		CONSOLE_TEXTMODE_BUFFER,
		nullptr);

	if (m_screenBuffer != INVALID_HANDLE_VALUE)
	{
		// Copy the font from the original console buffer so the dedicated
		// buffer has the same cell size (and therefore physical window size).
		{
			HANDLE origOutput = ::CreateFileA("CONOUT$",
				GENERIC_READ, FILE_SHARE_WRITE,
				nullptr, OPEN_EXISTING, 0, nullptr);
			if (origOutput != INVALID_HANDLE_VALUE)
			{
				CONSOLE_FONT_INFOEX fontInfo = {};
				fontInfo.cbSize = sizeof(CONSOLE_FONT_INFOEX);
				if (::GetCurrentConsoleFontEx(origOutput, FALSE, &fontInfo))
				{
					::SetCurrentConsoleFontEx(m_screenBuffer, FALSE, &fontInfo);
				}
				::CloseHandle(origOutput);
			}
		}

		// Size the buffer and visible window to match the render target.
		SMALL_RECT minWin = { 0, 0, 0, 0 };
		::SetConsoleWindowInfo(m_screenBuffer, TRUE, &minWin);

		COORD bufSize = { w, h };
		::SetConsoleScreenBufferSize(m_screenBuffer, bufSize);

		SMALL_RECT winRect = { 0, 0,
			static_cast<SHORT>(w - 1), static_cast<SHORT>(h - 1) };
		::SetConsoleWindowInfo(m_screenBuffer, TRUE, &winRect);

		if (::SetConsoleActiveScreenBuffer(m_screenBuffer))
		{
			m_bDedicatedBuffer = true;
		}
		else
		{
			// Windows Terminal - SetConsoleActiveScreenBuffer unsupported.
			::CloseHandle(m_screenBuffer);
			m_screenBuffer = INVALID_HANDLE_VALUE;
		}
	}

	// Strategy 2: Fall back to the active console output (Windows Terminal).
	// Open CONOUT$ directly and redirect stdout/stderr to NUL so that
	// printf/log output does not corrupt the rendered frame.
	if (!m_bDedicatedBuffer)
	{
		m_screenBuffer = ::CreateFileA("CONOUT$",
			GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE,
			nullptr, OPEN_EXISTING, 0, nullptr);
		checkf(m_screenBuffer != INVALID_HANDLE_VALUE,
			"Failed to open CONOUT$ for renderer fallback");

		// Suppress log output that would overwrite rendered cells.
		std::freopen("NUL", "w", stdout);
		std::freopen("NUL", "w", stderr);

		// Clear the screen to erase any printf output that was written
		// before the renderer took over (module loading, config init, etc.).
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		if (::GetConsoleScreenBufferInfo(m_screenBuffer, &csbi))
		{
			DWORD total = static_cast<DWORD>(csbi.dwSize.X) * csbi.dwSize.Y;
			COORD origin = { 0, 0 };
			DWORD written = 0;
			::FillConsoleOutputCharacterA(m_screenBuffer, ' ', total, origin, &written);
			::FillConsoleOutputAttribute(m_screenBuffer, csbi.wAttributes, total, origin, &written);
			::SetConsoleCursorPosition(m_screenBuffer, origin);
		}
	}

	void* consoleOutputHandle = static_cast<void*>(m_screenBuffer);
	createBackend(EAsciiRenderBackendType::Auto, consoleOutputHandle);
#else
	createBackend(EAsciiRenderBackendType::Classic, nullptr);
#endif

	m_bInitialized = true;
}

void FAsciiRendererModule::createBackend(
	EAsciiRenderBackendType type, void* consoleOutputHandle)
{
	if (type == EAsciiRenderBackendType::Auto)
	{
		// Try VT first - if initialization succeeds, VT is supported.
		auto vtBackend = std::make_unique<FVTConsoleBackend>();
		if (vtBackend->Initialize(consoleOutputHandle))
		{
			m_backend = std::move(vtBackend);
			return;
		}
		// Fallback to Classic.
		type = EAsciiRenderBackendType::Classic;
	}

	if (type == EAsciiRenderBackendType::VT)
	{
		auto vtBackend = std::make_unique<FVTConsoleBackend>();
		checkf(vtBackend->Initialize(consoleOutputHandle),
			"FVTConsoleBackend::Initialize failed");
		m_backend = std::move(vtBackend);
	}
	else // Classic
	{
		auto classicBackend = std::make_unique<FClassicConsoleBackend>();
		checkf(classicBackend->Initialize(consoleOutputHandle),
			"FClassicConsoleBackend::Initialize failed");
		m_backend = std::move(classicBackend);
	}
}

// ---------------------------------------------------------------
// IRendererModule - Frame lifecycle
// ---------------------------------------------------------------
void FAsciiRendererModule::BeginFrame()
{
	ensure(!m_bFrameInProgress);
	m_bFrameInProgress = true;

	// Clear back buffer and draw list for the new frame.
	if (m_backBuffer)
	{
		m_backBuffer->Clear();
	}
	if (m_rasterizer)
	{
		m_rasterizer->Clear();
	}

	// Reset blend state to default opaque.
	m_blendState = FAsciiBlendState::Opaque();
}

void FAsciiRendererModule::EndFrame()
{
	m_bFrameInProgress = false;

	// Rasterize draw list into back buffer.
	if (m_rasterizer && m_backBuffer)
	{
		m_rasterizer->Rasterize(m_activeView, *m_backBuffer);
	}

	// Present back buffer via backend.
	if (m_backend && m_backBuffer)
	{
		m_backend->Present(m_backBuffer->GetData(),
		                   m_backBuffer->GetWidth(),
		                   m_backBuffer->GetHeight());
	}
}

void FAsciiRendererModule::SetActiveView(const FSceneView& view)
{
	m_activeView = view;

	// Convert camera Y from world-space (Y-up) to screen-space (Y-down).
	// In Y-up: camera moving up (+Y) should shift objects down on screen.
	// The rasterizer subtracts camera position, so negating Y achieves this.
	auto translation = m_activeView.ViewTransform.GetTranslation();
	translation.Y = -translation.Y;
	m_activeView.ViewTransform.SetTranslation(translation);
}

int32_t FAsciiRendererModule::GetFrameBufferWidth() const
{
	return m_backBuffer ? m_backBuffer->GetWidth() : 0;
}

int32_t FAsciiRendererModule::GetFrameBufferHeight() const
{
	return m_backBuffer ? m_backBuffer->GetHeight() : 0;
}

// ---------------------------------------------------------------
// IAsciiRendererModule - Draw commands
//
// Public API uses Y-up world coordinates (right=+X, up=+Y).
// Internally, the rasterizer uses screen coordinates (Y-down).
// Each draw method flips Y before submitting to the rasterizer.
// ---------------------------------------------------------------
void FAsciiRendererModule::DrawCell(
	int32_t worldX, int32_t worldY, int32_t zOrder, FAsciiCell cell)
{
	if (!m_rasterizer) return;

	FDrawCommand cmd;
	cmd.Type = EDrawCommandType::Cell;
	cmd.WorldX = worldX;
	cmd.WorldY = flipY(worldY);
	cmd.ZOrder = zOrder;
	cmd.BlendState = m_blendState;
	cmd.Cell = cell;
	m_rasterizer->AddCommand(std::move(cmd));
}

void FAsciiRendererModule::DrawSprite(
	int32_t worldX, int32_t worldY, int32_t zOrder,
	const FAsciiSprite& sprite)
{
	if (!m_rasterizer) return;

	FDrawCommand cmd;
	cmd.Type = EDrawCommandType::Sprite;
	cmd.WorldX = worldX;
	cmd.WorldY = flipY(worldY, sprite.Height);
	cmd.ZOrder = zOrder;
	cmd.BlendState = m_blendState;
	cmd.Sprite = &sprite;
	m_rasterizer->AddCommand(std::move(cmd));
}

void FAsciiRendererModule::DrawText(
	int32_t worldX, int32_t worldY, int32_t zOrder,
	const char* text, FColor fg, FColor bg)
{
	if (!m_rasterizer || !text) return;

	FDrawCommand cmd;
	cmd.Type = EDrawCommandType::Text;
	cmd.WorldX = worldX;
	cmd.WorldY = flipY(worldY);
	cmd.ZOrder = zOrder;
	cmd.BlendState = m_blendState;
	cmd.Text = text;
	cmd.Fg = fg;
	cmd.Bg = bg;
	m_rasterizer->AddCommand(std::move(cmd));
}

void FAsciiRendererModule::FillRect(
	int32_t worldX, int32_t worldY, int32_t width,
	int32_t height, int32_t zOrder, FAsciiCell cell)
{
	if (!m_rasterizer) return;

	FDrawCommand cmd;
	cmd.Type = EDrawCommandType::FillRect;
	cmd.WorldX = worldX;
	cmd.WorldY = flipY(worldY, height);
	cmd.ZOrder = zOrder;
	cmd.BlendState = m_blendState;
	cmd.Cell = cell;
	cmd.Width = width;
	cmd.Height = height;
	m_rasterizer->AddCommand(std::move(cmd));
}

void FAsciiRendererModule::DrawBox(
	int32_t worldX, int32_t worldY, int32_t width,
	int32_t height, int32_t zOrder, FColor fg, FColor bg)
{
	if (!m_rasterizer) return;

	FDrawCommand cmd;
	cmd.Type = EDrawCommandType::DrawBox;
	cmd.WorldX = worldX;
	cmd.WorldY = flipY(worldY, height);
	cmd.ZOrder = zOrder;
	cmd.BlendState = m_blendState;
	cmd.Fg = fg;
	cmd.Bg = bg;
	cmd.Width = width;
	cmd.Height = height;
	m_rasterizer->AddCommand(std::move(cmd));
}

void FAsciiRendererModule::SetBlendState(const FAsciiBlendState& state)
{
	m_blendState = state;
}

} // namespace Enigma

// Module registration - name "Renderer" so GetRendererModule() finds it.
IMPLEMENT_MODULE(Enigma::FAsciiRendererModule, Renderer)
