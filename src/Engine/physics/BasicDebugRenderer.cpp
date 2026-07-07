#define EXCLUDE_JOLT

#ifdef EXCLUDE_JOLT
#else
#include <Jolt/Jolt.h>
#define JPH_DEBUG_RENDERER
#include <Jolt/Renderer/DebugRendererSimple.h>
#endif