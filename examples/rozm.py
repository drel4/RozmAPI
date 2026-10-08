"""ctypes binding; works with both 32-bit and 64-bit Python on Windows."""
import ctypes as C
from pathlib import Path
import struct
import wave

class Audio(C.Structure):
    _fields_ = [("data", C.POINTER(C.c_uint8)), ("size", C.c_uint32),
                ("sample_rate", C.c_uint32), ("channels", C.c_uint16),
                ("bits_per_sample", C.c_uint16)]

class Rozm:
    def __init__(self, installation=None, dll=None):
        arch = "win64" if struct.calcsize("P") == 8 else "win32"
        dll = dll or Path(__file__).resolve().parents[1] / "build" / arch / "RozmAPI.dll"
        self.lib = C.CDLL(str(dll))
        self.lib.Rozm_CreateW.argtypes = [C.c_wchar_p, C.POINTER(C.c_void_p)]
        self.lib.Rozm_CreateW.restype = C.c_int
        self.lib.Rozm_Destroy.argtypes = [C.c_void_p]
        self.lib.Rozm_Destroy.restype = None
        self.lib.Rozm_GetLastError.restype = C.c_char_p
        self.lib.Rozm_SynthesizeUtf8.argtypes = [C.c_void_p, C.c_char_p, C.c_int, C.c_int, C.POINTER(Audio)]
        self.lib.Rozm_SynthesizeUtf8.restype = C.c_int
        self.lib.Rozm_SaveWavW.argtypes = [C.c_void_p, C.c_wchar_p, C.c_int, C.c_int, C.c_wchar_p]
        self.lib.Rozm_SaveWavW.restype = C.c_int
        self.lib.Rozm_FreeAudio.argtypes = [C.POINTER(Audio)]
        self.lib.Rozm_FreeAudio.restype = None
        self.handle = C.c_void_p()
        self._check(self.lib.Rozm_CreateW(installation, C.byref(self.handle)))

    def _check(self, status):
        if status:
            raise RuntimeError(f"Rozm error {status}: {self.lib.Rozm_GetLastError().decode('utf-8', 'replace')}")

    def pcm(self, text, voice=1, speed=5):
        audio = Audio()
        self._check(self.lib.Rozm_SynthesizeUtf8(self.handle, text.encode("utf-8"), voice, speed, C.byref(audio)))
        try:
            return C.string_at(audio.data, audio.size) if audio.size else b""
        finally:
            self.lib.Rozm_FreeAudio(C.byref(audio))

    def save_wav(self, text, filename, voice=1, speed=5):
        self._check(self.lib.Rozm_SaveWavW(self.handle, text, voice, speed, str(filename)))

    def close(self):
        if self.handle:
            self.lib.Rozm_Destroy(self.handle)
            self.handle = C.c_void_p()

    def __enter__(self): return self
    def __exit__(self, *_): self.close()

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("text", nargs="?", default="Привіт, світе! Це Розмовлялька поза своєю програмою.")
    parser.add_argument("--output", default="speech.wav")
    parser.add_argument("--voice", type=int, default=1)
    parser.add_argument("--speed", type=int, default=5)
    parser.add_argument("--installation")
    args = parser.parse_args()
    with Rozm(args.installation) as tts:
        tts.save_wav(args.text, args.output, args.voice, args.speed)
    with wave.open(args.output, "rb") as wav:
        print(f"Saved {args.output}: {wav.getnframes()/wav.getframerate():.2f}s, {wav.getframerate()} Hz")
