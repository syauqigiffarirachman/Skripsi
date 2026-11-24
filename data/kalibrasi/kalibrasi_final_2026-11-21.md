# Kalibrasi INA226 final, 21 November 2026 (kelompok terjaga, 12-21 mA)

## Parameter untuk analisis
| Parameter | Nilai | Dasar |
|---|---|---|
| R_shunt efektif | **0,0821 Ohm** (faktor koreksi arus 1,218; nominal modul 0,100) | rerata 6 selisih dI = V_bus / R_nominal (1 kOhm x3, 2,2 kOhm x3), `hasil_selisih_terjaga_2026-11-21.txt` |
| Offset nol | **-2,83 satuan** (2,5 uV/satuan) | awal -2,820 dan akhir -2,833 (bus 5,23-5,25 V), selisih 0,013 satuan |
| u_kal | **1,5 %** | metode selisih 1,2 %; regresi absolut multimeter memberi 0,0833 Ohm (selisih 1,4 %) |

Perintah: `analisis_d.py --stop --rshunt 0.0821 --offset -2.83 --u-rel 0.015`; `gambar_bab4_stop.py --shunt 0.0821`.

## Hasil per resistor
| Resistor | R_eff per putaran (Ohm) | Rerata | Sebaran maks-min |
|---|---|---|---|
| 1 kOhm (dX ~173 satuan) | 0,08246 / 0,08235 / 0,08256 | 0,08246 | 0,25 % |
| 2,2 kOhm (dX ~78 satuan) | 0,08123 / 0,08287 / 0,08099 | 0,08170 | 2,30 % |

## Kriteria penerimaan (ditetapkan sebelum pengukuran)
1. Sebaran per resistor < 1 %: 1 kOhm LULUS; **2,2 kOhm TIDAK TERPENUHI (2,3 %)**. Dilaporkan apa adanya. Sebaran konsisten dengan kebisingan baseline terukur (sd antar baseline ~0,9 satuan; galat +-1,1 satuan pada dX 78 satuan = +-1,4 %, jadi sebaran wajar tiga rekaman 2-2,5 %); kriteria < 1 % terlalu ketat untuk sinyal 2,2 kOhm. Tidak ada data yang dibuang.
2. R_eff antar resistor < 2 %: LULUS (0,0825 vs 0,0817, beda 0,9 %).
3. Antar kelompok (terjaga vs Stop): TIDAK DINILAI (kelompok Stop tidak dikerjakan, keputusan user 21 Nov).

## Pemeriksaan silang multimeter SANWA CD800a (rentang 40 mA, rerata 3 kali; +-(2,2 % + 5 digit))
| Keadaan | Multimeter | INA226 @ 0,0821 | Selisih |
|---|---|---|---|
| tanpa beban | 15,38 mA | 15,56 mA | +1,1 % |
| + 2,2 kOhm | 17,68 mA | 17,93 mA | +1,4 % |
| + 1 kOhm | 20,51 mA | 20,83 mA | +1,6 % |

Regresi absolut multimeter melalui nol: R_eff = 0,0833 Ohm, residu +0,26 / +0,01 / -0,15 %.

## Keterbatasan yang harus ditulis di naskah
- Kalibrasi diverifikasi pada 12-21 mA (terjaga); linearitas sampai +-2 mA (Stop) diasumsikan (INA226 linear; offset diukur terpisah dan stabil).
- Acuan: V_bus INA226 (u 0,5 %, konservatif), resistor 1 % nominal, bukan nilai ukur multimeter.
- Pembacaan multimeter pertama (12,54 / 14,82 / 17,40 mA) dibatalkan (selisih ~3 mA dari pengukuran ulang; penyebab tidak diketahui); lihat `catatan_multimeter_2026-11-21.md`.
- Dua berkas titik awal (board baru disolder, panas) disisihkan: `titik_PANAS_B0_R1k_2026-11-21.csv`; dua offset tidak sah (bus 0,35 V dan 0,02 V, daya sumber terputus) disisihkan sebagai `offset_TIDAKSAH_*`. Baseline setelah dingin tidak bergeser (B0 508,18 vs B1 507,79).
- Hasil alternatif (hanya 1 kOhm, R_eff 0,0825): `hasil_selisih_terjaga_hanya1k_2026-11-21.txt`; tidak dipilih (keputusan user: opsi simpan semua data).
