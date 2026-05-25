import time
import sys

print("=== QGC Script Runner Test ===")
print(f"Python version: {sys.version}")
print(f"Script path: {__file__}")
print()

for i in range(1, 6):
    print(f"[{i}/5] Running... timestamp: {time.strftime('%H:%M:%S')}")
    time.sleep(1)

print()
print("Test completed successfully!")
