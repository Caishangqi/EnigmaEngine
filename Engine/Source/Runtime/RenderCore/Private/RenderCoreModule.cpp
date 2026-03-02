// Copyright EnigmaEngine. All Rights Reserved.

// RenderCore module entry point and out-of-line implementations.

#include "RenderCoreAPI.generated.h"
#include "RenderCore/AsciiSprite.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleMacros.h"

#include <cassert>
#include <cstdio>

namespace Enigma
{

// ---------------------------------------------------------------
// FAsciiSprite
// ---------------------------------------------------------------

FAsciiSprite::FAsciiSprite(int32_t w, int32_t h)
	: Cells(static_cast<size_t>(w) * h)
	, Width(w)
	, Height(h)
{
}

FAsciiCell& FAsciiSprite::At(int32_t x, int32_t y)
{
	assert(x >= 0 && x < Width && y >= 0 && y < Height);
	return Cells[static_cast<size_t>(y) * Width + x];
}

const FAsciiCell& FAsciiSprite::At(int32_t x, int32_t y) const
{
	assert(x >= 0 && x < Width && y >= 0 && y < Height);
	return Cells[static_cast<size_t>(y) * Width + x];
}

} // namespace Enigma

// ---------------------------------------------------------------
// Module entry point
// ---------------------------------------------------------------

class FRenderCoreModule : public Enigma::IModuleInterface
{
public:
	void StartupModule() override
	{
		std::printf("[RenderCore] StartupModule\n");
	}

	void ShutdownModule() override
	{
		std::printf("[RenderCore] ShutdownModule\n");
	}
};

IMPLEMENT_MODULE(FRenderCoreModule, RenderCore)
