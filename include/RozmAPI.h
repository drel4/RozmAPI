#pragma once
#include <stdint.h>
#include <wchar.h>
#if defined(ROZM_BUILD)
#define ROZM_API __declspec(dllexport)
#else
#define ROZM_API __declspec(dllimport)
#endif
#ifdef __cplusplus
extern "C" {
#endif
typedef struct RozmContext* RozmHandle;
typedef struct RozmAudio {
    uint8_t* data;
    uint32_t size;
    uint32_t sample_rate;
    uint16_t channels;
    uint16_t bits_per_sample;
} RozmAudio;
enum RozmStatus { ROZM_OK=0, ROZM_INVALID_ARGUMENT=1, ROZM_IO_ERROR=2,
    ROZM_ENGINE_ERROR=3, ROZM_TIMEOUT=4, ROZM_OUT_OF_MEMORY=5 };
/* All exports use cdecl. Null installation_directory selects the installed default.
   Contexts may be used concurrently, but must not be destroyed during a call.
   Voice: 1 Anatol, 2 Stiopa, 3 Rusya. Speed: 1..9, default 5.
   Synthesis is synchronous. Audio is unsigned 8-bit mono PCM at 11025 Hz.
   On success release audio with Rozm_FreeAudio, including before reusing it. */
ROZM_API int __cdecl Rozm_CreateW(const wchar_t* installation_directory, RozmHandle* out);
ROZM_API void __cdecl Rozm_Destroy(RozmHandle handle);
ROZM_API int __cdecl Rozm_SynthesizeW(RozmHandle handle,const wchar_t* text,int voice,int speed,RozmAudio* out);
ROZM_API int __cdecl Rozm_SynthesizeUtf8(RozmHandle handle,const char* text,int voice,int speed,RozmAudio* out);
ROZM_API void __cdecl Rozm_FreeAudio(RozmAudio* audio);
ROZM_API int __cdecl Rozm_SaveWavW(RozmHandle handle,const wchar_t* text,int voice,int speed,const wchar_t* filename);
/* UTF-8 error string valid until the next API call on this thread. */
ROZM_API const char* __cdecl Rozm_GetLastError(void);
ROZM_API uint32_t __cdecl Rozm_GetVersion(void);
#ifdef __cplusplus
}
#endif
