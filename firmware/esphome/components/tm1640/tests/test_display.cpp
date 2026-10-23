#include "../tm1640_display.h"

#include <assert.h>
#include <stdint.h>

using esphome::tm1640::display::brightness_level;
using esphome::tm1640::display::channel_on;
using esphome::tm1640::display::render_clock;
using esphome::tm1640::display::render_temperature_small;
using esphome::tm1640::display::scale_brightness;

int main() {
  assert(!channel_on(0));
  assert(!channel_on(127));
  assert(channel_on(128));
  assert(channel_on(255));
  assert(brightness_level(0, 7) == 0);
  assert(brightness_level(128, 7) == 4);
  assert(brightness_level(255, 7) == 7);
  assert(brightness_level(255, 4) == 4);
  assert(scale_brightness(200, 50) == 100);
  assert(scale_brightness(255, 25) == 64);
  assert(scale_brightness(255, 100) == 255);

  const uint8_t full_rgb[3] = {1, 1, 1};
  const uint8_t red_only[3] = {1, 0, 0};
  const uint8_t green_only[3] = {0, 1, 0};
  uint8_t ram[16]{};

  // The large digits use static per-channel on/off values; chip brightness is
  // controlled independently through the TM1640 display command.
  render_clock(ram, 12, 34, 56, true, true, true, true, false, full_rgb);
  assert(ram[0] == 0x86 && ram[1] == 0x86 && ram[2] == 0x86);
  assert(ram[3] == 0xDB && ram[4] == 0xDB && ram[5] == 0xDB);
  assert(ram[6] == 0xCF && ram[7] == 0xCF && ram[8] == 0xCF);
  assert(ram[9] == 0x66 && ram[10] == 0x66 && ram[11] == 0x66);
  assert(ram[13] == 0x6D && ram[14] == 0x7D && ram[15] == 0);

  // A single selected color writes only its corresponding RGB grid.
  render_clock(ram, 0, 5, 9, true, true, false, false, false, red_only);
  assert(ram[0] == 0 && ram[1] == 0 && ram[2] == 0);
  assert(ram[3] == 0x3F && ram[4] == 0 && ram[5] == 0);
  assert(ram[6] == 0x3F && ram[7] == 0 && ram[8] == 0);
  assert(ram[9] == 0x6D && ram[10] == 0 && ram[11] == 0);
  assert(ram[13] == 0x3F && ram[14] == 0x6F);

  // Midnight becomes 12 AM, while 3:04 PM lights the PM point and alarm point.
  render_clock(ram, 0, 0, 0, true, true, true, false, false, full_rgb);
  assert(ram[0] == 0x06 && ram[3] == 0x5B);
  render_clock(ram, 15, 4, 9, true, true, true, false, true, red_only);
  assert(ram[0] == 0 && ram[3] == 0x4F);
  assert(ram[6] == 0xBF && ram[9] == 0xE6);

  // An invalid clock source shows dashes rather than stale time or seconds.
  render_clock(ram, 0, 0, 0, false, true, true, false, true, green_only);
  for (uint8_t grid = 0; grid < 12; grid++)
    assert(ram[grid] == ((grid % 3) == 1 ? 0x40 : 0));
  assert(ram[12] == 0 && ram[13] == 0 && ram[14] == 0 && ram[15] == 0);

  render_clock(ram, 12, 34, 56, true, false, true, false, false, full_rgb);
  assert(ram[12] == (1U << 6));

  // The hardwired offline indicator stays on for invalid time and combines
  // with the no-sync symbol.
  render_clock(ram, 0, 0, 0, false, false, true, false, false, full_rgb);
  assert(ram[12] == (1U << 6));
  render_clock(ram, 12, 34, 56, true, false, true, false, false, full_rgb,
               true);
  assert(ram[12] == ((1U << 5) | (1U << 6)));

  // The temperature overlay keeps HH:MM and replaces seconds with whole °C.
  render_clock(ram, 12, 34, 56, true, true, true, true, false, full_rgb);
  render_temperature_small(ram, 23, false);
  assert(ram[0] == 0x86 && ram[3] == 0xDB && ram[6] == 0xCF && ram[9] == 0x66);
  assert(ram[12] == (1U << 3));
  assert(ram[13] == 0x5B && ram[14] == 0x4F && ram[15] == 0);

  // Whole °F uses its documented GRID13 symbol and keeps the hourglass.
  render_clock(ram, 12, 34, 56, true, false, true, true, false, full_rgb, true);
  render_temperature_small(ram, 68, true);
  assert(ram[12] == ((1U << 2) | (1U << 5) | (1U << 6)));
  assert(ram[13] == 0x7D && ram[14] == 0x7F);

  // Values outside the two-digit small display range are made explicit.
  render_temperature_small(ram, 100, false);
  assert(ram[13] == 0x40 && ram[14] == 0x40);
}
