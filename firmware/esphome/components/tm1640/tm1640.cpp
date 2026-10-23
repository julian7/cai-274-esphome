#include "tm1640.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "tm1640_display.h"

#include <cmath>

namespace esphome {
namespace tm1640 {

static const char *const TAG = "tm1640";

void Tm1640::setup() {
  clk_->setup();
  dio_->setup();
  clk_->digital_write(true);
  dio_->digital_write(true);

  // The TM1640 discards serial commands during its power-on reset interval.
  delayMicroseconds(200);
  if (time_ != nullptr)
    this->update_clock_(true);
  this->write_display_();
}

void Tm1640::loop() {
  if (temperature_display_active_ &&
      static_cast<uint32_t>(millis() - temperature_display_started_ms_) >=
          10000U) {
    temperature_display_active_ = false;
    clock_dirty_ = true;
  }
  if (time_ != nullptr)
    this->update_clock_(false);
  if (dirty_)
    this->write_display_();
}

void Tm1640::update_brightness_() {
  const uint8_t effective =
      display::scale_brightness(common_brightness_, dim_percent_);
  const uint8_t next =
      display::brightness_level(effective, driver_brightness_max_);
  if (driver_brightness_ != next) {
    driver_brightness_ = next;
    dirty_ = true;
  }
  this->set_display_on(effective > 0 && driver_brightness_max_ > 0);
}

void Tm1640::dump_config() {
  ESP_LOGCONFIG(TAG, "TM1640:");
  LOG_PIN("  CLK: ", clk_);
  LOG_PIN("  DIO: ", dio_);
  ESP_LOGCONFIG(TAG, "  Driver brightness: %u/7 (maximum %u)",
                driver_brightness_, driver_brightness_max_);
  if (time_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Clock: %s-hour, %s colon, RGB channels %u/%u/%u",
                  twelve_hour_ ? "12" : "24",
                  blink_colon_ ? "blinking" : "steady", rgb_on_[0], rgb_on_[1],
                  rgb_on_[2]);
  }
}

void Tm1640::set_grid_data(uint8_t address, uint8_t segments) {
  if (address >= sizeof(ram_) || ram_[address] == segments)
    return;
  ram_[address] = segments;
  dirty_ = true;
}

void Tm1640::set_output(uint8_t address, uint8_t segment, bool on) {
  if (address >= sizeof(ram_) || segment >= 8)
    return;

  const uint8_t mask = 1U << segment;
  uint8_t value = ram_[address];
  if (on)
    value |= mask;
  else
    value &= static_cast<uint8_t>(~mask);
  this->set_grid_data(address, value);
}

void Tm1640::clear() {
  for (auto &value : ram_) {
    if (value != 0) {
      value = 0;
      dirty_ = true;
    }
  }
}

void Tm1640::set_display_on(bool on) {
  if (display_on_ == on)
    return;
  display_on_ = on;
  dirty_ = true;
}

void Tm1640::start_() {
  dio_->digital_write(true);
  clk_->digital_write(true);
  delayMicroseconds(2);
  dio_->digital_write(false);
  delayMicroseconds(2);
  clk_->digital_write(false);
  delayMicroseconds(2);
}

void Tm1640::stop_() {
  clk_->digital_write(false);
  dio_->digital_write(false);
  delayMicroseconds(2);
  clk_->digital_write(true);
  delayMicroseconds(2);
  dio_->digital_write(true);
  delayMicroseconds(2);
}

void Tm1640::write_byte_(uint8_t value) {
  for (uint8_t bit = 0; bit < 8; bit++) {
    clk_->digital_write(false);
    dio_->digital_write((value & 1U) != 0);
    delayMicroseconds(1);
    clk_->digital_write(true);
    delayMicroseconds(1);
    value >>= 1;
  }
}

void Tm1640::write_command_(uint8_t command) {
  this->start_();
  this->write_byte_(command);
  this->stop_();
}

void Tm1640::show_temperature(float celsius, bool fahrenheit) {
  if (!std::isfinite(celsius) || celsius < -50.0f || celsius > 150.0f)
    return;

  temperature_celsius_ = celsius;
  temperature_fahrenheit_ = fahrenheit;
  temperature_display_started_ms_ = millis();
  temperature_display_active_ = true;
  clock_dirty_ = true;
}

void Tm1640::update_clock_(bool force) {
  const ESPTime now = time_->now();
  const bool valid = now.is_valid();
  const int key = valid ? now.hour * 3600 + now.minute * 60 + now.second : -1;
  const bool colon = !blink_colon_ || (valid && now.second % 2 == 0);
  if (!clock_dirty_ && !force && key == last_clock_key_ && colon == last_colon_)
    return;

  uint8_t frame[16]{};
  display::render_clock(frame, now.hour, now.minute, now.second, valid, online_,
                        twelve_hour_, colon, alarm_indicator_, rgb_on_,
                        no_sync_);
  if (temperature_display_active_) {
    float temperature = temperature_celsius_;
    if (temperature_fahrenheit_)
      temperature = temperature * 9.0f / 5.0f + 32.0f;
    display::render_temperature_small(frame, static_cast<int>(temperature),
                                      temperature_fahrenheit_);
  }
  for (uint8_t address = 0; address < sizeof(frame); address++)
    this->set_grid_data(address, frame[address]);

  last_clock_key_ = key;
  last_colon_ = colon;
  clock_dirty_ = false;
}

void Tm1640Light::write_state(light::LightState *state) {
  if (display_ == nullptr)
    return;

  const auto values = state->current_values;
  float brightness;
  state->current_values_as_brightness(&brightness);
  const auto to_byte = [](float value) {
    return static_cast<uint8_t>(lroundf(clamp(value, 0.0f, 1.0f) * 255.0f));
  };
  const uint8_t master = values.is_on() ? to_byte(brightness) : 0;
  display_->set_rgb(to_byte(values.get_red()), to_byte(values.get_green()),
                    to_byte(values.get_blue()), master);
}

void Tm1640::write_display_() {
  // Select auto-increment mode, then write the complete 16-byte display RAM.
  this->write_command_(0x40);
  this->start_();
  this->write_byte_(0xC0);
  for (uint8_t value : ram_)
    this->write_byte_(value);
  this->stop_();

  this->write_command_(0x80 | (display_on_ ? 0x08 : 0x00) | driver_brightness_);
  dirty_ = false;
}

} // namespace tm1640
} // namespace esphome
