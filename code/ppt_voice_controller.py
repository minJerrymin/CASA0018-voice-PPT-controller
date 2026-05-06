import time
import serial
import pyautogui

PORT = "COM6"      
BAUD_RATE = 115200

COMMAND_TO_KEY = {
    "CMD:NEXT": "right",
    "CMD:BACK": "left",
    "CMD:ESC": "esc",
}

pyautogui.FAILSAFE = True
pyautogui.PAUSE = 0.05


def main():
    print(f"Opening serial port: {PORT}")

    with serial.Serial(PORT, BAUD_RATE, timeout=1) as ser:
        time.sleep(2)

        print("Listening for Arduino commands...")
        print("Only CMD:NEXT / CMD:BACK / CMD:ESC will control PowerPoint.")
        print("Keep PowerPoint slideshow focused.")
        print("Press Ctrl+C to stop.\n")

        while True:
            raw = ser.readline()
            line = raw.decode("utf-8", errors="ignore").strip()

            if not line:
                continue

            print("Serial:", line)

            if line in COMMAND_TO_KEY:
                key = COMMAND_TO_KEY[line]
                print("Command detected:", line)
                print("Pressing key:", key)
                pyautogui.press(key)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nStopped.")