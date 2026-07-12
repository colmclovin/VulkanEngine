#pragma once

// Wrapper to include EnTT with proper compiler settings
// This disables narrowing conversion errors that occur in EnTT with MSVC

#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable: 4244 4267 4305 4819)
	// Temporarily disable conformance mode for EnTT
	#pragma conform(forScope, push)
#endif

#include <entt/entt.hpp>

#ifdef _MSC_VER
	#pragma conform(forScope, pop)
	#pragma warning(pop)
#endif
