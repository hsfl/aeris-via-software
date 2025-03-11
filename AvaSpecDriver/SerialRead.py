import serial

ser = serial.Serial('COM12', 115200, timeout=30)
file_path = "measurement_data.txt."

with (open(file_path, "ab") as file):
    print("Waiting for data...")
    while True:
            line = ser.readline().strip()

            if line == b"START_TRANSMISSION":
                  print("Receiving data...")

    while True:
        data = ser.read(4106)
        if data:
            file.write(data)
            print("Data received, saved to", file_path)
            break

ser.close()