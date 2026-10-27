import sys, os
from dulwich import porcelain

# Đọc token từ biến môi trường GITHUB_TOKEN hoặc từ tham số dòng lệnh
token = os.environ.get("GITHUB_TOKEN")
if not token and len(sys.argv) > 1:
    token = sys.argv[1].strip()

if not token:
    print("\n[!] CHƯA CÓ GITHUB TOKEN ĐỂ XÁC THỰC QUYỀN GHI.")
    print("Để đẩy code lên kho lưu trữ riêng hoặc bảo mật trên GitHub, bạn cần GitHub Personal Access Token.")
    print("Cách tạo nhanh:")
    print("1. Truy cập: https://github.com/settings/tokens")
    print("2. Bấm 'Generate new token (classic)'")
    print("3. Tích chọn quyền: [x] repo (Full control of private repositories)")
    print("4. Bấm 'Generate token' và copy mã token (dạng: ghp_xxxxxxxxxxxxxxxxxxxx)")
    print("\nSau đó chạy lệnh:")
    print("  python tools/push_repo.py <MÃ_TOKEN_CỦA_BẠN>\n")
    sys.exit(1)

url = f"https://{token}@github.com/rogerphamvn/audronio_test_sg.git"
print("\nĐang kết nối và đẩy mã nguồn lên GitHub...")
try:
    porcelain.push(".", url, "refs/heads/main:refs/heads/main")
    print("\n🎉 ĐẨY LÊN GITHUB THÀNH CÔNG 100%!")
    print("👉 Xem kho mã nguồn tại: https://github.com/rogerphamvn/audronio_test_sg\n")
except Exception as e:
    print("\n[X] Lỗi khi đẩy lên GitHub:", e)
    print("Xin hãy kiểm tra lại quyền của Token hoặc kiểm tra xem kho https://github.com/rogerphamvn/audronio_test_sg có tồn tại và bạn có quyền ghi không.")
    sys.exit(1)
