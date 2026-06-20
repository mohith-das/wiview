/**
 * wiview — Phase 3: Multi-screen UI with breathing detection
 *
 * Home view: presence, motion, IMU gate, calibration
 * Waterfall: CSI amplitude heatmap
 * Breathing: waveform + BPM
 * Press G0 to cycle views, or keys 1/2/3 for direct access.
 */
#include "app/app_controller.h"

wiview::AppController app;

void setup() {
    app.begin();
}

void loop() {
    app.update();
}
