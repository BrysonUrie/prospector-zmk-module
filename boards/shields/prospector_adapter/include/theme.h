/*
 * Prospector screen theme. Every color the status screen uses lives here.
 * Values are 0xRRGGBB. Edit, rebuild (./build.sh dongle) and flash the dongle.
 *
 * Palette: Tokyo Night (night variant), https://github.com/folke/tokyonight.nvim
 */
#pragma once

/* Tokyo Night palette */
#define TN_BG 0x1A1B26
#define TN_BG_HIGHLIGHT 0x292E42
#define TN_FG 0xC0CAF5
#define TN_COMMENT 0x565F89
#define TN_BLUE 0x7AA2F7
#define TN_CYAN 0x7DCFFF
#define TN_MAGENTA 0xBB9AF7
#define TN_ORANGE 0xFF9E64
#define TN_YELLOW 0xE0AF68
#define TN_GREEN 0x9ECE6A
#define TN_GREEN1 0x73DACA
#define TN_TEAL 0x1ABC9C
#define TN_RED 0xF7768E
#define TN_RED1 0xDB4B4B

/* Screen */
#define THEME_BG TN_BG
#define THEME_DIVIDER TN_BG_HIGHLIGHT /* line under the header */

/* Layer roller (big layer name) */
#define THEME_LAYER_ACTIVE TN_FG       /* current layer */
#define THEME_LAYER_INACTIVE TN_COMMENT /* layers above/below */

/* Battery bars (bottom), one per half. A level at or above a threshold uses that band. */
#define THEME_BATTERY_FULL_AT 51
#define THEME_BATTERY_MID_AT 20
#define THEME_BATTERY_TRACK TN_BG_HIGHLIGHT /* empty part of the bar */
#define THEME_BATTERY_SIDE_TEXT TN_COMMENT  /* "L" / "R" */

/* Full: bar gradient left/right end, percentage text */
#define THEME_BATTERY_FULL_START TN_GREEN
#define THEME_BATTERY_FULL_END TN_GREEN1
#define THEME_BATTERY_FULL_TEXT TN_GREEN

/* Full and charging. Unused: ZMK doesn't send the halves' charging state to the dongle. */
#define THEME_BATTERY_CHARGING_START TN_TEAL
#define THEME_BATTERY_CHARGING_END TN_CYAN
#define THEME_BATTERY_CHARGING_TEXT TN_TEAL

/* Mid */
#define THEME_BATTERY_MID_START 0xF2D35B /* brighter than TN_YELLOW, which reads amber */
#define THEME_BATTERY_MID_END 0xFFE680
#define THEME_BATTERY_MID_TEXT 0xF2D35B

/* Low */
#define THEME_BATTERY_LOW_START TN_RED1
#define THEME_BATTERY_LOW_END TN_RED
#define THEME_BATTERY_LOW_TRACK 0x3B2229 /* dimmed red */
#define THEME_BATTERY_LOW_TEXT TN_RED

/* Pet shadow: baked into images by scripts/sprites.py (BG and SHADOW there must match THEME_BG). */

/* Pet level (under the pet): "LV n" and exp bar */
#define THEME_LEVEL_TEXT TN_FG
#define THEME_EXP_BAR TN_CYAN
#define THEME_EXP_TRACK TN_BG_HIGHLIGHT

/* Output (top right): USB, or Bluetooth profile number */
#define THEME_OUTPUT_USB TN_BLUE
#define THEME_OUTPUT_BLE_CONNECTED TN_CYAN
#define THEME_OUTPUT_BLE_DISCONNECTED TN_COMMENT /* paired, not connected */
#define THEME_OUTPUT_BLE_OPEN 0xF2D35B           /* unpaired, ready to pair */

/* Half disconnected */
#define THEME_DISCONNECTED_BAR TN_RED1
#define THEME_DISCONNECTED_TEXT TN_RED
