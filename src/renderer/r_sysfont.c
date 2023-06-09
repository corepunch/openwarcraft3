#include "r_local.h"

#define FONT_WIDTH 128
#define FONT_HEIGHT 128

const BYTE font_map[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0xff, 0x0f, 0x00, 0x00, 0x30, 0x18,
    0x00, 0x3c, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0xff, 0x07, 0x7e, 0x18, 0x38, 0x18,
    0x00, 0x42, 0x7e, 0x00, 0x00, 0x18, 0x18, 0x00, 0xff, 0x00, 0xff, 0x0f, 0xe7, 0x1c, 0x3c, 0xdb,
    0x00, 0x81, 0xff, 0x66, 0x18, 0x3c, 0x3c, 0x00, 0xff, 0x3c, 0xc3, 0x1d, 0xe7, 0x1e, 0x3e, 0xdb,
    0x00, 0xe7, 0x99, 0xff, 0x3c, 0x3c, 0x7e, 0x18, 0xe7, 0x66, 0x99, 0x38, 0xe7, 0x1f, 0x3f, 0x7e,
    0x00, 0xa5, 0xdb, 0xff, 0x7e, 0x5a, 0xff, 0x3c, 0xc3, 0xc3, 0x3c, 0x7e, 0x7e, 0x1f, 0x37, 0x3c,
    0x00, 0x99, 0xe7, 0xff, 0xff, 0xff, 0xff, 0x7e, 0x81, 0x81, 0x7e, 0xe7, 0x3c, 0x1d, 0x77, 0xff,
    0x00, 0x81, 0xff, 0xff, 0x7e, 0xff, 0xff, 0x7e, 0x81, 0x81, 0x7e, 0xe7, 0x3c, 0x1c, 0xf7, 0x3c,
    0x00, 0x99, 0xe7, 0x7e, 0x3c, 0x5a, 0x5a, 0x3c, 0xc3, 0xc3, 0x3c, 0xe7, 0xff, 0x7c, 0x67, 0x7e,
    0x00, 0x42, 0x7e, 0x3c, 0x18, 0x18, 0x18, 0x18, 0xe7, 0x66, 0x99, 0xe7, 0x3c, 0xfc, 0x07, 0xdb,
    0x00, 0x3c, 0x3c, 0x18, 0x00, 0x3c, 0x3c, 0x00, 0xff, 0x3c, 0xc3, 0x7e, 0x3c, 0xf8, 0x0f, 0xdb,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0xff, 0x00, 0x00, 0x70, 0x1e, 0x18,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0xff, 0x00, 0x00, 0x00, 0x0c, 0x18,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x77, 0x00, 0x00, 0x00,
    0xe0, 0x07, 0x18, 0xee, 0x7f, 0x3e, 0x00, 0x18, 0x18, 0x3c, 0x00, 0x00, 0xee, 0x00, 0x00, 0x00,
    0xf0, 0x0f, 0x3c, 0xee, 0xd7, 0x78, 0x00, 0x3c, 0x3c, 0x3c, 0x00, 0x00, 0xee, 0x00, 0x00, 0x00,
    0xf8, 0x1f, 0x7e, 0xee, 0xd7, 0xdc, 0x00, 0x7e, 0x7e, 0x3c, 0x08, 0x10, 0x00, 0x00, 0x18, 0xff,
    0xfc, 0x3f, 0xff, 0xee, 0xd7, 0xce, 0x00, 0xff, 0xff, 0x3c, 0x0c, 0x30, 0x00, 0x24, 0x3c, 0xff,
    0xfe, 0x7f, 0x3c, 0xee, 0x7f, 0xe7, 0x00, 0x3c, 0x3c, 0x3c, 0xfe, 0x7f, 0x00, 0x66, 0x3c, 0x7e,
    0xff, 0xff, 0x3c, 0xee, 0x17, 0x73, 0x00, 0x3c, 0x3c, 0x3c, 0xff, 0xff, 0x00, 0xff, 0x7e, 0x7e,
    0xfe, 0x7f, 0x3c, 0xee, 0x17, 0x3b, 0xff, 0x3c, 0x3c, 0x3c, 0xff, 0xff, 0x00, 0xff, 0x7e, 0x3c,
    0xfc, 0x3f, 0x3c, 0x00, 0x17, 0x1e, 0xff, 0x3c, 0x3c, 0x3c, 0xfe, 0x7f, 0x00, 0xff, 0xff, 0x3c,
    0xf8, 0x1f, 0xff, 0xee, 0x17, 0x7c, 0xff, 0xff, 0x3c, 0xff, 0x0c, 0x30, 0x00, 0x66, 0xff, 0x18,
    0xf0, 0x0f, 0x7e, 0xee, 0x17, 0x78, 0xff, 0x7e, 0x3c, 0x7e, 0x08, 0x10, 0x00, 0x24, 0x00, 0x00,
    0xe0, 0x07, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x3c, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x18, 0x3c, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x77, 0x00, 0x18, 0x40, 0x00, 0x1c, 0x1c, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x38, 0x77, 0x00, 0x7e, 0xe2, 0x7c, 0x38, 0x38, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
    0x00, 0x7c, 0xee, 0x66, 0xff, 0xe6, 0xfe, 0x70, 0x70, 0x0e, 0x18, 0x00, 0x00, 0x00, 0x00, 0x06,
    0x00, 0x7c, 0x00, 0x66, 0xe7, 0x4e, 0xee, 0x00, 0x70, 0x0e, 0x18, 0x38, 0x00, 0x00, 0x00, 0x0e,
    0x00, 0x7c, 0x00, 0xff, 0xe0, 0x1e, 0x7c, 0x00, 0x70, 0x0e, 0xff, 0x38, 0x00, 0x00, 0x00, 0x1e,
    0x00, 0x38, 0x00, 0xff, 0xfe, 0x3c, 0x7c, 0x00, 0x70, 0x0e, 0xff, 0xfe, 0x00, 0xfe, 0x00, 0x3c,
    0x00, 0x38, 0x00, 0x66, 0x7f, 0x78, 0xff, 0x00, 0x70, 0x0e, 0x3c, 0xfe, 0x00, 0xfe, 0x00, 0x78,
    0x00, 0x38, 0x00, 0xff, 0x07, 0xf0, 0xef, 0x00, 0x70, 0x0e, 0x7e, 0x38, 0x00, 0x00, 0x00, 0xf0,
    0x00, 0x00, 0x00, 0xff, 0xe7, 0xe4, 0xee, 0x00, 0x70, 0x0e, 0x66, 0x38, 0x00, 0x00, 0x38, 0xe0,
    0x00, 0x38, 0x00, 0x66, 0xff, 0xce, 0xff, 0x00, 0x70, 0x0e, 0x00, 0x00, 0x1c, 0x00, 0x38, 0xc0,
    0x00, 0x38, 0x00, 0x66, 0x7e, 0x8e, 0x77, 0x00, 0x38, 0x1c, 0x00, 0x00, 0x38, 0x00, 0x38, 0x80,
    0x00, 0x00, 0x00, 0x00, 0x18, 0x04, 0x00, 0x00, 0x1c, 0x38, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x3c, 0x1c, 0x7e, 0x7e, 0x0e, 0xff, 0x7e, 0xff, 0x7e, 0x7e, 0x00, 0x00, 0x0e, 0x00, 0x70, 0x7e,
    0x7e, 0x3c, 0xff, 0xff, 0x1e, 0xff, 0xff, 0xff, 0xff, 0xff, 0x38, 0x38, 0x1c, 0x00, 0x38, 0xff,
    0xe7, 0x7c, 0xe7, 0xe7, 0x3e, 0xe0, 0xe7, 0x07, 0xe7, 0xe7, 0x38, 0x38, 0x38, 0x00, 0x1c, 0xe7,
    0xe7, 0x1c, 0x07, 0x07, 0x7e, 0xfe, 0xe0, 0x07, 0xe7, 0xe7, 0x38, 0x38, 0x70, 0xfe, 0x0e, 0x07,
    0xe7, 0x1c, 0x0e, 0x3e, 0xee, 0xff, 0xfe, 0x0e, 0x7e, 0xff, 0x00, 0x00, 0xe0, 0xfe, 0x07, 0x0e,
    0xe7, 0x1c, 0x1c, 0x3e, 0xff, 0x07, 0xff, 0x1c, 0x7e, 0x7f, 0x00, 0x00, 0xe0, 0x00, 0x07, 0x1c,
    0xe7, 0x1c, 0x38, 0x07, 0xff, 0x07, 0xe7, 0x1c, 0xe7, 0x07, 0x38, 0x38, 0x70, 0xfe, 0x0e, 0x1c,
    0xe7, 0x1c, 0x70, 0xe7, 0x0e, 0xe7, 0xe7, 0x38, 0xe7, 0xe7, 0x38, 0x38, 0x38, 0xfe, 0x1c, 0x00,
    0x7e, 0x1c, 0xff, 0xff, 0x0e, 0xff, 0xff, 0x38, 0xff, 0xff, 0x38, 0x38, 0x1c, 0x00, 0x38, 0x1c,
    0x3c, 0x1c, 0xff, 0x7e, 0x0e, 0x7e, 0x7e, 0x38, 0x7e, 0x7e, 0x00, 0x70, 0x0e, 0x00, 0x70, 0x1c,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x7e, 0x18, 0xfe, 0x7e, 0xfc, 0xff, 0xff, 0x3e, 0xe7, 0x7c, 0x0e, 0xe7, 0xe0, 0xc3, 0xe7, 0x7e,
    0xff, 0x3c, 0xff, 0xff, 0xfe, 0xff, 0xff, 0x7f, 0xe7, 0x7c, 0x0e, 0xe7, 0xe0, 0xe7, 0xe7, 0xff,
    0xcf, 0x3c, 0xe7, 0xe7, 0xe7, 0xe0, 0xe0, 0xe7, 0xe7, 0x38, 0x0e, 0xee, 0xe0, 0xff, 0xf7, 0xe7,
    0xdb, 0x7e, 0xe7, 0xe0, 0xe7, 0xe0, 0xe0, 0xe0, 0xe7, 0x38, 0x0e, 0xee, 0xe0, 0xff, 0xff, 0xe7,
    0xdb, 0x66, 0xfe, 0xe0, 0xe7, 0xfc, 0xfc, 0xe0, 0xff, 0x38, 0x0e, 0xfc, 0xe0, 0xff, 0xff, 0xe7,
    0xdb, 0xe7, 0xff, 0xe0, 0xe7, 0xfc, 0xfc, 0xef, 0xff, 0x38, 0x0e, 0xfc, 0xe0, 0xe7, 0xff, 0xe7,
    0xde, 0xff, 0xe7, 0xe0, 0xe7, 0xe0, 0xe0, 0xe7, 0xe7, 0x38, 0xee, 0xee, 0xe0, 0xe7, 0xef, 0xe7,
    0xc0, 0xff, 0xe7, 0xe7, 0xe7, 0xe0, 0xe0, 0xe3, 0xe7, 0x38, 0xee, 0xee, 0xe0, 0xe7, 0xe7, 0xe7,
    0xfe, 0xe7, 0xff, 0xff, 0xfe, 0xff, 0xe0, 0x7f, 0xe7, 0x7c, 0xfe, 0xe7, 0xff, 0xe7, 0xe7, 0xff,
    0x7e, 0xe7, 0xfe, 0x7e, 0xfc, 0xff, 0xe0, 0x3e, 0xe7, 0x7c, 0x7c, 0xe7, 0xff, 0xe7, 0xe7, 0x7e,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x7c, 0x3c, 0x00,
    0xfe, 0x7e, 0xfe, 0x7e, 0xfe, 0xe7, 0xe7, 0xe7, 0xe7, 0xee, 0xff, 0x3e, 0x80, 0x7c, 0x7e, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xfe, 0xe7, 0xe7, 0xe7, 0xe7, 0xee, 0xff, 0x38, 0xc0, 0x1c, 0xe7, 0x00,
    0xe7, 0xe7, 0xe7, 0xe7, 0x38, 0xe7, 0xe7, 0xe7, 0x7e, 0xee, 0x07, 0x38, 0xe0, 0x1c, 0x00, 0x00,
    0xe7, 0xe7, 0xe7, 0xe0, 0x38, 0xe7, 0xe7, 0xe7, 0x7e, 0xee, 0x0e, 0x38, 0xf0, 0x1c, 0x00, 0x00,
    0xff, 0xe7, 0xfe, 0xfe, 0x38, 0xe7, 0xe7, 0xe7, 0x3c, 0x7c, 0x1c, 0x38, 0x78, 0x1c, 0x00, 0x00,
    0xfe, 0xe7, 0xfe, 0x7f, 0x38, 0xe7, 0x66, 0xff, 0x3c, 0x7c, 0x38, 0x38, 0x3c, 0x1c, 0x00, 0x00,
    0xe0, 0xe7, 0xe7, 0x07, 0x38, 0xe7, 0x7e, 0xff, 0x7e, 0x38, 0x70, 0x38, 0x1e, 0x1c, 0x00, 0x00,
    0xe0, 0xf7, 0xe7, 0xe7, 0x38, 0xe7, 0x3c, 0xff, 0x7e, 0x38, 0xe0, 0x38, 0x0e, 0x1c, 0x00, 0x00,
    0xe0, 0xff, 0xe7, 0xff, 0x38, 0xff, 0x3c, 0x66, 0xe7, 0x38, 0xff, 0x38, 0x06, 0x1c, 0x00, 0x00,
    0xe0, 0x7e, 0xe7, 0x7e, 0x38, 0x7e, 0x18, 0x66, 0xe7, 0x38, 0xff, 0x3e, 0x02, 0x7c, 0x00, 0x00,
    0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x7c, 0x00, 0xff,
    0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x38, 0x00, 0xe0, 0x00, 0x07, 0x00, 0x3e, 0x00, 0xe0, 0x38, 0x0e, 0xe0, 0x38, 0x00, 0x00, 0x00,
    0x1c, 0x00, 0xe0, 0x00, 0x07, 0x00, 0x7f, 0x00, 0xe0, 0x38, 0x0e, 0xe0, 0x38, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xe0, 0x00, 0x07, 0x00, 0x73, 0x00, 0xe0, 0x00, 0x00, 0xe0, 0x38, 0x00, 0x00, 0x00,
    0x00, 0x3e, 0xfe, 0x7e, 0x7f, 0x7e, 0x70, 0x7f, 0xfe, 0x38, 0x0e, 0xe7, 0x38, 0xc3, 0xee, 0x7e,
    0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xfc, 0xff, 0xff, 0x38, 0x0e, 0xef, 0x38, 0xe7, 0xff, 0xff,
    0x00, 0x07, 0xe7, 0xe7, 0xe7, 0xe7, 0xfc, 0xe7, 0xe7, 0x38, 0x0e, 0xfc, 0x38, 0xff, 0xf7, 0xe7,
    0x00, 0x7f, 0xe7, 0xe0, 0xe7, 0xff, 0x70, 0xe7, 0xe7, 0x38, 0x0e, 0xfc, 0x38, 0xff, 0xe7, 0xe7,
    0x00, 0xe7, 0xe7, 0xe7, 0xe7, 0xe0, 0x70, 0xe7, 0xe7, 0x38, 0x0e, 0xee, 0x38, 0xe7, 0xe7, 0xe7,
    0x00, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x70, 0xff, 0xe7, 0x38, 0x0e, 0xe7, 0x38, 0xe7, 0xe7, 0xff,
    0x00, 0x7f, 0xfe, 0x7e, 0x7f, 0x7e, 0x70, 0x7f, 0xe7, 0x38, 0x0e, 0xe7, 0x38, 0xe7, 0xe7, 0x7e,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0xee, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x38, 0xf0, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x38, 0xf8, 0x76, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x38, 0x38, 0xfe, 0x18,
    0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x38, 0x38, 0xdc, 0x3c,
    0xfe, 0x7f, 0xee, 0x7e, 0xfe, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xff, 0x1c, 0x38, 0x38, 0x00, 0x7e,
    0xff, 0xff, 0xff, 0xff, 0xfe, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xff, 0x78, 0x38, 0x1e, 0x00, 0xe7,
    0xe7, 0xe7, 0xf7, 0xf0, 0x38, 0xe7, 0xe7, 0xe7, 0x7e, 0xe7, 0x1e, 0x78, 0x38, 0x1e, 0x00, 0xe7,
    0xe7, 0xe7, 0xe0, 0x7e, 0x38, 0xe7, 0x7e, 0xff, 0x3c, 0xe7, 0x3c, 0x1c, 0x38, 0x38, 0x00, 0xff,
    0xe7, 0xe7, 0xe0, 0x0f, 0x38, 0xe7, 0x7e, 0xff, 0x7e, 0xe7, 0x78, 0x1c, 0x38, 0x38, 0x00, 0xff,
    0xff, 0xff, 0xe0, 0xff, 0x3e, 0xff, 0x3c, 0xe7, 0xe7, 0xff, 0xff, 0x1c, 0x38, 0x38, 0x00, 0x00,
    0xfe, 0x7f, 0xe0, 0x7e, 0x1e, 0x7e, 0x18, 0xc3, 0xe7, 0x7f, 0xff, 0x1f, 0x38, 0xf8, 0x00, 0x00,
    0xe0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x0f, 0x38, 0xf0, 0x00, 0x00,
    0xe0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xe0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

LPTEXTURE R_MakeSysFontTexture(void) {
    LPTEXTURE texture = ri.MemAlloc(sizeof(TEXTURE));
    texture->width = FONT_WIDTH;
    texture->height = FONT_HEIGHT;
    
    COLOR32 color[FONT_WIDTH * FONT_HEIGHT];
    FOR_LOOP(i, FONT_WIDTH * FONT_HEIGHT) {
        BYTE value = font_map[i / 8];
        COLOR32 col = COLOR32_WHITE;
        col.a *= ((value >> (7 - (i % 8))) & 1);
        color[i] = col;
    }
    R_Call(glGenTextures, 1, &texture->texid);
    R_Call(glBindTexture, GL_TEXTURE_2D, texture->texid);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    R_Call(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA, FONT_WIDTH, FONT_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, color);
    return texture;
}
