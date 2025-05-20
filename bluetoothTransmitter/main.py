import serial
import time
import keyboard
import hid

# Pro Controller (nintendo stuff): VID = 0x057e, PID = 0x2009
PRO_CON_VID = 0x057e
PRO_CON_PID = 0x2009

JOY_STICK_SCALAR = 1 / .7

CONTROLLER_DEADBAND = 0.2

FLYWHEEL_DEADBAND = 0.1

# this is 0x30 joystick encoding, thanks chatgpt for this
def read_stick(data, deadband=True):
    x = data[0] | ((data[1] & 0x0F) << 8)
    y = (data[1] >> 4) | (data[2] << 4)

    x = apply_deadband(x/2048 - 1, CONTROLLER_DEADBAND if apply_deadband else 0) * JOY_STICK_SCALAR
    y = apply_deadband(y/2048 - 1, CONTROLLER_DEADBAND if apply_deadband else 0) * JOY_STICK_SCALAR

    x = max(min(x, 1), -1)
    y = max(min(y, 1), -1)

    return x, y


def parse_data(data, deadband=True):
    buttons_bytes = data[3:6]
    left_x, left_y = read_stick(data[6:9], deadband)
    right_x, right_y = read_stick(data[9:12], deadband)

    return {
        "buttons_raw": buttons_bytes,
        "left_joystick": (left_x, left_y),
        "right_joystick": (right_x, right_y),

        "y": buttons_bytes[0] & 1 != 0,
        "x": buttons_bytes[0] & 2 != 0,
        "b": buttons_bytes[0] & 4 != 0,
        "a": buttons_bytes[0] & 8 != 0,

        "right_trigger": buttons_bytes[0] & 128 != 0,
        "left_trigger": buttons_bytes[2] & 128 != 0
    }

def apply_deadband(x, deadband=CONTROLLER_DEADBAND):
    return 0 if abs(x) < deadband else x;

device = hid.Device(vid=PRO_CON_VID, pid=PRO_CON_PID)
print(f"Pro Controller Connected, pid:{PRO_CON_VID}")
device.nonblocking = True

bt_port = "/dev/tty.StokesTheorem"
baud_rate = 115200

try:
    ser = serial.Serial(bt_port, baud_rate)
    ser.dtr = False
except Exception as e:
    print(f"Error opening connection: {e}")
    exit() # consider this error caught B)

def send(message : str):
    print(f"Sent [{message}]")
    ser.write((message+"\n").encode())
    ser.flush()


print("Connecting ... ")

time.sleep(2)
send("connect from python")
if ser.read_until(b'\n').decode().strip() == "connected from esp":
    print("Connected!")

print(f"Bluetooth connection established, port:{bt_port}, baud:{baud_rate}")

keyboard.add_hotkey("space", lambda: send("stop"))

flywheel_speed = 0.0
flywheel_accel = 0.05
break_accel = 0.1

data = parse_data(device.read(64))

while True:
    try:
        data = parse_data(device.read(64))
    except Exception:
        continue

    _, drive = data["left_joystick"]
    turn, _ = data["right_joystick"]

    accel = data["right_trigger"]
    decel = data["left_trigger"]
    break_ = data["b"]

    if break_ and (flywheel_speed < FLYWHEEL_DEADBAND) and (flywheel_speed > -FLYWHEEL_DEADBAND):
        flywheel_speed = 0.0

    if break_ and flywheel_speed != 0.0:
        flywheel_speed += -break_accel if (flywheel_speed > 0) else break_accel
    else:
        if accel:
            flywheel_speed += flywheel_accel
        if decel:
            flywheel_speed -= flywheel_accel

    flywheel_speed = max(min(flywheel_speed, 1), -1)

    send(f"f,{flywheel_speed}")

    send(f"d,{drive},{turn}")

    if ser.in_waiting > 0:
        line = ser.read_until(b'\n').decode().strip()
        print(f"ESP32 : [{line}]")

    # time.sleep(0.05)
