#pragma once

#include "esphome/components/light/light_output.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "tm1640_display.h"

namespace esphome {
namespace tm1640 {

// Low-level TM1640 output driver. Each of the 16 grid addresses stores
// eight segment outputs; the board's seven-segment/RGB wiring determines which
// grid/segment pair controls each LED channel.
class Tm1640 : public Component {
public:
  void set_pins(InternalGPIOPin *clk, InternalGPIOPin *dio) {
    clk_ = clk;
    dio_ = dio;
  }
  void set_driver_brightness(uint8_t level) {
    driver_brightness_max_ = level > 7 ? 7 : level;
    this->update_brightness_();
  }

  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  // Grid addresses are 0..15 and segment outputs are 0..7. Invalid indices
  // are ignored so a bad mapping cannot write outside the display RAM buffer.
  void set_grid_data(uint8_t address, uint8_t segments);
  void set_output(uint8_t address, uint8_t segment, bool on);
  void clear();
  void set_display_on(bool on);

  // Clock rendering is enabled when a time source is configured. The TM1640
  // scans its display RAM internally.
  void set_time(time::RealTimeClock *time) {
    time_ = time;
    clock_dirty_ = true;
  }
  void show_temperature(float celsius, bool fahrenheit);
  void set_twelve_hour(bool enabled) {
    twelve_hour_ = enabled;
    clock_dirty_ = true;
  }
  void set_blink_colon(bool enabled) {
    blink_colon_ = enabled;
    clock_dirty_ = true;
  }
  void set_online(bool online) {
    if (online_ != online) {
      online_ = online;
      clock_dirty_ = true;
    }
  }

  void set_no_sync(bool no_sync) {
    if (no_sync_ != no_sync) {
      no_sync_ = no_sync;
      clock_dirty_ = true;
    }
  }
  void set_dim_dark_percent(uint8_t percent) {
    dim_dark_percent_ = percent > 100 ? 100 : percent;
  }
  void set_dim_darker_percent(uint8_t percent) {
    dim_darker_percent_ = percent > 100 ? 100 : percent;
  }
  void dim_normal() { this->set_dim_percent_(100); }
  void dim_dark() { this->set_dim_percent_(dim_dark_percent_); }
  void dim_darker() { this->set_dim_percent_(dim_darker_percent_); }
  void set_rgb(uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness) {
    const uint8_t next[3] = {display::channel_on(red),
                             display::channel_on(green),
                             display::channel_on(blue)};
    for (uint8_t channel = 0; channel < 3; channel++) {
      if (rgb_on_[channel] != next[channel]) {
        rgb_on_[channel] = next[channel];
        clock_dirty_ = true;
      }
    }

    common_brightness_ = brightness;
    this->update_brightness_();
  }
  void set_alarm_indicator(bool enabled) {
    if (alarm_indicator_ != enabled) {
      alarm_indicator_ = enabled;
      clock_dirty_ = true;
    }
  }

protected:
  void start_();
  void stop_();
  void write_byte_(uint8_t value);
  void write_command_(uint8_t command);
  void write_display_();
  void update_clock_(bool force);
  void set_dim_percent_(uint8_t percent) {
    if (dim_percent_ == percent)
      return;
    dim_percent_ = percent;
    this->update_brightness_();
  }
  void update_brightness_();

  InternalGPIOPin *clk_{nullptr};
  InternalGPIOPin *dio_{nullptr};
  uint8_t ram_[16]{};
  uint8_t driver_brightness_{7};
  uint8_t driver_brightness_max_{7};
  uint8_t common_brightness_{255};
  uint8_t dim_dark_percent_{50};
  uint8_t dim_darker_percent_{25};
  uint8_t dim_percent_{100};
  bool display_on_{true};
  bool dirty_{true};
  time::RealTimeClock *time_{nullptr};
  bool temperature_display_active_{false};
  bool temperature_fahrenheit_{false};
  float temperature_celsius_{0.0f};
  uint32_t temperature_display_started_ms_{0};
  uint8_t rgb_on_[3]{1, 1, 1};
  bool twelve_hour_{true};
  bool blink_colon_{true};
  bool online_{false};

  bool no_sync_{true};
  bool alarm_indicator_{false};
  bool clock_dirty_{true};
  int last_clock_key_{-2};
  bool last_colon_{false};
};

class Tm1640Light : public light::LightOutput {
public:
  void set_display(Tm1640 *display) { display_ = display; }

  light::LightTraits get_traits() override {
    auto traits = light::LightTraits();
    traits.set_supported_color_modes({light::ColorMode::RGB});
    return traits;
  }

  void write_state(light::LightState *state) override;

protected:
  Tm1640 *display_{nullptr};
};

} // namespace tm1640
} // namespace esphome
