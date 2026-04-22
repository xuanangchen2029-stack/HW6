#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BOARD_TUD_RHPORT
#define BOARD_TUD_RHPORT      0
#endif

#ifndef BOARD_TUD_MAX_SPEED
#define BOARD_TUD_MAX_SPEED   OPT_MODE_FULL_SPEED
#endif

// DO NOT redefine CFG_TUSB_OS here!
// The Pico SDK cmake already passes -DCFG_TUSB_OS=OPT_OS_PICO on the command
// line. Adding "#define CFG_TUSB_OS OPT_OS_NONE" here causes the warning
// "CFG_TUSB_OS redefined" and breaks TinyUSB internals (tud_rhport_init,
// tusb_inited, hidd_* all become undefined).

#define CFG_TUSB_DEBUG          0
#define CFG_TUD_ENABLED         1
#define CFG_TUH_ENABLED         0

#define CFG_TUSB_RHPORT0_MODE   (OPT_MODE_DEVICE | BOARD_TUD_MAX_SPEED)

#define CFG_TUD_ENDPOINT0_SIZE  64

// Mouse-only HID project
#define CFG_TUD_HID             1
#define CFG_TUD_CDC             0
#define CFG_TUD_MSC             0
#define CFG_TUD_MIDI            0
#define CFG_TUD_VENDOR          0

#define CFG_TUD_HID_EP_BUFSIZE  16

#ifdef __cplusplus
}
#endif

#endif /* _TUSB_CONFIG_H_ */
