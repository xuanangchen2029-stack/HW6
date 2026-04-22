# HW6 - USB Mouse with MPU6050

Start from the TinyUSB `dev_hid_composite` example.

In `usb_descriptors.h`:
- set `REPORT_ID_MOUSE` to `1`

Use these files:
- HW6_main.c
- mpu6050.c
- mpu6050.h
- usb_descriptors.c
- usb_descriptors.h
- tusb_config.h
- CMakeLists.txt

Wiring:
- Pico GP0 / I2C0 SDA -> MPU6050 SDA
- Pico GP1 / I2C0 SCL -> MPU6050 SCL
- Pico 3V3(OUT) -> MPU6050 VCC
- Pico GND -> MPU6050 GND
- Button from GP14 to GND, using internal pull-up in code
- External mode LED: GP16 -> 330 ohm resistor -> LED -> GND
- USB cable to computer for mouse HID

Modes:
- Regular mode: IMU controls mouse movement
- Remote-working mode: mouse moves in a slow circle
- Press the button to toggle modes
- External LED ON = remote-working mode
