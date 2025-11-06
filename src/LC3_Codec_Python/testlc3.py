import serial
import wave
import numpy as np
import time

# Serial ke ESP32
ser = serial.Serial("COM8", 115200)

# Load WAV mono 16-bit
wav_file = wave.open("audio.wav", "rb")
frames_per_packet = 30

while True:
    raw = wav_file.readframes(frames_per_packet)
    if len(raw) == 0:
        break
    # Convert ke int16
    pcm = np.frombuffer(raw, dtype=np.int16)
    # Dummy LC3 encode: 60 byte
    lc3_frame = bytearray(60)
    for i in range(60):
        lc3_frame[i] = pcm[i % len(pcm)] & 0xFF
    # Kirim ke ESP32
    ser.write(b'\x55' + lc3_frame)
    time.sleep(0.01)  # 10ms per frame
