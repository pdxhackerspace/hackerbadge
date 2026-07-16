# PDX Hackerspace Hackerbadge — CircuitPython hardware test
#
# Exercises the 37 WS2812B LEDs (colors + patterns) and the LSM6DSO IMU.
#
# Pinout (Seeed Xiao ESP32-S3):
#   LED data  D10
#   I2C SDA   D1
#   I2C SCL   D0
#   LSM6DSO   0x6A
#
# Setup:
#   1. Install CircuitPython for Seeed Xiao ESP32-S3
#   2. Copy from the Adafruit CircuitPython library bundle into CIRCUITPY/lib/:
#        neopixel.mpy
#        adafruit_pixelbuf.mpy   (dependency of neopixel on some builds)
#        adafruit_lsm6ds/
#        adafruit_bus_device/
#        adafruit_register/
#   3. Copy this file to CIRCUITPY/code.py
#
# After boot the badge cycles solid colors and patterns. Serial (REPL / console)
# logs print accel/gyro samples. The Accel Tilt effect maps orientation to LEDs.

import math
import random
import time

import board
import busio
import neopixel
from adafruit_lsm6ds.lsm6dsox import LSM6DSOX

NUM_LEDS = 37
LED_PIN = board.D10
BRIGHTNESS = 0.4
EFFECT_SECONDS = 8.0
IMU_LOG_SECONDS = 2.0
FRAME_DT = 0.04  # ~25 FPS for animated effects

# Badge I2C is on D0/D1, not the Xiao default SDA/SCL pads.
i2c = busio.I2C(board.D0, board.D1)

pixels = neopixel.NeoPixel(
    LED_PIN,
    NUM_LEDS,
    brightness=BRIGHTNESS,
    auto_write=False,
    pixel_order=neopixel.GRB,
)

imu = None
try:
    imu = LSM6DSOX(i2c, address=0x6A)
    print("IMU: LSM6DSO ready at 0x6A")
except Exception as exc:  # pylint: disable=broad-except
    print("IMU: init failed —", exc)
    print("IMU: check wiring / I2C pullups; LED tests will still run")


def _clamp(value, lo, hi):
    return lo if value < lo else hi if value > hi else value


def _wheel(pos):
    """0–255 -> rainbow RGB."""
    pos = pos & 0xFF
    if pos < 85:
        return (255 - pos * 3, pos * 3, 0)
    if pos < 170:
        pos -= 85
        return (0, 255 - pos * 3, pos * 3)
    pos -= 170
    return (pos * 3, 0, 255 - pos * 3)


def solid(color, duration):
    pixels.fill(color)
    pixels.show()
    _idle(duration)


def rainbow(duration):
    start = time.monotonic()
    offset = 0
    while time.monotonic() - start < duration:
        for i in range(NUM_LEDS):
            pixels[i] = _wheel(offset + (i * 256 // NUM_LEDS))
        pixels.show()
        offset = (offset + 4) & 0xFF
        _maybe_log_imu()
        time.sleep(FRAME_DT)


def color_wipe(duration):
    colors = (
        (255, 0, 0),
        (0, 255, 0),
        (0, 0, 255),
        (255, 255, 255),
    )
    start = time.monotonic()
    while time.monotonic() - start < duration:
        for color in colors:
            for i in range(NUM_LEDS):
                if time.monotonic() - start >= duration:
                    return
                pixels[i] = color
                pixels.show()
                _maybe_log_imu()
                time.sleep(0.04)


def scan(duration):
    start = time.monotonic()
    pos = 0
    direction = 1
    width = 3
    while time.monotonic() - start < duration:
        pixels.fill((0, 0, 0))
        for w in range(width):
            idx = pos + w
            if 0 <= idx < NUM_LEDS:
                pixels[idx] = (0, 180, 255)
        pixels.show()
        pos += direction
        if pos <= 0:
            pos = 0
            direction = 1
        elif pos >= NUM_LEDS - width:
            pos = NUM_LEDS - width
            direction = -1
        _maybe_log_imu()
        time.sleep(FRAME_DT)


def twinkle(duration, random_color=False):
    start = time.monotonic()
    levels = [0] * NUM_LEDS
    colors = [(0, 0, 0)] * NUM_LEDS
    while time.monotonic() - start < duration:
        for i in range(NUM_LEDS):
            if levels[i] == 0 and random.random() < 0.08:
                levels[i] = 255
                colors[i] = (
                    (random.randint(32, 255), random.randint(32, 255), random.randint(32, 255))
                    if random_color
                    else (255, 255, 255)
                )
            if levels[i] > 0:
                scale = levels[i] / 255.0
                c = colors[i]
                pixels[i] = (int(c[0] * scale), int(c[1] * scale), int(c[2] * scale))
                levels[i] = max(0, levels[i] - 12)
            else:
                pixels[i] = (0, 0, 0)
        pixels.show()
        _maybe_log_imu()
        time.sleep(FRAME_DT)


def fireworks(duration):
    start = time.monotonic()
    levels = [0] * NUM_LEDS
    colors = [(0, 0, 0)] * NUM_LEDS
    while time.monotonic() - start < duration:
        if random.random() < 0.12:
            i = random.randrange(NUM_LEDS)
            levels[i] = 255
            colors[i] = (
                random.randint(64, 255),
                random.randint(64, 255),
                random.randint(64, 255),
            )
        for i in range(NUM_LEDS):
            if levels[i] > 0:
                scale = levels[i] / 255.0
                c = colors[i]
                pixels[i] = (int(c[0] * scale), int(c[1] * scale), int(c[2] * scale))
                levels[i] = max(0, levels[i] - 8)
            else:
                pixels[i] = (0, 0, 0)
        pixels.show()
        _maybe_log_imu()
        time.sleep(FRAME_DT)


def accel_tilt(duration):
    """Map pitch/roll-ish accel into color + a bright position marker."""
    start = time.monotonic()
    while time.monotonic() - start < duration:
        _maybe_log_imu()
        if imu is None:
            pixels.fill((40, 0, 0))
            pixels.show()
            time.sleep(0.2)
            continue
        ax, ay, az = imu.acceleration  # m/s^2
        # Rough pitch/roll from gravity vector
        pitch = math.degrees(math.atan2(-ax, math.sqrt(ay * ay + az * az)))
        roll = math.degrees(math.atan2(ay, az))
        pr = _clamp((pitch + 45.0) / 90.0, 0.0, 1.0)
        rr = _clamp((roll + 45.0) / 90.0, 0.0, 1.0)
        red = int(pr * 255)
        green = int(rr * 255)
        blue = int((1.0 - abs(pr - 0.5) * 2.0) * 80)
        pixels.fill((red // 4, green // 4, blue // 4))
        idx = int(rr * (NUM_LEDS - 1))
        pixels[idx] = (red, green, 255)
        pixels.show()
        time.sleep(FRAME_DT)


def _idle(duration):
    """Hold solid color while still emitting periodic IMU logs."""
    end = time.monotonic() + duration
    while time.monotonic() < end:
        _maybe_log_imu()
        time.sleep(0.1)


_last_imu_log = 0.0


def _maybe_log_imu(force=False):
    global _last_imu_log  # pylint: disable=global-statement
    now = time.monotonic()
    if not force and now - _last_imu_log < IMU_LOG_SECONDS:
        return
    _last_imu_log = now
    if imu is None:
        print("IMU: no accelerometer data (check I2C 0x6A)")
        return
    ax, ay, az = imu.acceleration
    gx, gy, gz = imu.gyro
    # gyro is rad/s in CircuitPython — convert to dps to match ESPHome logs
    print(
        "IMU accel m/s2=(%.2f, %.2f, %.2f) gyro dps=(%.1f, %.1f, %.1f)"
        % (ax, ay, az, math.degrees(gx), math.degrees(gy), math.degrees(gz))
    )


EFFECTS = (
    ("Solid Red", lambda d: solid((255, 0, 0), d)),
    ("Solid Green", lambda d: solid((0, 255, 0), d)),
    ("Solid Blue", lambda d: solid((0, 0, 255), d)),
    ("Solid White", lambda d: solid((255, 255, 255), d)),
    ("Solid Cyan", lambda d: solid((0, 255, 255), d)),
    ("Solid Magenta", lambda d: solid((255, 0, 255), d)),
    ("Solid Yellow", lambda d: solid((255, 255, 0), d)),
    ("Rainbow", rainbow),
    ("Color Wipe", color_wipe),
    ("Scan", scan),
    ("Twinkle", lambda d: twinkle(d, random_color=False)),
    ("Random Twinkle", lambda d: twinkle(d, random_color=True)),
    ("Fireworks", fireworks),
    ("Accel Tilt", accel_tilt),
)


print("Hackerbadge test starting — LED pattern loop + IMU")
_maybe_log_imu(force=True)

while True:
    for name, effect in EFFECTS:
        print("LED effect ->", name)
        effect(EFFECT_SECONDS)
