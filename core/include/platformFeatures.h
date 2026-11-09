#ifndef PLATFORM_FEATURES_H
#define PLATFORM_FEATURES_H

// Per-platform feature switches. Features test these, never PLATFORM_IMX directly.

#ifdef PLATFORM_IMX
#define FEATURE_REALTIME_FX 1
#else
#define FEATURE_REALTIME_FX 0
#endif

#endif // PLATFORM_FEATURES_H
