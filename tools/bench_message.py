"""Benchmark tanda tangan ECDSA P-256 atas sebuah PESAN (SHA-256 di host lalu sign),
untuk firmware software (softwaretes) atau DS28E38 (main.cpp), dengan verifikasi PC.

Pemakaian:
  python tools/bench_message.py sw COM12 "ayah mau mandi" 30 data/k1_sw_msg.csv
  python tools/bench_message.py hw COM12 "ayah mau mandi" 30 data/k2_hw_msg.csv
"""
import hashlib, math, os, re, sys, time
import serial
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from p256 import verify, on_curve

mode, port, msg = sys.argv[1], sys.argv[2], sys.argv[3]
N = int(sys.argv[4]) if len(sys.argv) > 4 else 30
out = sys.argv[5] if len(sys.argv) > 5 else None
digest_pc = hashlib.sha256(msg.encode()).digest()

s = serial.Serial(port, 115200, timeout=0.3); s.dtr = True
time.sleep(0.5); s.reset_input_buffer()

def readline(timeout=10):
    t0, buf = time.time(), b""
    while time.time() - t0 < timeout:
        c = s.read(1)
        if c == b"\n": return buf.decode(errors="replace").strip()
        buf += c
    raise TimeoutError("timeout serial")

def stats(xs):
    m = sum(xs) / len(xs)
    sd = math.sqrt(sum((x - m) ** 2 for x in xs) / (len(xs) - 1)) if len(xs) > 1 else 0
    return m, sd, min(xs), max(xs)

def summary(name, xs, unit):
    m, sd, lo, hi = stats(xs)
    print(f"  {name:<12} mean={m:.3f} sd={sd:.3f} min={lo:.3f} max={hi:.3f} {unit}")

print(f"Pesan: {msg!r}  SHA-256 (PC): {digest_pc.hex().upper()}")

if mode == "sw":
    s.write(b"P\n"); time.sleep(0.8)
    pub_hex = re.search(r"Public key\s*:\s*([0-9A-F]{128})", s.read(8192).decode()).group(1)
    pub = (int(pub_hex[:64], 16), int(pub_hex[64:], 16))
    s.reset_input_buffer()
    s.write(f"B{N}|{msg}\n".encode())
    rows = []; hdr_dev = None
    while True:
        line = readline(120)
        if line.startswith("#"): print(line)
        if line.startswith("i,"): hdr_dev = line
        if line and line[0].isdigit(): rows.append(line.split(","))
        if line.startswith("# SELESAI"): break
    hdr = hdr_dev or "i,cyc_hash,cyc_sign,ms_hash,ms_sign,ms_total,stack_bytes,heap_delta,sign_ok,verify_ok"
    # Verifikasi PC: beberapa tanda tangan baru dari perintah M (r||s tidak dikirim saat benchmark)
    n_valid = 0; dig_ok = True
    for _ in range(5):
        s.reset_input_buffer(); s.write(f"M{msg}\n".encode())
        dg, sig = readline(60).split("|")[:2]
        dig_ok &= bytes.fromhex(dg) == digest_pc
        r, sv = int(sig[:64], 16), int(sig[64:], 16)
        n_valid += verify(pub, digest_pc, r, sv) and not verify(pub, digest_pc, r, sv ^ 1)
        last = sig
    print(f"\nRingkasan software (N={len(rows)}):")
    summary("T_hash", [float(r[3]) for r in rows], "ms")
    summary("T_sign", [float(r[4]) for r in rows], "ms")
    summary("T_total", [float(r[5]) for r in rows], "ms")
    summary("stack", [float(r[6]) for r in rows], "B")
    if "heap_peak" in hdr.split(","): summary("heap_peak", [float(r[7]) for r in rows], "B")
    print(f"  digest board == PC : {dig_ok}")
    print(f"  verifikasi PC      : {n_valid}/5 valid (sig dirusak ditolak)")
    print(f"  contoh r||s        : {last}")

else:
    def cmd(c, t=2.0):
        s.reset_input_buffer(); s.write((c + "\n").encode())
        lines, t0 = [], time.time()
        while time.time() - t0 < t:
            try: l = readline(t)
            except TimeoutError: break
            if l: lines.append(l)
            if l and not l.startswith("[") and l != c: break
        return lines
    ai = cmd("AI")[-1]; rom = bytes.fromhex(ai.split("|")[0]); status = bytes.fromhex(ai.split("|")[1])
    page0 = bytes.fromhex(cmd("AR0")[-1])
    pub = (int(cmd("AR4")[-1], 16), int(cmd("AR5")[-1], 16))
    manid = status[7:9]
    print(f"ROM ID {rom.hex().upper()} | ManID {manid.hex().upper()} | pubkey di kurva: {on_curve(pub)}")
    hdr = "i,hash_us,send_us,wait_us,read_us,total_us,stack_op_bytes,verify_ok"
    rows, last = [], None
    for i in range(N):
        lines = cmd(f"AM0|{msg}", 3.0)
        st = int(re.search(r"\[STACK op\] (\d+)", "\n".join(lines)).group(1))
        lat = dict(re.findall(r"(\w+)=(\d+)", next(l for l in lines if l.startswith("[LAT")).split("|")[0]))
        dg, sig = lines[-1].split("|")
        r, sv = int(sig[:64], 16), int(sig[64:], 16)
        hw_msg = rom + page0 + bytes.fromhex(dg) + b"\x00" + manid   # pesan yang di-hash DS28E38
        ok = (bytes.fromhex(dg) == digest_pc and
              verify(pub, hashlib.sha256(hw_msg).digest(), r, sv) and
              not verify(pub, hashlib.sha256(hw_msg).digest(), r, sv ^ 1))
        rows.append([str(i + 1), lat["hash"], lat["send"], lat["wait"], lat["read"], lat["total"], str(st), str(int(ok))])
        last = sig
        print(",".join(rows[-1]))
    ram = next((l for l in lines if l.startswith("[RAM SIGN]")), "")
    print(f"\nRingkasan DS28E38 (N={len(rows)}):")
    for k, name in [(1, "T_hash"), (2, "T_send"), (3, "T_wait"), (4, "T_read"), (5, "T_total")]:
        summary(name, [int(r[k]) / 1000 for r in rows], "ms")
    summary("stack_op", [int(r[6]) for r in rows], "B")
    print(f"  {ram}")
    print(f"  verifikasi PC      : {sum(int(r[7]) for r in rows)}/{len(rows)} valid (digest cocok, sig dirusak ditolak)")
    print(f"  contoh r||s        : {last}")

if out:
    with open(out, "w") as f:
        f.write(hdr + "\n" + "\n".join(",".join(r) for r in rows) + "\n")
    print("CSV disimpan:", out)
s.close()
