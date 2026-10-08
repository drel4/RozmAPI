"""Integration checks against the actual installed legacy engine, not mocks."""
import ctypes as C
from pathlib import Path
import statistics
import sys
import wave
from concurrent.futures import ThreadPoolExecutor
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "examples"))
from rozm import Rozm, Audio

output = Path(__file__).resolve().parents[1] / "build" / "test-output" / str(C.sizeof(C.c_void_p)*8)
output.mkdir(parents=True, exist_ok=True)
with Rozm() as tts:
    text = "Привіт, світе! Це перевірка синтезу української мови."
    voices = [tts.pcm(text, voice=i) for i in (1, 2, 3)]
    assert len(set(voices)) == 3
    for voice, data in enumerate(voices, 1):
        assert len(data) > 10000 and statistics.pstdev(data) > 1
        tts.save_wav(text, output / f"voice{voice}.wav", voice=voice)
    slow, normal, fast = [tts.pcm("Привіт, світе!", speed=s) for s in (1, 5, 9)]
    assert len(slow) > len(normal) > len(fast)
    assert tts.pcm("") == b""
    assert tts.pcm("   \n") == b""
    assert len(tts.pcm("123 гривні, 45 копійок. Hello, world!")) > 10000
    # Repeated calls and simultaneous calls are independent worker processes.
    assert tts.pcm(text) == voices[0]
    with ThreadPoolExecutor(max_workers=3) as pool:
        concurrent = list(pool.map(tts.pcm, [text] * 3))
    assert all(data == voices[0] for data in concurrent)
    long_text = "Це довге речення без крапки " * 40
    assert len(tts.pcm(long_text)) > len(tts.pcm(long_text[:160])) * 3
    tts.save_wav("Привіт!", output / "український шлях.wav")
    with wave.open(str(output / "український шлях.wav"), "rb") as wav:
        assert (wav.getnchannels(), wav.getsampwidth(), wav.getframerate()) == (1, 1, 11025)
        assert wav.getnframes() > 1000
    for text, voice, speed in [("текст", 0, 5), ("текст", 1, 10), ("😀", 1, 5)]:
        try: tts.pcm(text, voice, speed)
        except RuntimeError: pass
        else: raise AssertionError("Invalid input accepted")
    invalid = Audio()
    assert tts.lib.Rozm_SynthesizeUtf8(tts.handle, b"\xff", 1, 5, C.byref(invalid)) == 1
    assert not invalid.data
try: Rozm(str(output / "missing-installation"))
except RuntimeError: pass
else: raise AssertionError("Missing installation accepted")
# The build guard must reject even a same-sized executable with a one-byte change.
original = Path("C:/Program Files (x86)/Розмовлялька/Rozm.exe").read_bytes()
bad_install = output / "wrong-build"
bad_install.mkdir(exist_ok=True)
changed = bytearray(original)
changed[-1] ^= 1
(bad_install / "Rozm.exe").write_bytes(changed)
with Rozm(str(bad_install)) as tts:
    try: tts.pcm("Привіт!")
    except RuntimeError as exc: assert "SHA256" in str(exc)
    else: raise AssertionError("Unrecognized engine image accepted")
print(f"PASS ({C.sizeof(C.c_void_p)*8}-bit): three voices, speed, WAV, Unicode paths, empty input, numbers/English, repeatability, concurrency, long text, invalid inputs")
