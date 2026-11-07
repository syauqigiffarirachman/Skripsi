"""Jalankan benchmark ECDSA software di board, simpan CSV, dan verifikasi tanda tangan
secara independen di PC (implementasi P-256 murni Python).
Pemakaian: python ecdsa_bench.py COM12 [N] [out.csv]"""
import sys, time, serial

PORT = sys.argv[1] if len(sys.argv) > 1 else "COM12"
N = int(sys.argv[2]) if len(sys.argv) > 2 else 30
OUT = sys.argv[3] if len(sys.argv) > 3 else None

# --- NIST P-256 ---
p  = 0xFFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFF
a  = p - 3
n  = 0xFFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551
G  = (0x6B17D1F2E12C4247F8BCE6E563A440F277037D812DEB33A0F4A13945D898C296,
      0x4FE342E2FE1A7F9B8EE7EB4A7C0F9E162BCE33576B315ECECBB6406837BF51F5)

def add(P, Q):
    if P is None: return Q
    if Q is None: return P
    if P[0] == Q[0] and (P[1] + Q[1]) % p == 0: return None
    if P == Q: l = (3 * P[0] * P[0] + a) * pow(2 * P[1], -1, p) % p
    else:      l = (Q[1] - P[1]) * pow(Q[0] - P[0], -1, p) % p
    x = (l * l - P[0] - Q[0]) % p
    return (x, (l * (P[0] - x) - P[1]) % p)

def mul(k, P):
    R = None
    while k:
        if k & 1: R = add(R, P)
        P = add(P, P); k >>= 1
    return R

def verify(pub, digest, r, s):
    if not (0 < r < n and 0 < s < n): return False
    w = pow(s, -1, n)
    X = add(mul(int.from_bytes(digest, "big") * w % n, G), mul(r * w % n, pub))
    return X is not None and X[0] % n == r

def readline(s, timeout=30):
    t0 = time.time(); buf = b""
    while time.time() - t0 < timeout:
        c = s.read(1)
        if c == b"\n": return buf.decode(errors="replace").strip()
        buf += c
    raise TimeoutError("timeout membaca serial")

s = serial.Serial(PORT, 115200, timeout=0.5); s.dtr = True
time.sleep(0.5); s.reset_input_buffer()

# 1) Benchmark di board
s.write(f"B{N}\n".encode())
info, rows = {}, []
while True:
    line = readline(s, 60)
    print(line)
    if line.startswith("# Public key"): info["pub"] = line.split(":")[1].strip()
    if line.startswith("# Digest"):     info["digest"] = line.split(":")[1].strip()
    if line and line[0].isdigit():      rows.append(line)
    if line.startswith("# SELESAI"): break
if OUT:
    with open(OUT, "w") as f:
        f.write("i,cycles,ms,stack_bytes,heap_delta,sign_ok,verify_ok\n")
        f.write("\n".join(rows) + "\n")
    print("CSV disimpan:", OUT)

# 2) Verifikasi independen di PC: minta board menandatangani beberapa digest
pub_b = bytes.fromhex(info["pub"]); pub = (int.from_bytes(pub_b[:32], "big"), int.from_bytes(pub_b[32:], "big"))
digests = [bytes.fromhex(info["digest"]), bytes(32), bytes([0xFF]) * 32, bytes(range(32))]
ok = 0
for d in digests:
    s.reset_input_buffer(); s.write(b"S" + d.hex().upper().encode() + b"\n")
    sig_hex = readline(s).split("|")[0]
    sig = bytes.fromhex(sig_hex)
    good = verify(pub, d, int.from_bytes(sig[:32], "big"), int.from_bytes(sig[32:], "big"))
    # tanda tangan rusak harus ditolak
    bad = verify(pub, d, int.from_bytes(sig[:32], "big"), (int.from_bytes(sig[32:], "big") ^ 1) or 1)
    print(f"verifikasi PC digest={d.hex()[:16]}..  valid={good}  sig_rusak_ditolak={not bad}")
    ok += good and not bad
s.close()
print(f"HASIL VERIFIKASI PC: {ok}/{len(digests)}")
sys.exit(0 if ok == len(digests) and len(rows) == N else 1)
