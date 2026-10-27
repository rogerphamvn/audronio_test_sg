import subprocess
import sys
import time
import serial

sys.stdout.reconfigure(encoding='utf-8', errors='replace')

cmd_erase = [
    r"C:\Users\nguye\.local\bin\uv.exe", "run", "--with", "esptool", "esptool",
    "--chip", "esp32",
    "--port", "COM3",
    "--baud", "460800",
    "--before", "no-reset",
    "--after", "hard-reset",
    "erase-flash"
]

def listen_and_erase():
    print("================================================================")
    print("ĐANG CHỜ TÍN HIỆU ĐỂ XÓA TRẮNG BỘ NHỚ FLASH ESP32 (60 GIÂY)...")
    print("Thao tác: Giữ nút BOOT -> Bấm nhấp nút RST -> Thả nút BOOT")
    print("================================================================")
    
    try:
        ser = serial.Serial('COM3', 115200, timeout=0.1)
    except Exception as e:
        print("Lỗi mở COM3:", e)
        return

    start = time.time()
    while time.time() - start < 60:
        line = ser.readline()
        if line:
            text = line.decode('utf-8', errors='replace').strip()
            print("ESP32:", text)
            if "boot:0x3" in text or "waiting for download" in text:
                print("\n[BẮT ĐƯỢC TÍN HIỆU!] Đang tiến hành xóa sạch Flash về trạng thái xuất xưởng...")
                ser.close()
                time.sleep(0.3)
                res = subprocess.run(cmd_erase, text=True, capture_output=True, encoding='utf-8', errors='replace')
                print(res.stdout)
                if res.stderr:
                    print(res.stderr)
                if "Chip erase completed successfully" in res.stdout or res.returncode == 0:
                    print("\n>>> ĐÃ XÓA TRẮNG FLASH THÀNH CÔNG 100%! BO MẠCH VỀ XUẤT XƯỞNG! <<<")
                    return
                else:
                    print("Lỗi xóa flash:", res.stderr)
                    return
        time.sleep(0.02)

    ser.close()
    print("Hết thời gian theo dõi (60s).")

if __name__ == "__main__":
    listen_and_erase()
