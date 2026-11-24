# Catatan multimeter, kalibrasi 21 Nov 2026

Multimeter: SANWA CD800a, rentang mA DC 40 mA (resolusi 0,01 mA, akurasi katalog ±(2,2 % + 5 digit)), dipasang SERI antara sumber 5 V (USB-C ke PC) dan IN+ INA226.
Host: K2S terjaga (luar jendela), ST-Link dan USB-C host dicabut, POR dengan IN- dilepas.

Offset awal (INA226, IN- lepas, 60 s): -2,82 satuan, bus 5,233 V (data/kalibrasi/offset.csv).

## Kelompok terjaga, putaran 1: bacaan layar multimeter (acuan silang saja)

| Keadaan | Layar (mA) | Selisih thd tanpa beban (mA) | V implisit = dI x R (V) |
|---|---|---|---|
| tanpa beban | 12,54 | - | - |
| + 2,2 kOhm | 14,82 | 2,28 | 5,02 |
| + 1 kOhm | 17,40 | 4,86 | 4,86 |

Catatan: selisih 1 kOhm lebih rendah ~3 % dari V/R perkiraan (burden voltage multimeter / tegangan beban turun saat arus naik); masih dalam ketidakpastian multimeter. Perhitungan resmi memakai V_bus INA226 (selisih) tanpa bergantung pada layar ini.

## Pembacaan ULANG layar multimeter (kelompok terjaga), menggantikan tabel di atas

Tabel pertama (12,54 / 14,82 / 17,40 mA) DIBATALKAN sebagai acuan: selisih ~3 mA terhadap INA226 dan terhadap pengukuran ulang; penyebab tidak diketahui (kemungkinan kontak/penyetelan saat pengukuran pertama). Acuan sah adalah pengukuran ulang berikut.

| Keadaan | Layar (mA) | INA226 R_eff selisih 0,0821 (mA) | INA226 regresi multimeter 0,08285 (mA) |
|---|---|---|---|
| tanpa beban | 15,48 | 15,56 | 15,41 |
| + 2,2 kOhm | 17,71 | 17,93 | 17,77 |
| + 1 kOhm | 20,64 | 20,83 | 20,64 |

- dI multimeter: 2,23 dan 5,16 mA; V_bus/R: 2,39 dan 5,252 mA.
- Regresi melalui nol (acuan absolut multimeter): R_eff = 0,08285 Ohm, residu +0,42 / -0,31 / -0,01 %.
- Metode selisih (V_bus/R): R_eff = 0,0821 Ohm (1 kOhm 0,08246; 2,2 kOhm 0,08170). Dua metode berbeda 0,9 %.
- Regresi bebas: R = 0,0838 Ohm, intersep +0,21 mA (resolusi dan akurasi multimeter 0,05 mA + 2,2 %, intersep tidak signifikan).

## Rerata 3 kali pengukuran multimeter (dilaporkan user, menggantikan pembacaan tunggal di atas)

| Keadaan | Layar rerata (mA) | INA226 R_eff selisih 0,0821 (mA) | Selisih INA vs meter |
|---|---|---|---|
| tanpa beban | 15,48 (belum dilaporkan sebagai rerata 3 kali) | 15,56 | +0,5 % |
| + 2,2 kOhm | 17,68 | 17,93 | +1,4 % |
| + 1 kOhm | 20,51 | 20,83 | +1,6 % |

- Regresi absolut melalui nol (meter): R_eff = 0,08313 Ohm, residu +0,75 / -0,15 / -0,31 %.
- dI dari meter: 2,20 dan 5,03 mA (V_bus/R: 2,39 dan 5,252 mA); R_eff dari dI meter saja 0,0885 / 0,0861 Ohm, kurang teliti karena pengurangan dua pembacaan resolusi 0,01 mA, akurasi 2,2 % + 5 digit.

Koreksi: rerata 3 kali multimeter tanpa beban = 15,38 mA (bukan 15,48 yang pembacaan tunggal). Angka ini yang dipakai di regresi final.
