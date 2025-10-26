import sounddevice as sd
import numpy as np
import time
import struct

FS = 16000
FRAME_MS = 7.5
SAMPLES_PER_FRAME = int(FS * FRAME_MS / 1000)
sequence_number = 0

def lc3_encode(pcm_frame):
    return bytes((pcm_frame >> 8).astype(np.uint8))

def make_lc3_packet(sequence_number, lc3_payload):
    timestamp = int(time.time() * 1000) & 0xFFFFFFFF
    packet = struct.pack('<HI', sequence_number, timestamp) + lc3_payload
    return packet

# ------------------------
# Stream input + output
# ------------------------
def audio_callback(indata, outdata, frames, time_info, status):
    global sequence_number
    pcm_frame = (indata[:,0] * 32767).astype(np.int16)

    # Encode LC3 (simulasi)
    lc3_frame = lc3_encode(pcm_frame)
    pkt = make_lc3_packet(sequence_number, lc3_frame)
    sequence_number = (sequence_number + 1) % 65536

    # Kirim PCM asli ke outdata (realtime)
    outdata[:,0] = pcm_frame / 32768.0  # normalisasi float -1..1

    print(f"SN={sequence_number}, Paket size={len(pkt)} bytes")

with sd.Stream(samplerate=FS, blocksize=SAMPLES_PER_FRAME,
               dtype='float32',
               channels=1, callback=audio_callback):
    print("Recording from mic... Speak now! Press Ctrl+C to stop")
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("Stopped")
