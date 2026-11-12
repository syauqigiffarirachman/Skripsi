# Patch upload lewat bootloader serial (stm32flash) untuk board attacker (STM32F030C8T6).
#
# Platform ststm32 (builder/main.py) mengganti UPLOADERFLAGS secara hardcoded untuk
# upload_protocol="serial", sehingga opsi "upload_flags" biasa di platformio.ini TIDAK
# terpakai pada jalur ini. Skrip ini menambahkan argumen "-i" (GPIO sequence) ke stm32flash
# supaya BOOT0 dan NRST ditoggle otomatis lewat DTR/RTS CH340 (sirkuit auto-reset papan
# attacker), tanpa perlu memindah jumper BOOT0 / menekan reset manual tiap upload.
#
# Sequence "-i masuk:keluar" (lihat stm32flash --help):
#   Masuk ke bootloader : rts,-dtr,dtr   -> RTS=1 (BOOT0 tinggi), DTR=0 (reset aktif),
#                                           DTR=1 (lepas reset -> boot ke bootloader)
#   Keluar (jalankan FW): -rts           -> RTS=0 (BOOT0 rendah kembali, setelah "go" dieksekusi)
# Kalau polaritas sirkuit sebenarnya terbalik, ganti ke "dtr,-rts,rts:-dtr" (tukar rts<->dtr)
# atau balik setiap tanda '-'.
Import("env")

# PENTING: harus Prepend, bukan Append. UPLOADERFLAGS hardcoded di platform diakhiri "-w"
# TANPA nama file (nama file ditambahkan belakangan oleh UPLOADCMD lewat token $SOURCE).
# Kalau argumen kita ditaruh di belakang (Append), "-i ..." akan dibaca stm32flash sebagai
# nama file untuk "-w" (parse error / dump usage). Prepend menaruhnya sebelum "-g ... -w".
env.Prepend(UPLOADERFLAGS=["-i", "rts,-dtr,dtr:-rts"])
