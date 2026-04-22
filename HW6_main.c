/*
 * HW6 — USB HID Mouse with MPU6050
 *
 * MODE_IMU    : Tilt the board to move the cursor.
 *               X-accel → mouse X,  Y-accel → mouse Y.
 *               4 speed levels based on acceleration magnitude.
 *
 * MODE_REMOTE : Mouse moves in a slow circle (anti-screensaver).
 *
 * Button on PIN 14 (active-low, internal pull-up) toggles modes.
 * LED   on PIN 16  is ON  in MODE_REMOTE, OFF in MODE_IMU.
 * On-board LED blinks at a rate that reflects USB mount status.
 *
 * Hardware: Pico2, MPU6050 on I2C0 (SDA=GP0, SCL=GP1)
 */

#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "bsp/board_api.h"   // board_init, board_millis, board_led_write
#include "tusb.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "mpu6050.h"
#include "usb_descriptors.h"

// ─── I2C ────────────────────────────────────────────────────────────────────
#define I2C_PORT      i2c0
#define I2C_SDA_PIN   0
#define I2C_SCL_PIN   1
#define I2C_BAUD      400000

// ─── GPIO ───────────────────────────────────────────────────────────────────
#define MODE_BUTTON_PIN  14   // active-low with internal pull-up
#define MODE_LED_PIN     16   // ON = remote mode

// ─── Mode ───────────────────────────────────────────────────────────────────
typedef enum { MODE_IMU = 0, MODE_REMOTE = 1 } mouse_mode_t;
static mouse_mode_t g_mode = MODE_IMU;

// ─── IMU data ───────────────────────────────────────────────────────────────
static mpu6050_data_t imu_data;

// ─── USB blink intervals (ms) ────────────────────────────────────────────────
enum {
    BLINK_NOT_MOUNTED = 250,
    BLINK_MOUNTED     = 1000,
    BLINK_SUSPENDED   = 2500,
};
static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

// ─── USB mount callbacks ─────────────────────────────────────────────────────
void tud_mount_cb(void)   { blink_interval_ms = BLINK_MOUNTED; }
void tud_umount_cb(void)  { blink_interval_ms = BLINK_NOT_MOUNTED; }
void tud_suspend_cb(bool remote_wakeup_en) {
    (void)remote_wakeup_en;
    blink_interval_ms = BLINK_SUSPENDED;
}
void tud_resume_cb(void) {
    blink_interval_ms = tud_mounted() ? BLINK_MOUNTED : BLINK_NOT_MOUNTED;
}

// ─── TinyUSB HID callbacks (required by tinyusb, unused here) ───────────────
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const *report, uint16_t len) {
    (void)instance; (void)report; (void)len;
}
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                                hid_report_type_t report_type,
                                uint8_t *buffer, uint16_t reqlen) {
    (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)reqlen;
    return 0;
}
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                            hid_report_type_t report_type,
                            uint8_t const *buffer, uint16_t bufsize) {
    (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)bufsize;
}

// ─── On-board LED blink task ─────────────────────────────────────────────────
static void led_blinking_task(void) {
    static uint32_t start_ms = 0;
    static bool     led_state = false;
    if (board_millis() - start_ms < blink_interval_ms) return;
    start_ms += blink_interval_ms;
    board_led_write(led_state);
    led_state = !led_state;
}

// ─── Mode button task (debounced, falling-edge detect) ───────────────────────
static void mode_button_task(void) {
    static bool     btn_last    = true;   // pulled-up → idle = true
    static uint32_t last_press  = 0;

    bool btn_now = gpio_get(MODE_BUTTON_PIN);
    uint32_t now = board_millis();

    // falling edge + 200 ms debounce
    if (btn_last && !btn_now && (now - last_press > 200)) {
        last_press = now;
        g_mode = (g_mode == MODE_IMU) ? MODE_REMOTE : MODE_IMU;
        gpio_put(MODE_LED_PIN, g_mode == MODE_REMOTE ? 1 : 0);
    }
    btn_last = btn_now;
}

// ─── Acceleration → discrete mouse delta (4 speed levels) ───────────────────
// Threshold units: g  (±2 g full-scale from MPU6050)
static int8_t accel_to_delta(float a_g) {
    float mag = fabsf(a_g);
    int8_t sign = (a_g >= 0.0f) ? 1 : -1;

    if (mag < 0.08f) return 0;          // dead zone
    if (mag < 0.25f) return sign * 2;   // slow
    if (mag < 0.50f) return sign * 5;   // medium
    if (mag < 0.75f) return sign * 8;   // fast
    return sign * 12;                   // very fast
}

// ─── Remote mode: pre-computed 16-step circle ───────────────────────────────
// Radius ≈ 2 px steps → forms a smooth circle at 100 ms/step
static const int8_t circle_dx[16] = { 2, 2, 1, 0,-1,-2,-2,-2,-2,-2,-1, 0, 1, 2, 2, 2 };
static const int8_t circle_dy[16] = { 0, 1, 2, 2, 2, 1, 0,-1,-2,-2,-2,-2,-2,-1, 0, 1 };
static uint8_t circle_idx = 0;

// ─── HID mouse task (runs every 10 ms) ───────────────────────────────────────
static void hid_task(void) {
    const uint32_t interval_ms = 10;
    static uint32_t start_ms   = 0;

    if (board_millis() - start_ms < interval_ms) return;
    start_ms += interval_ms;

    if (!tud_hid_ready()) return;

    int8_t dx = 0, dy = 0;

    if (g_mode == MODE_IMU) {
        // Read accelerometer; X-accel → cursor X, Y-accel → cursor Y
        mpu6050_read_all(g_imu_addr, &imu_data);
        dx =  accel_to_delta(imu_data.ax_g);
        dy = -accel_to_delta(imu_data.ay_g);  // invert so tilt-up = cursor up
    } else {
        // Remote mode: advance one step around the circle every 100 ms
        static uint32_t circle_ms = 0;
        if (board_millis() - circle_ms >= 100) {
            circle_ms += 100;
            dx = circle_dx[circle_idx];
            dy = circle_dy[circle_idx];
            circle_idx = (circle_idx + 1) & 0x0F;
        }
        // else dx=dy=0 → no movement this tick
    }

    tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, dx, dy, 0, 0);
}

// ─── main ────────────────────────────────────────────────────────────────────
int main(void) {
    board_init();

    // Mode indicator LED
    gpio_init(MODE_LED_PIN);
    gpio_set_dir(MODE_LED_PIN, GPIO_OUT);
    gpio_put(MODE_LED_PIN, 0);

    // Mode toggle button (internal pull-up, active-low)
    gpio_init(MODE_BUTTON_PIN);
    gpio_set_dir(MODE_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(MODE_BUTTON_PIN);

    // I2C for MPU6050
    i2c_init(I2C_PORT, I2C_BAUD);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    sleep_ms(50);

    // If MPU6050 not found, blink LED fast and hang
    if (!mpu6050_setup()) {
        while (1) {
            gpio_put(MODE_LED_PIN, 1); sleep_ms(100);
            gpio_put(MODE_LED_PIN, 0); sleep_ms(100);
        }
    }

    tusb_init();

    while (1) {
        tud_task();
        led_blinking_task();
        mode_button_task();
        hid_task();
    }

    return 0;
}
