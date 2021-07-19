#pragma once
#include <Core/CoreMacros.hpp>

/// Defines the correct macro to export dll symbols.
#if defined Headless_EXPORTS
#    define HEADLESS_API DLL_EXPORT
#elif defined Headless_STATIC
#    define HEADLESS_API
#else
#    define HEADLESS_API DLL_IMPORT
#endif
