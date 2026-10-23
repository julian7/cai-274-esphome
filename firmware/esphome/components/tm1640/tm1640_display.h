#pragma once

#include <stdint.h>

namespace esphome {
namespace tm1640 {
namespace display {

constexpr bool channel_on(uint8_t channel) { return channel >= 128; }

constexpr uint8_t brightness_level(uint8_t brightness, uint8_t maximum) {
  return static_cast<uint8_t>((brightness * maximum + 127U) / 255U);
}

constexpr uint8_t scale_brightness(uint8_t brightness, uint8_t percent) {
  return static_cast<uint8_t>(
      (static_cast<uint16_t>(brightness) * percent + 50U) / 100U);
}

constexpr uint8_t glyph(char c) {
  switch (c) {
  case '0':
    return 0b0111111;
  case '1':
    return 0b0000110;
  case '2':
    return 0b1011011;
  case '3':
    return 0b1001111;
  case '4':
    return 0b1100110;
  case '5':
    return 0b1101101;
  case '6':
    return 0b1111101;
  case '7':
    return 0b0000111;
  case '8':
    return 0b1111111;
  case '9':
    return 0b1101111;
  case '-':
    return 0b1000000;
  default:
    return 0;
  }
}

inline void set_large_digit(uint8_t *ram, uint8_t digit, char c,
                            const uint8_t *rgb_on) {
  if (digit >= 4)
    return;

  const uint8_t segments = glyph(c);
  for (uint8_t channel = 0; channel < 3; channel++) {
    if (rgb_on[channel])
      ram[digit * 3 + channel] |= segments;
  }
}

inline void set_large_dot(uint8_t *ram, uint8_t digit, const uint8_t *rgb_on) {
  if (digit >= 4)
    return;

  for (uint8_t channel = 0; channel < 3; channel++) {
    if (rgb_on[channel])
      ram[digit * 3 + channel] |= 0x80;
  }
}

inline void set_small_digit(uint8_t *ram, uint8_t grid, char c) {
  if (grid < 13 || grid > 14)
    return;
  ram[grid] = glyph(c);
}

// Render HH:MM on the four large RGB digits, with SS on the two full small
// digits. RAM indices follow the board documentation: each large digit has
// three consecutive R/G/B grids; the small digits occupy GRID14 and GRID15.
inline void render_clock(uint8_t *ram, int hour, int minute, int second,
                         bool valid, bool online, bool twelve_hour,
                         bool colon_on, bool alarm_on, const uint8_t *rgb_on,
                         bool no_sync = false) {
  for (uint8_t i = 0; i < 16; i++)
    ram[i] = 0;

  if (!online)
    ram[12] |= 1U << 6;
  if (no_sync)
    ram[12] |= 1U << 5;

  if (!valid) {
    for (uint8_t digit = 0; digit < 4; digit++)
      set_large_digit(ram, digit, '-', rgb_on);
    return;
  }

  const bool pm = hour >= 12;
  if (twelve_hour) {
    hour %= 12;
    if (hour == 0)
      hour = 12;
  }

  if (hour >= 10)
    set_large_digit(ram, 0, static_cast<char>('0' + hour / 10), rgb_on);
  set_large_digit(ram, 1, static_cast<char>('0' + hour % 10), rgb_on);
  set_large_digit(ram, 2, static_cast<char>('0' + minute / 10), rgb_on);
  set_large_digit(ram, 3, static_cast<char>('0' + minute % 10), rgb_on);

  if (colon_on) {
    set_large_dot(ram, 0, rgb_on);
    set_large_dot(ram, 1, rgb_on);
  }
  if (twelve_hour && pm)
    set_large_dot(ram, 2, rgb_on);
  if (alarm_on)
    set_large_dot(ram, 3, rgb_on);

  // Seconds use GRID14 and GRID15; GRID13 carries a partial digit and symbols.
  set_small_digit(ram, 13, static_cast<char>('0' + second / 10));
  set_small_digit(ram, 14, static_cast<char>('0' + second % 10));
}

inline void render_temperature_small(uint8_t *ram, int whole_degrees,
                                     bool fahrenheit) {
  if (whole_degrees < 0 || whole_degrees > 99) {
    ram[13] = glyph('-');
    ram[14] = glyph('-');
  } else {
    ram[13] = whole_degrees < 10 ? 0 : glyph('0' + whole_degrees / 10);
    ram[14] = glyph('0' + whole_degrees % 10);
  }
  ram[12] |= 1U << (fahrenheit ? 2 : 3);
}

} // namespace display
} // namespace tm1640
} // namespace esphome
