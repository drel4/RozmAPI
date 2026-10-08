# RozmAPI

> This project is AI-assisted by Codex. If you're against AI-assisted projects, please don't use it.

A Windows C ABI DLL for using **Розмовлялька TTS outside its original application**.

The wrapper calls the original synthesis machine code, including its dictionaries,
stress assignment, number expansion, phonetic conversion and voice samples.
It does **not** launch `Rozm.exe`, automate its windows, initialize VCL, or open an audio device.
Both 32-bit and 64-bit applications are supported.

## Quick start

Use the DLL matching **your application's architecture**, not your Windows architecture:

| Application | Files to place beside each other |
| --- | --- |
| 32-bit | `build/win32/RozmAPI.dll`, `build/win32/RozmWorker32.exe` |
| 64-bit | `build/win64/RozmAPI.dll`, `build/win64/RozmWorker32.exe` |

Keep the original Розмовлялька installation. The default location is
`C:\Program Files (x86)\Розмовлялька`; a custom installation directory can be passed to `Rozm_CreateW`.
The original executable is read as engine code and its voice/dictionary files are required.
No files in the installation are modified.

### Python

No third-party Python packages are needed:

```powershell
python examples/rozm.py "Привіт, світе!" --output speech.wav --voice 1 --speed 5
```

Or use the binding:

```python
from rozm import Rozm  # examples/rozm.py

with Rozm() as tts:
    tts.save_wav("Це український синтез мовлення.", "speech.wav")
    samples = tts.pcm("Привіт!")  # unsigned 8-bit PCM, mono, 11025 Hz
```

### C / C++

Include `include/RozmAPI.h` and link the matching `RozmAPI.lib`, or use
`LoadLibraryW` / `GetProcAddress` for dynamic loading. All exports use **cdecl**;
export names are undecorated in both DLL builds.

```c
RozmHandle tts = NULL;
int status = Rozm_CreateW(NULL, &tts);
if (status == ROZM_OK) {
    RozmAudio audio = {0};
    status = Rozm_SynthesizeUtf8(tts, "Привіт, світе!", 1, 5, &audio);
    if (status == ROZM_OK) {
        /* Consume audio.data[0 .. audio.size-1] here. */
        Rozm_FreeAudio(&audio);
    }
    Rozm_Destroy(tts);
}
```

Compile source containing literal Ukrainian text as UTF-8 (`cl /utf-8`).
`examples/native.c` uses Unicode escapes and demonstrates direct WAV output.
To build it from a Visual Studio developer prompt for the desired architecture:

```bat
cl /nologo examples\native.c build\win64\RozmAPI.lib /Febuild\win64\native-example.exe
build\win64\native-example.exe
```

For a 32-bit developer prompt, replace `win64` with `win32`.

## API

| Export | Purpose |
| --- | --- |
| `Rozm_CreateW(directory, &handle)` | Create a context; `NULL` selects the default installation |
| `Rozm_Destroy(handle)` | Release the context |
| `Rozm_SynthesizeW(handle, text, voice, speed, &audio)` | UTF-16 text to PCM |
| `Rozm_SynthesizeUtf8(handle, text, voice, speed, &audio)` | UTF-8 text to PCM |
| `Rozm_FreeAudio(&audio)` | Release returned samples and zero the structure |
| `Rozm_SaveWavW(handle, text, voice, speed, filename)` | UTF-16 text and filename to a PCM WAV file |
| `Rozm_GetLastError()` | Thread-local UTF-8 error message |
| `Rozm_GetVersion()` | API version, currently `0x00010000` (1.0) |

Synthesis and WAV saving return `0` on success. Error codes are defined in the header.
Initialize the output structure to zero and free successful results before reusing it.
Empty/whitespace-only text produces no samples; empty WAV output is a valid zero-frame WAV.
`Rozm_SaveWavW` replaces an existing output file.

| Voice | Name |
| --- | --- |
| 1 | Анатоль |
| 2 | Стьопа |
| 3 | Руся |

Speed accepts `1..9`, with `5` as normal. The original engine only applies speed
adjustment to voice 1; voices 2 and 3 retain their recorded timing.

The API accepts Unicode but the original engine uses Windows-1251. Unsupported
characters, such as emoji, return an error instead of silently becoming `?`.
Curly apostrophes and long dashes are normalized. Text follows the original
engine's Ukrainian pronunciation rules; English words use its bundled dictionary.

## Implementation and limits

- Each request runs in a short-lived **headless `RozmWorker32.exe`** subprocess,
  for both DLL architectures. The DLL is an API bridge, not a reimplementation of TTS.
- The helper maps the exact supported `Rozm.exe` image, resolves Windows imports,
  relocates it, replaces its Delphi allocator with Windows heap adapters, loads
  dictionaries directly, and redirects the final PCM output to a buffer.
- Original EXE entry point, unit initialization, form creation, playback, and
  WAV/file-output code are not executed. The in-memory PCM patch does not touch disk.
- Supported executable: **July 27, 2007**, 520704 bytes, SHA-256
  `90e26b39a4ca518df3b1ebcf4bbaf90a8543ed90a31273241d580d22b39e1a54`.
  Other versions are rejected; engine addresses are build-specific.
- The supported target is Windows 10/11 x86/x64. The original proprietary
  executable/data are **not included** in the wrapper archive.
- Contexts can be used concurrently. Do not destroy a context while a request is running.
  Each call reloads the engine/dictionaries; this trades startup overhead for isolation.
- Requests are synchronous, with a 60-second worker timeout, a 262144 UTF-16-code-unit
  input limit and a 256 MiB PCM output limit. A legacy-engine fault is reported to the caller.
- Long phrases are divided into short chunks to stay within the original 500000-byte
  scratch buffer. Chunk boundaries can affect stress/context and pause placement.
  This API is not a streaming engine or a SAPI voice registration.

## Build and verification

Run `build.cmd` with Visual Studio C++ Build Tools and the Windows SDK installed.
For a different installation of Visual Studio, set `VCVARSALL` to its
`VC\Auxiliary\Build\vcvarsall.bat`. The runtime is linked statically (`/MT`).

```powershell
.\build.cmd
python tests/integration.py
```

The integration suite uses the actual installed engine and checks all three voices,
non-silent PCM, voice-1 speed changes, WAV headers, Unicode output paths, numeric
and English input, repeatability, concurrent calls, long text, empty input and errors.
Run it under both 32-bit and 64-bit Python to test both DLL builds.
Sample outputs are written to `build/test-output/64/voice1.wav` through `voice3.wav`
(or the `32` subdirectory for 32-bit Python).

Reverse-engineering scripts and address notes are in `tools/` and `docs/engine-notes.md`.
Ghidra projects/decompilation are local build artifacts, excluded from the package.

## License

The RozmAPI wrapper source code is released under the [WTFPL, version 2](LICENSE).
This license applies to this project's code, not to the original Розмовлялька
executable or voice/dictionary data, which are not distributed here.
