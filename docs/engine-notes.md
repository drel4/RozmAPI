# Engine adapter notes

Analyzed with Ghidra 12.1.2, x86 LE / Borland Delphi compiler specification.
All addresses below are original virtual addresses at preferred image base `0x400000`;
the adapter resolves them relative to the actual mapping and applies PE relocations.

## Native text-to-speech path

The application's reading thread at `0x4678b4` gathers phrases, reads GUI state,
processes UI directives, then calls the following pipeline. We call the pipeline
directly; the reading thread and UI directives are deliberately not used.

| Address | Use in adapter |
| --- | --- |
| `0x465888` | Initial text normalization |
| `0x4659dc` | Number-to-word expansion |
| `0x465e9c` | Dictionary / lexical conversion |
| `0x46621c` | Further pronunciation normalization |
| `0x465210` | Stress assignment, modifies an AnsiString pointer |
| `0x46662c` | Phonetic encoding |
| `0x466e7c` | Diphone/sample synthesis into unsigned 8-bit PCM |
| `0x4674f2` | Output branch patched to the PCM capture callback |
| `0x46768f` | Original synthesis cleanup, resumes after capture |

Delphi register calling convention: first argument in EAX, second in EDX, third
in ECX. String-returning stages receive the result pointer in EDX. AnsiStrings
have a signed 32-bit reference count at `data-8`, length at `data-4`, and a trailing NUL.
The helper installs adapters at the original memory-manager pointers so the native
reference-counting and copy-on-write string operations remain usable without RTL startup.

## Initialization reproduced from FormActivate (`0x463ef4`)

| Original address | Data |
| --- | --- |
| `0x46d040/44/48` | Delphi GetMem / FreeMem / ReallocMem function pointers |
| `0x46e6e0` | Diphone matrix width: 42, already initialized in original DATA |
| `0x46e6e4` | Speed, 1..9 |
| `0x46e6e8` | Voice character: `'1'`, `'2'`, `'3'` |
| `0x46fca4` | 500000-byte synthesis buffer |
| `0x4ec0f0`, `0x4ec178` | Crossfade ramps, 17 doubles each, value `i/16` |
| `0x4ec200` | Word-index count from `skfs/wif[0]` |
| `0x4ec204` | Stress-dictionary record count |
| `0x4ec218` | Correction-dictionary byte size / base of English string-pointer array |
| `0x5578dc` | English dictionary line count |
| `0x67c860` | `skfs/skf` |
| `0x770aa4` | `skfs/wif` |
| `0x7d2528` | `skfs/wlf` |
| `0x7eabd4` | `snfN/ip2f`, 42x42 32-bit offsets |
| `0x7ec764` | `snfN/lp2f`, 42x42 32-bit lengths |
| `0x7ee2f4` | `BSNbn` |
| `0x1b00ff8` | Stress-dictionary byte length, followed by record-offset array |
| `0x217d27c` | `snfN/Sd2f`, voice sample data |

Stress-dictionary indexing mirrors the original: start with index 1, zero first
offset, and append `i+1` for each byte in the interval `(0x25, 0x51)` starting at byte 1.
English dictionary lines retain their original CP1251 bytes and CRLF is removed.
No registry changes, installed-file modifications, or original executable startup are needed.

## PCM capture patch

Replace the code starting at `0x4674f2` with a relative CALL to an x86 naked adapter
followed by a relative JMP to `0x46768f`. The adapter preserves general registers,
reads the sample count from `[EBP-0x0c]`, and copies that many bytes from `0x46fca4`.
The original device/file branches are skipped, but the synthesis routine's own
SEH-chain restoration and AnsiString cleanup still execute.

The engine CODE section is writable during mapping/patching and executable/read-only
afterward. The expected full-image SHA-256 is checked before any PE parsing or mapping.
The helper exits after each request, so native global state and mapped code cannot leak
between concurrent calls or outlive the caller's DLL unload.
