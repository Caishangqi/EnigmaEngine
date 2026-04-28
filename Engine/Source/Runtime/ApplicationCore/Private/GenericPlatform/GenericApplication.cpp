// Copyright EnigmaEngine. All Rights Reserved.

#include "GenericPlatform/GenericApplication.h"
#include "HAL/Platform.h"

#if ENIGMA_PLATFORM_WINDOWS
    #include "Windows/WindowsApplication.h"
#endif

namespace Enigma
{

FGenericApplication* FGenericApplication::s_application = nullptr;

FGenericApplication::~FGenericApplication()
{
    if (s_application == this)
    {
        s_application = nullptr;
    }
}

void FGenericApplication::SetMessageHandler(FGenericApplicationMessageHandler* handler)
{
    MessageHandler = handler;
}

FGenericApplicationMessageHandler* FGenericApplication::GetMessageHandler() const
{
    return MessageHandler;
}

FGenericApplication* FGenericApplication::CreateApplication()
{
    if (s_application == nullptr)
    {
#if ENIGMA_PLATFORM_WINDOWS
        s_application = new FWindowsApplication();
#else
        // No platform implementation available.
        s_application = nullptr;
#endif
    }
    return s_application;
}

FGenericApplication* FGenericApplication::GetApplication()
{
    return s_application;
}

} // namespace Enigma
