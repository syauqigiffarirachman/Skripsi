"""Jalankan jendela pengamatan T_obs (Bab 3.5.2) pada firmware softwaretes_mbedtls:
  normal   : skenario A / K1 -> firmware membangkitkan 1 permintaan sah tiap `periode` ms
  attack   : skenario B / K3 -> skrip ini menjadi NODE PENYERANG eksternal lewat USB (perintah A)
  internal : skenario B / K3 -> penyerang di dalam firmware, permintaan selalu tersedia (perintah F);
             tanpa USB selama jendela, dianjurkan untuk pengukuran arus yang bersih

Pemakaian:
  python tools/window_test.py normal   COM12 60 10000 data/k1_window_normal.csv
  python tools/window_test.py attack   COM12 60 data/k3_window_attack.csv [digest_hex64]
  python tools/window_test.py internal COM12 60 data/k3_window_internal.csv

PENTING (mode attack): core STM32duino 4.21 di STM32F103 HARD-FAULT bila antrean RX USB (192 B) penuh.
Karena itu penyerang ber-flow-control: firmware mengirim '.' tiap mulai memproses sebuah permintaan, dan
skrip ini mengirim TEPAT SATU baris per '.' (permintaan pertama dikirim begitu jendela mulai). Antrean tidak
pernah > 1 baris, tetapi selalu ada permintaan menunggu saat tanda tangan selesai, sehingga CPU tetap jenuh.

Keluaran: CSV latensi tiap permintaan (i,t_start_ms,cyc_sign,ms_sign), berkas .log berisi seluruh
keluaran firmware (ringkasan %S_cpu, T_obs, jumlah permintaan), dan ringkasan di layar.
Untuk pengukuran arus: nyalakan pencatat catu daya DULU, lalu jalankan skrip ini; pin PB10 (WINDOW)
naik tepat saat jendela mulai dan turun saat selesai.
"""
import os, sys, time
import serial

DIGEST = "D72C103978443C67EAD7542910F0EBF1C8BAF63182C07E062BCC6FC2E9A8A50E"   # SHA-256 konstan (variabel kontrol)
PEND = bytearray()                                                             # byte sisa yang belum jadi baris


def readline(s, timeout):
    t0 = time.time()
    while time.time() - t0 < timeout:
        i = PEND.find(b"\n")
        if i >= 0:
            line = bytes(PEND[:i]); del PEND[:i + 1]
            return line.decode(errors="replace").strip()
        PEND.extend(s.read(max(1, s.in_waiting)))
    raise TimeoutError("timeout menunggu keluaran firmware")


def main():
    if len(sys.argv) < 5:
        sys.exit(__doc__)
    mode, port, secs = sys.argv[1], sys.argv[2], int(sys.argv[3])
    out = digest = None
    if mode == "normal":
        period = int(sys.argv[4]); out = sys.argv[5] if len(sys.argv) > 5 else None; cmd = f"N{secs},{period}"
    elif mode == "attack":
        out = sys.argv[4]; digest = (sys.argv[5] if len(sys.argv) > 5 else DIGEST).upper(); cmd = f"A{secs}"
        assert len(digest) == 64 and all(c in "0123456789ABCDEF" for c in digest), "digest harus 64 hex"
    elif mode == "internal":
        out = sys.argv[4]; cmd = f"F{secs}"
    else:
        sys.exit(__doc__)

    s = serial.Serial(port, 115200, timeout=0.3, write_timeout=2); s.dtr = True
    time.sleep(0.5); s.reset_input_buffer()
    log = []
    print(f"> {cmd}")
    s.write((cmd + "\n").encode())

    # tunggu firmware siap (pemanasan heap bisa memakan ~5 s pada perintah pertama setelah reset)
    while True:
        l = readline(s, 60); log.append(l); print(l)
        if l.startswith("# MULAI"):
            t_start = time.monotonic(); break
        if l == "-1":
            sys.exit("firmware menolak perintah")

    if mode == "attack":
        req = f"S{digest}\n".encode()
        s.write(req); sent = 1                                   # permintaan pertama
        while True:
            data = bytes(PEND) + s.read(max(1, s.in_waiting)); PEND.clear()
            i = data.find(b"#")                                  # teks hasil dimulai dengan '#': jendela selesai
            acks = (data if i < 0 else data[:i]).count(b".")
            for _ in range(acks):
                if time.monotonic() - t_start < secs:
                    s.write(req); sent += 1                      # tepat 1 baris per ack
            if i >= 0:
                PEND.extend(data[i:]); break
            if time.monotonic() - t_start > secs + 60:
                sys.exit("firmware tidak mengakhiri jendela")
        print(f"penyerang: {sent} permintaan dikirim (1 per ack) selama {secs} s")

    rows, in_csv, hdr = [], False, "i,t_start_ms,cyc_sign,ms_sign"
    while True:
        l = readline(s, secs + 120); log.append(l)
        if l.startswith("i,t_start_ms"): in_csv = True; hdr = l; continue
        if in_csv and l and l[0].isdigit(): rows.append(l.split(","))
        elif l.startswith("#"): in_csv = False; print(l)
        if l.startswith("# SELESAI") and not l.startswith("# SELESAI JENDELA"):
            break
    s.close()

    if out:
        os.makedirs(os.path.dirname(out) or ".", exist_ok=True)
        with open(out, "w") as f:
            f.write(hdr + "\n" + "\n".join(",".join(r) for r in rows) + "\n")
        with open(os.path.splitext(out)[0] + ".log", "w") as f:
            f.write(f"# perintah: {cmd}\n" + "\n".join(log) + "\n")
        print(f"CSV disimpan: {out}  (+ {os.path.splitext(out)[0]}.log)")


if __name__ == "__main__":
    main()
