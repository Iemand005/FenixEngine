
#include "kiss_fftr.h"

#ifdef _WIN32
#include <audio/WasapiLoopbackCapture.hpp>
#else
#include <audio/PipeWireLoopbackCapture.hpp>
#endif

class AudioVisualiser {

};