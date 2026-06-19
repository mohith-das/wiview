/**
 * wiview — Phase 0: Hello Cardputer-Adv
 * Initializes M5Cardputer, draws text on screen, reads a keypress,
 * reads one BMI270 IMU sample, and prints to Serial.
 */
#include <M5Cardputer.h>
#include <M5Unified.h>

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\n=== wiview Phase 0 — Hello Cardputer-Adv ===\n");

    // Initialize the Cardputer (display, keyboard, audio, IMU)
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);

    Serial.println("[OK] M5Cardputer initialized");
    Serial.printf("  Board: %s\n", M5Cardputer.getBoardName());
    Serial.printf("  Display: %dx%d\n", M5Cardputer.Display.width(), M5Cardputer.Display.height());

    // Draw hello text on screen
    M5Cardputer.Display.clear(TFT_BLACK);
    M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setCursor(4, 4);
    M5Cardputer.Display.println("wiview");
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(4, 36);
    M5Cardputer.Display.println("Cardputer-Adv");
    M5Cardputer.Display.setCursor(4, 52);
    M5Cardputer.Display.println("Phase 0 ready.");
    M5Cardputer.Display.setCursor(4, 76);
    M5Cardputer.Display.println("Press any key or G0...");
}

void loop() {
    // Read a keypress
    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isChange()) {
        if (M5Cardputer.Keyboard.isPressed()) {
            auto keys = M5Cardputer.Keyboard.keysState();
            Serial.print("Key pressed: ");
            for (auto k : keys) {
                if (k != 0) Serial.write(k);
            }
            Serial.println();

            M5Cardputer.Display.fillRect(0, 100, 240, 30, TFT_BLACK);
            M5Cardputer.Display.setCursor(4, 104);
            M5Cardputer.Display.setTextColor(TFT_GREEN, TFT_BLACK);
            M5Cardputer.Display.print("Keys: ");
            for (auto k : keys) {
                if (k != 0) M5Cardputer.Display.write(k);
            }
        } else {
            Serial.println("Key released");
        }
    }

    // Read one BMI270 sample
    M5Cardputer.update();
    if (M5Cardputer.Imu.isEnabled()) {
        float ax, ay, az;
        float gx, gy, gz;
        M5Cardputer.Imu.getAccel(&ax, &ay, &az);
        M5Cardputer.Imu.getGyro(&gx, &gy, &gz);

        Serial.printf("IMU  accel: (%+.3f, %+.3f, %+.3f) g | gyro: (%+.3f, %+.3f, %+.3f) dps\n",
                      ax, ay, az, gx, gy, gz);

        char buf[64];
        snprintf(buf, sizeof(buf), "A:%+.2f %+.2f %+.2f", ax, ay, az);
        M5Cardputer.Display.fillRect(0, 110, 240, 12, TFT_BLACK);
        M5Cardputer.Display.setCursor(4, 110);
        M5Cardputer.Display.setTextColor(TFT_CYAN, TFT_BLACK);
        M5Cardputer.Display.print(buf);
    }

    delay(250);
}
