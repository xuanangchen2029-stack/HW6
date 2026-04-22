#ifndef USB_DESCRIPTORS_H_
#define USB_DESCRIPTORS_H_

// Only one report ID: mouse = 1
// (The assignment says: change REPORT_ID_MOUSE to 1 so the Pico enumerates
//  as a mouse only. KEYBOARD and MOUSE must NOT share the same value in an
//  enum — that is a compile-time duplicate, so we keep only MOUSE here.)
enum
{
  REPORT_ID_MOUSE = 1,
  REPORT_ID_COUNT
};

#endif /* USB_DESCRIPTORS_H_ */
