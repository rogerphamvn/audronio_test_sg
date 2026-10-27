import sys, os

token = os.environ.get("GITHUB_TOKEN")
if not token and len(sys.argv) > 1:
    token = sys.argv[1].strip()

if not token:
    print("Error: No GITHUB_TOKEN provided.")
    sys.exit(1)

from dulwich import porcelain

url = f"https://{token}@github.com/rogerphamvn/audronio_test_sg.git"
print("Connecting and pushing repository to GitHub...")
try:
    porcelain.push(".", url, "refs/heads/main:refs/heads/main")
    print("SUCCESS: Pushed to GitHub successfully!")
    print("View repository at: https://github.com/rogerphamvn/audronio_test_sg")
except Exception as e:
    print("Error pushing to GitHub:", e)
    sys.exit(1)
