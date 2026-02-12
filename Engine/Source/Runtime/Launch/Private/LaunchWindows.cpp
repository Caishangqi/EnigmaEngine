// Copyright EnigmaEngine. All Rights Reserved.
//
// Platform entry points for the Launch module.
// Provides main() for all platforms and WinMain() for Windows GUI apps.

#include "Launch.h"

#include <cstdio>
#include <cstring>
#include <string>

#if ENIGMA_PLATFORM_WINDOWS
#include <windows.h>
#endif

// ---------------------------------------------------------------
// main -- standard C++ entry point
// ---------------------------------------------------------------
int main(int argc, char* argv[])
{
    // Disable stdout buffering so output is visible immediately when piped
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    // Concatenate argv into a single command-line string
    std::string cmdLine;
    for (int i = 1; i < argc; ++i)
    {
        if (i > 1) cmdLine += ' ';
        cmdLine += argv[i];
    }

    int32_t result = Enigma::GuardedMain(cmdLine.c_str());
    return static_cast<int>(result);
}

// ---------------------------------------------------------------
// WinMain -- Windows GUI subsystem entry point
// ---------------------------------------------------------------
#if ENIGMA_PLATFORM_WINDOWS

int WINAPI WinMain(
    _In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_     LPSTR     lpCmdLine,
    _In_     int       nShowCmd)
{
    (void)hInstance;
    (void)hPrevInstance;
    (void)nShowCmd;

    int32_t result = Enigma::GuardedMain(lpCmdLine);
    return static_cast<int>(result);
}

#endif // ENIGMA_PLATFORM_WINDOWS
