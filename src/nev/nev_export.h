#pragma once

// Defines NEV_EXPORT so that functionality implemented by the net module can
// be exported to consumers, and NEV_EXPORT_PRIVATE that allows unit tests to
// access features not intended to be used directly by real consumers.

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(NEV_IMPLEMENTATION)
#define NEV_EXPORT __declspec(dllexport)
#define NEV_EXPORT_PRIVATE __declspec(dllexport)
#else
#define NEV_EXPORT __declspec(dllimport)
#define NEV_EXPORT_PRIVATE __declspec(dllimport)
#endif  // defined(NEV_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(NEV_IMPLEMENTATION)
#define NEV_EXPORT __attribute__((visibility("default")))
#define NEV_EXPORT_PRIVATE __attribute__((visibility("default")))
#else
#define NEV_EXPORT
#define NEV_EXPORT_PRIVATE
#endif
#endif

#else  /// defined(COMPONENT_BUILD)
#define NEV_EXPORT
#define NEV_EXPORT_PRIVATE
#endif
