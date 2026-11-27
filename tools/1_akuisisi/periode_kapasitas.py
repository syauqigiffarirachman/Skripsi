"""Periode attacker untuk skenario C (kapasitas, rho = 1), 27 Nov 2026.

Dari sesi uji pendahuluan (run A dan B, tanpa IDLE) per arsitektur:
  t_sign  = rerata latensi nyata run A sah, ditimbang jumlah tanda tangan (lat_n)  -> periode nyata skenario C
  lambda_B = sum N_dtg / sum T_obs_log run B sah (laju attacker menurut kristal logger; nominal 10/s)
  k        = 10 / lambda_B  (k > 1: jam HSI attacker lambat, periode nyata lebih panjang dari perintah)
  periode_kirim_us = round(t_sign / k): nilai yang dikirim ke attacker ('p<us>') agar periode NYATA = t_sign.
Hanya run tanpa 'masalah' yang dipakai. Tanpa run A atau B sah -> ValueError (tidak memakai k = 1 diam-diam).

Pakai:
  python tools/1_akuisisi/periode_kapasitas.py data/runs/<sesi_K2S> data/runs/<sesi_K1S>
      -> data/kapasitas_periode.json (arsitektur yang sama: sesi terakhir menimpa)
Desain: docs/superpowers/specs/2026-11-27-skenario-c-kapasitas-design.md
"""
import sys as _sys, pathlib as _pl  # noqa: E401
_sys.path.insert(0, str(next(p for p in _pl.Path(__file__).resolve().parents if (p / "_jalur.py").is_file())))  # folder tools/
from _jalur import ROOT  # noqa: E402
import argparse
import csv
import json
from pathlib import Path

LAJU_B_NOMINAL = 10.0
JSON_BAWAAN = ROOT / "data" / "kapasitas_periode.json"


def _f(r, k):
    v = r.get(k)
    return float(v) if v not in (None, "") else None


def hitung_periode(rows):
    """rows: baris ringkasan.csv (dict). Mengembalikan {arch: {t_sign_ms, n_sign, lambda_B, k, periode_kirim_us,
    periode_nyata_us}}. ValueError bila suatu arsitektur tidak punya run A sah atau run B sah."""
    per = {}
    for r in rows:
        if r.get("masalah") or r.get("scen") not in ("A", "B") or not r.get("arch"):
            continue
        per.setdefault(r["arch"], []).append(r)
    if not per:
        raise ValueError("tidak ada run A/B sah")
    out = {}
    for arch, rs in sorted(per.items()):
        a = [(_f(r, "lat_mean_ms_nyata"), _f(r, "lat_n")) for r in rs if r["scen"] == "A"]
        a = [(m, n) for m, n in a if m is not None and n]
        b = [(_f(r, "N_dtg"), _f(r, "T_obs_log_s")) for r in rs if r["scen"] == "B"]
        b = [(n, t) for n, t in b if n is not None and t]
        if not a:
            raise ValueError(f"{arch}: tidak ada run A sah (latensi)")
        if not b:
            raise ValueError(f"{arch}: tidak ada run B sah (kalibrasi jam attacker)")
        n_sign = sum(n for _, n in a)
        t_sign = sum(m * n for m, n in a) / n_sign
        lam = sum(n for n, _ in b) / sum(t for _, t in b)
        k = LAJU_B_NOMINAL / lam
        out[arch] = {"t_sign_ms": t_sign, "n_sign": int(n_sign), "lambda_B": lam, "k": k,
                     "periode_kirim_us": round(t_sign * 1000 / k), "periode_nyata_us": round(t_sign * 1000)}
    return out


def baca_sesi(sesi):
    with (Path(sesi) / "ringkasan.csv").open(encoding="utf-8") as fh:
        return list(csv.DictReader(fh))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("sesi", nargs="+", help="folder data/runs/<sesi> uji pendahuluan")
    ap.add_argument("--keluar", default=str(JSON_BAWAAN))
    args = ap.parse_args()
    hasil = {}
    for s in args.sesi:
        for arch, h in hitung_periode(baca_sesi(s)).items():
            h["sesi"] = Path(s).name
            hasil[arch] = h
    Path(args.keluar).write_text(json.dumps(hasil, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    for arch, h in hasil.items():
        print(f"{arch}: t_sign={h['t_sign_ms']:.3f} ms (n={h['n_sign']})  lambda_B={h['lambda_B']:.4f}/s  "
              f"k={h['k']:.5f}  kirim p{h['periode_kirim_us']}  (nyata {h['periode_nyata_us']} us)  [{h['sesi']}]")
    print(f"-> {args.keluar}")


if __name__ == "__main__":
    main()
