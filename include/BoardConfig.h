#pragma once
// Hardwareprofil. Alt her er ANTAGELSER indtil verificeret på den fysiske enhed.
#define TOUCH_CLK  25
#define TOUCH_MOSI 32
#define TOUCH_MISO 39
#define TOUCH_CS   33
#define TOUCH_IRQ  36
// Rå XPT2046-grænser (groft estimat; kalibreres i test)
#define TOUCH_RAW_X_MIN 200
#define TOUCH_RAW_X_MAX 3800
#define TOUCH_RAW_Y_MIN 240
#define TOUCH_RAW_Y_MAX 3800
#define DEFAULT_ROTATION 1   // landscape 320x240
