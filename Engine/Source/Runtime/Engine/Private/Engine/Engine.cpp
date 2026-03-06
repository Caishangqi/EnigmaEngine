// Copyright EnigmaEngine. All Rights Reserved.

#include "Engine/Engine.h"

#include <chrono>
#include <cstdio>
#include <thread>

namespace Enigma
{

// Global engine pointer
FEngine* GEngine = nullptr;

void FEngine::Init(FEngineLoop* /*engineLoop*/)
{
    std::printf("[FEngine] Init\n");
    SubsystemCollection.Initialize();
}

void FEngine::Start()
{
    std::printf("[FEngine] Start\n");
}

void FEngine::Tick(float deltaTime)
{
    DeltaTime = deltaTime;
    ++TickCount;
    SubsystemCollection.Tick(deltaTime);
}

void FEngine::Shutdown()
{
    SubsystemCollection.Deinitialize();
    std::printf("[FEngine] Shutdown\n");
}

void FEngine::SetMaxFPS(float fps)
{
    m_maxFPS = (fps > 0.0f) ? fps : 0.0f;
}

float FEngine::GetMaxFPS() const
{
    return m_maxFPS;
}

void FEngine::UpdateTimeAndHandleMaxTickRate()
{
    if (m_maxFPS <= 0.0f)
    {
        // Uncapped -- just update the timestamp
        m_lastTickTime = Clock::now();
        return;
    }

    using namespace std::chrono;

    const auto targetInterval = duration<double>(1.0 / static_cast<double>(m_maxFPS));
    const auto now = Clock::now();

    if (m_lastTickTime.time_since_epoch().count() != 0)
    {
        const auto elapsed = now - m_lastTickTime;
        const auto waitTime = targetInterval - elapsed;

        if (waitTime > milliseconds(0))
        {
            // Hybrid sleep strategy:
            // Sleep for bulk of wait time (minus 2ms margin), then spin-wait remainder.
            const auto sleepThreshold = milliseconds(2);
            if (waitTime > sleepThreshold)
            {
                std::this_thread::sleep_for(waitTime - sleepThreshold);
            }

            // Spin-wait for the remaining time
            while (Clock::now() - m_lastTickTime < targetInterval)
            {
                // busy-wait
            }
        }
    }

    m_lastTickTime = Clock::now();
}

} // namespace Enigma
