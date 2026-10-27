import subprocess, time, sys

print("=== ESP32 CYD AUTO-FLASHER (03_Khung_Anh_Dien_Thoai) ===")
print("Dang lang nghe cong COM3... Xin hay bam to hop nut tren mach:")
print("1. Nhan va GIU nut BOOT (nut o canh duoi)")
print("2. Bam va NHA nut RST (nut o canh tren) 1 lan")
print("3. NHA nut BOOT ra\n")

build_dir = r"C:\Users\nguye\AppData\Local\arduino\sketches\F79C8217A7A6506C52D03C94B13AEE6A"
bootloader = f"{build_dir}\\03_Khung_Anh_Dien_Thoai.ino.bootloader.bin"
partitions = f"{build_dir}\\03_Khung_Anh_Dien_Thoai.ino.partitions.bin"
boot_app0  = f"{build_dir}\\boot_app0.bin"
app_bin    = f"{build_dir}\\03_Khung_Anh_Dien_Thoai.ino.bin"

cmd = [
    r"C:\Users\nguye\.local\bin\uv.exe", "tool", "run", "esptool",
    "--port", "COM3",
    "--baud", "921600",
    "--before", "no-reset",
    "--after", "hard-reset",
    "write-flash", "-z",
    "--flash_mode", "dio",
    "--flash_freq", "80m",
    "--flash_size", "4MB",
    "0x1000", bootloader,
    "0x8000", partitions,
    "0xe000", boot_app0,
    "0x10000", app_bin
]

start_time = time.time()
while time.time() - start_time < 90:
    res = subprocess.run(cmd, capture_output=True, text=True)
    if "Hash of data verified" in res.stdout or res.returncode == 0:
        print("\n>>> NAP FIRMWARE THANH CONG 100%! <<<")
        print(res.stdout)
        sys.exit(0)
    time.sleep(0.3)

print("\nHet thoi gian cho (90 giay).")
sys.exit(1)
