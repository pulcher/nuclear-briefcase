/******************************************************************************
 * Stuff that needs defining in the blu-clear-briefcase
 */

#define BCB_NUM_LEDS                    768 // 256 * 3

#define BCB_NEOPIXEL_PIN                12  // where the signal line is for that neopixels.
#define BCB_LID_PIN                     10  // pin that watches the reed switch on the briefcase lid.
#define BCB_SWITCH_PIN                     6   // pin that sets the switch LED on or off.  Active-Low.

#define BCB_READPACKET_TIMEOUT          10  // Timeout in ms waiting to read a response

// Min and Max brightness ranges
#define BCB_MIN_BRIGHT                  5
#define BCB_MAX_BRIGHT                  120

#define BCB_ANIMATION_CYCLES            10   // number of animation cycles to execute before exiting

// Setup some defaults
#define BCB_DEFAULT_COLOR_1             0xFFFFFF    // White
#define BCB_DEFAULT_COLOR_2             0x0000FF    // Blue
#define BCB_DEFAULT_COLOR_RED           0xFF0000    // Red

// Available animations
#define BCB_ANIMATION_STOP             -1   // Stop the animation
#define BCB_ANIMATION_BREATHING         0   // Breathing LEDS alternate between color_1 and color_2
#define BCB_ANIMATION_COLOR_WIPE        1   // wipe all the LEDS alternate between color_1 and color_2
#define BCB_ANIMATION_PANEL_WIPE        2   // wipe grouping the LEDS by panel alternate between color_1 and color_2
#define BCB_ANIMATION_THEATER           3   // running theater lights using color1 and color2 colors.

// Animation Modes
#define BCB_SINGLE_ANIMATION            0   // only do one animation 
#define BCB_LOOP_ANIMATION              1   // the loop through all animations.
