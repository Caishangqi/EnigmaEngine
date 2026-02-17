// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

// -------------------------------------------------------------
// DelegateFwd.h -- Forward declarations for the Delegate system.
//
// Include this header when you only need to reference delegate
// types in declarations (e.g. function parameters, class members)
// without pulling in the full template definitions.
//
// For full definitions, include the individual headers:
//   #include "Delegates/DelegateHandle.h"
//   #include "Delegates/Delegate.h"
//   #include "Delegates/MulticastDelegate.h"
//
// Type declaration convention (using aliases):
//   using FSimpleDelegate       = TDelegate<void()>;
//   using FGetNameDelegate      = TDelegate<const char*()>;
//   using FOnWindowResized      = TMulticastDelegate<int32_t, int32_t>;
//   using FOnWindowClosed       = TMulticastDelegate<class FGenericWindow*>;
// -------------------------------------------------------------

namespace Enigma
{

class FDelegateHandle;

template <typename Signature>
class TDelegate;

template <typename... Args>
class TMulticastDelegate;

} // namespace Enigma
