// Copyright EnigmaEngine. All Rights Reserved.

#include "Delegates/DelegateHandle.h"

namespace Enigma
{

uint64_t FDelegateHandle::s_nextId = 1;

FDelegateHandle::FDelegateHandle(uint64_t InId) noexcept
    : m_id(InId)
{
}

bool FDelegateHandle::IsValid() const noexcept
{
    return m_id != 0;
}

void FDelegateHandle::Reset() noexcept
{
    m_id = 0;
}

FDelegateHandle FDelegateHandle::Generate()
{
    return FDelegateHandle(s_nextId++);
}

} // namespace Enigma
