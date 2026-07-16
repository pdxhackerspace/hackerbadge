/*
 * PDX Hackerspace Hackerbadge — Arduino / FastLED hardware test
 *
 * Exercises the 37 WS2812B LEDs (colors + patterns) and the LSM6DSO IMU.
 *
 * Pinout (Seeed Xiao ESP32-S3):
 *   LED data  D10 = GPIO9
 *   I2C SDA   D1  = GPIO2
 *   I2C SCL   D0  = GPIO1
 *   LSM6DSO   0x6A
 *
 * Board:  Seeed XIAO ESP32S3
 * Libraries (Library Manager):
 *   - FastLED
 *   - Adafruit LSM6DS
 *   - Adafruit Unified Sensor  (dependency)
 *   - Adafruit BusIO           (dependency)
 *
 * After boot the badge cycles solid colors and patterns. Serial logs print
 * accel/gyro samples. The Accel Tilt effect maps orientation to LEDs.
 */

#include <Wire.h>
#include <FastLED.h>
#include <Adafruit_LSM6DSOX.h>
#include <Adafruit_Sensor.h>
#include <math.h>

#if defined(ARDUINO_XIAO_ESP32S3) || defined(ARDUINO_SEEED_XIAO_ESP32S3)
#define LED_DATA_PIN D10
#define I2C_SDA_PIN D1
#define I2C_SCL_PIN D0
#else
// Xiao ESP32-S3 GPIO map when using a generic ESP32-S3 board package
#define LED_DATA_PIN 9
#define I2C_SDA_PIN 2
#define I2C_SCL_PIN 1
#endif

#define NUM_LEDS 37
#define BRIGHTNESS 102  // ~40% of 255
#define EFFECT_MS 8000
#define IMU_LOG_MS 2000
#define FRAME_MS 40

CRGB leds[NUM_LEDS];
Adafruit_LSM6DSOX imu;
bool imuOk = false;
uint32_t lastImuLogMs = 0;

static float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static void maybeLogImu(bool force = false) {
  uint32_t now = millis();
  if (!force && (now - lastImuLogMs) < IMU_LOG_MS) {
    return;
  }
  lastImuLogMs = now;

  if (!imuOk) {
    Serial.println(F("IMU: no accelerometer data (check I2C 0x6A)"));
    return;
  }

  sensors_event_t accel, gyro, temp;
  imu.getEvent(&accel, &gyro, &temp);

  Serial.printf(
      "IMU accel m/s2=(%.2f, %.2f, %.2f) gyro dps=(%.1f, %.1f, %.1f)\n",
      accel.acceleration.x, accel.acceleration.y, accel.acceleration.z,
      gyro.gyro.x * RAD_TO_DEG, gyro.gyro.y * RAD_TO_DEG,
      gyro.gyro.z * RAD_TO_DEG);
}

static void solid(const CRGB &color, uint32_t durationMs) {
  fill_solid(leds, NUM_LEDS, color);
  FastLED.show();
  uint32_t end = millis() + durationMs;
  while (millis() < end) {
    maybeLogImu();
    delay(50);
  }
}

static void rainbow(uint32_t durationMs) {
  uint32_t start = millis();
  uint8_t offset = 0;
  while (millis() - start < durationMs) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CHSV(offset + (i * 255 / NUM_LEDS), 255, 255);
    }
    FastLED.show();
    offset += 4;
    maybeLogImu();
    delay(FRAME_MS);
  }
}

static void colorWipe(uint32_t durationMs) {
  const CRGB colors[] = {
      CRGB::Red, CRGB::Green, CRGB::Blue, CRGB::White,
  };
  uint32_t start = millis();
  while (millis() - start < durationMs) {
    for (CRGB color : colors) {
      for (int i = 0; i < NUM_LEDS; i++) {
        if (millis() - start >= durationMs) {
          return;
        }
        leds[i] = color;
        FastLED.show();
        maybeLogImu();
        delay(40);
      }
    }
  }
}

static void scan(uint32_t durationMs) {
  uint32_t start = millis();
  int pos = 0;
  int direction = 1;
  const int width = 3;
  while (millis() - start < durationMs) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    for (int w = 0; w < width; w++) {
      int idx = pos + w;
      if (idx >= 0 && idx < NUM_LEDS) {
        leds[idx] = CRGB(0, 180, 255);
      }
    }
    FastLED.show();
    pos += direction;
    if (pos <= 0) {
      pos = 0;
      direction = 1;
    } else if (pos >= NUM_LEDS - width) {
      pos = NUM_LEDS - width;
      direction = -1;
    }
    maybeLogImu();
    delay(FRAME_MS);
  }
}

static void twinkle(uint32_t durationMs, bool randomColor) {
  uint32_t start = millis();
  uint8_t levels[NUM_LEDS] = {0};
  CRGB colors[NUM_LEDS];
  while (millis() - start < durationMs) {
    for (int i = 0; i < NUM_LEDS; i++) {
      if (levels[i] == 0 && random8() < 20) {  // ~8% of 255
        levels[i] = 255;
        colors[i] = randomColor ? CRGB(CHSV(random8(), 255, 255)) : CRGB::White;
      }
      if (levels[i] > 0) {
        leds[i] = colors[i];
        leds[i].nscale8(levels[i]);
        levels[i] = (levels[i] > 12) ? (levels[i] - 12) : 0;
      } else {
        leds[i] = CRGB::Black;
      }
    }
    FastLED.show();
    maybeLogImu();
    delay(FRAME_MS);
  }
}

static void fireworks(uint32_t durationMs) {
  uint32_t start = millis();
  uint8_t levels[NUM_LEDS] = {0};
  CRGB colors[NUM_LEDS];
  while (millis() - start < durationMs) {
    if (random8() < 30) {  // ~12%
      int i = random8(NUM_LEDS);
      levels[i] = 255;
      colors[i] = CHSV(random8(), 255, 255);
    }
    for (int i = 0; i < NUM_LEDS; i++) {
      if (levels[i] > 0) {
        leds[i] = colors[i];
        leds[i].nscale8(levels[i]);
        levels[i] = (levels[i] > 8) ? (levels[i] - 8) : 0;
      } else {
        leds[i] = CRGB::Black;
      }
    }
    FastLED.show();
    maybeLogImu();
    delay(FRAME_MS);
  }
}

static void accelTilt(uint32_t durationMs) {
  uint32_t start = millis();
  while (millis() - start < durationMs) {
    maybeLogImu();
    if (!imuOk) {
      fill_solid(leds, NUM_LEDS, CRGB(40, 0, 0));
      FastLED.show();
      delay(200);
      continue;
    }

    sensors_event_t accel, gyro, temp;
    imu.getEvent(&accel, &gyro, &temp);
    float ax = accel.acceleration.x;
    float ay = accel.acceleration.y;
    float az = accel.acceleration.z;

    float pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * RAD_TO_DEG;
    float roll = atan2f(ay, az) * RAD_TO_DEG;
    float pr = clampf((pitch + 45.0f) / 90.0f, 0.0f, 1.0f);
    float rr = clampf((roll + 45.0f) / 90.0f, 0.0f, 1.0f);

    uint8_t red = (uint8_t)(pr * 255.0f);
    uint8_t green = (uint8_t)(rr * 255.0f);
    uint8_t blue = (uint8_t)((1.0f - fabsf(pr - 0.5f) * 2.0f) * 80.0f);

    fill_solid(leds, NUM_LEDS, CRGB(red / 4, green / 4, blue / 4));
    int idx = (int)(rr * (NUM_LEDS - 1));
    leds[idx] = CRGB(red, green, 255);
    FastLED.show();
    delay(FRAME_MS);
  }
}

struct Effect {
  const char *name;
  void (*run)(uint32_t durationMs);
};

// Wrappers so twinkle variants fit the Effect table
static void twinkleWhite(uint32_t ms) { twinkle(ms, false); }
static void twinkleRandom(uint32_t ms) { twinkle(ms, true); }
static void solidRed(uint32_t ms) { solid(CRGB::Red, ms); }
static void solidGreen(uint32_t ms) { solid(CRGB::Green, ms); }
static void solidBlue(uint32_t ms) { solid(CRGB::Blue, ms); }
static void solidWhite(uint32_t ms) { solid(CRGB::White, ms); }
static void solidCyan(uint32_t ms) { solid(CRGB::Cyan, ms); }
static void solidMagenta(uint32_t ms) { solid(CRGB::Magenta, ms); }
static void solidYellow(uint32_t ms) { solid(CRGB::Yellow, ms); }

static const Effect EFFECTS[] = {
    {"Solid Red", solidRed},
    {"Solid Green", solidGreen},
    {"Solid Blue", solidBlue},
    {"Solid White", solidWhite},
    {"Solid Cyan", solidCyan},
    {"Solid Magenta", solidMagenta},
    {"Solid Yellow", solidYellow},
    {"Rainbow", rainbow},
    {"Color Wipe", colorWipe},
    {"Scan", scan},
    {"Twinkle", twinkleWhite},
    {"Random Twinkle", twinkleRandom},
    {"Fireworks", fireworks},
    {"Accel Tilt", accelTilt},
};

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println(F("Hackerbadge test starting — LED pattern loop + IMU"));

  FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear(true);

  // Badge I2C is on D0/D1, not the Xiao default SDA/SCL pads.
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  imuOk = imu.begin_I2C(0x6A, &Wire);
  if (imuOk) {
    imu.setAccelRange(LSM6DS_ACCEL_RANGE_4_G);
    imu.setGyroRange(LSM6DS_GYRO_RANGE_2000_DPS);
    imu.setAccelDataRate(LSM6DS_RATE_104_HZ);
    imu.setGyroDataRate(LSM6DS_RATE_104_HZ);
    Serial.println(F("IMU: LSM6DSO ready at 0x6A"));
  } else {
    Serial.println(F("IMU: init failed — check wiring / I2C pullups; LED tests will still run"));
  }

  maybeLogImu(true);
}

void loop() {
  for (const Effect &effect : EFFECTS) {
    Serial.print(F("LED effect -> "));
    Serial.println(effect.name);
    effect.run(EFFECT_MS);
  }
}
