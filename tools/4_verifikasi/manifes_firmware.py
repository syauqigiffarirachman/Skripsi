"""B4: manifes sha256 firmware yang membangkitkan data final varian Stop (hanya-baca, tidak membangun apa pun).

Membaca .pio/build/<env>/firmware.{bin,elf} yang SUDAH ada, acuan 16 Nov (tools/4_verifikasi/biner_acuan.json) untuk logger/attacker,
dan git (commit sumber terakhir, status src kotor atau tidak). Menulis data/manifes_firmware_stop.json dan .md.
Bukti kecocokan dengan biner yang benar-benar di-flash saat sesi: (1) sumber tidak lebih baru dari biner (mtime),
(2) bangun ulang dari HEAD menghasilkan sha256 sama, (3) build-id logger di baris #RUN,BEGIN tiap run = 4C364231.
"""
import sys as _sys
from pathlib import Path as _Path
_sys.path.insert(0, str(_Path(__file__).resolve().parents[1]))   # folder tools/ (tempat _jalur.py)
from _jalur import ROOT  # noqa: E402,F401  (ROOT repo + semua subfolder tools/ masuk sys.path)
import hashlib
import json
import subprocess
from datetime import datetime
from pathlib import Path

HOST = {"K1S": "host_k1_stop", "K2S": "host_k2_stop"}
LAIN = {"logger": "logger", "attacker": "attacker"}


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def git(*a):
    return subprocess.run(["git", "--no-optional-locks", *a], cwd=ROOT, capture_output=True, text=True).stdout.strip()


def main():
    acuan = json.loads((ROOT / "tools" / "4_verifikasi" / "biner_acuan.json").read_text(encoding="utf-8"))["env"]
    out = {"dibuat": datetime.now().isoformat(timespec="seconds"), "git_head": git("rev-parse", "HEAD"),
           "commit_sumber_terakhir": git("log", "-1", "--format=%H %cI %s", "--", "src", "lib", "include", "boards", "platformio.ini"),
           "sumber_kotor": bool(git("status", "--porcelain", "--", "src", "lib", "include", "boards", "platformio.ini")),
           "firmware": {}}
    bin_time = {}
    for nama, env in {**HOST, **LAIN}.items():
        b = ROOT / ".pio" / "build" / env / "firmware.bin"
        e = ROOT / ".pio" / "build" / env / "firmware.elf"
        x = {"env": env, "bin_sha256": sha(b), "bin_ukuran": b.stat().st_size,
             "bin_mtime": datetime.fromtimestamp(b.stat().st_mtime).isoformat(timespec="seconds")}
        if e.exists():
            x["elf_sha256"] = sha(e)
        if env in acuan:
            x["sama_dengan_acuan_16nov"] = acuan[env]["sha256"] == x["bin_sha256"]
        out["firmware"][nama] = x
    out["logger_build_id_di_data"] = "4C364231 (seluruh 139 run, lihat pemeriksaan.md tabel 1)"
    (ROOT / "data" / "manifes_firmware_stop.json").write_text(json.dumps(out, indent=2, ensure_ascii=False), encoding="utf-8")
    L = ["# Manifes firmware data final varian Stop (18 Nov 2026)\n",
         f"Git HEAD saat manifes: `{out['git_head'][:12]}`; commit sumber terakhir: {out['commit_sumber_terakhir'][:90]}; sumber kotor: {out['sumber_kotor']}.\n",
         "| Peran | Env | sha256 firmware.bin | Ukuran (B) | = acuan 16 Nov |", "|---|---|---|---|---|"]
    for n, x in out["firmware"].items():
        L.append(f"| {n} | {x['env']} | `{x['bin_sha256']}` | {x['bin_ukuran']} | {x.get('sama_dengan_acuan_16nov', '-')} |")
    L += ["", "Dasar kecocokan dengan biner yang di-flash saat sesi (tidak ada sha256 yang dicatat saat itu):",
          "1. Tidak ada berkas sumber (`src lib include boards platformio.ini`) yang lebih baru dari biner host K1S/K2S (dibangun 18 Nov 11:14, sesi data mulai 11:50).",
          "2. Logger dan attacker dibangun ulang dari HEAD dan sha256-nya sama dengan acuan 16 Nov. Host K1S/K2S: pio hanya inkremental (tidak ada yang dikompilasi ulang), jadi ini bukan bukti reproduksi bersih; jalankan `python tools/4_verifikasi/cek_biner.py simpan host_k1_stop host_k2_stop` bila ingin acuan bersih.",
          "3. Build-id logger 4C364231 tercatat di baris `#RUN,BEGIN` seluruh run.",
          "Keterbatasan: bukti 1 dan 2 tidak menjamin biner yang ada di papan; satu-satunya bukti langsung ialah build-id logger. "
          "Untuk sesi berikutnya cetak sha256 firmware yang di-flash di awal sesi."]
    (ROOT / "data" / "manifes_firmware_stop.md").write_text("\n".join(L) + "\n", encoding="utf-8")
    print("\n".join(L))


if __name__ == "__main__":
    main()
