# TM1640 ESPHome component

This ESP-IDF-compatible ESPHome component drives one TM1640 using ESPHome GPIO/HAL APIs.

With a `time_id`, it renders a clock into the chip's internal display RAM based on [CAI-274](http://www.diyleyuan.com/JC/274s.html) design.

Example ESPHome setup:

```yaml
tm1640:
  id: led_driver
  clk_pin: GPIO5
  dio_pin: GPIO6
  brightness: 255 # common brightness, 0-255
  driver_brightness: 7 # maximum TM1640 hardware brightness, 0-7
  time_id: sntp_time
  twelve_hour: true
  blink_colon: true
  red: 255
  green: 0
  blue: 0

light:
  - platform: tm1640
    id: display_color
    tm1640_id: led_driver
    name: Display Color
    restore_mode: RESTORE_DEFAULT_ON
    dim_dark: 50%
    dim_darker: 25%

time:
  - platform: sntp
    id: sntp_time
    timezone: !secret timezone
    on_time_sync:
      then:
        - logger.log: SNTP time synchronization succeeded
        - lambda: id(display)->set_no_sync(false);

```

The `Display Color` entity appears in Home Assistant as an RGB light with the standard color picker and brightness slider. Each RGB channel is on/off, so the color picker selects among seven additive colors plus off. The selected color applies to all four large digits.

The clock mode follows the documented CAI-274 grid layout: four large RGB digits show `HH:MM`, two full small digits show seconds, the two large decimal points blink as the colon, the third digit's point indicates PM in 12-hour mode, and the fourth digit's point is an alarm indicator. A channel is selected when its RGB value is at least 128. Common brightness maps to the TM1640's global 0-to-7 hardware duty setting, capped by `driver_brightness`. This brightness dims all lit channels and monochrome indicators together. The light's `dim_dark` and `dim_darker` percentages scale the selected common brightness without changing the HA light state or its brightness setting.

To enable clock mode, configure an ESPHome time source and set `time_id`. The component can still be used as a raw 16-grid output driver when `time_id` is omitted. Set a raw grid byte or output from a lambda:

```yaml
interval:
  - interval: 1s
    then:
      - lambda: |-
          id(led_driver)->set_grid_data(0, 0b0111111);
          id(led_driver)->set_output(1, 0, true);
```

Writes are buffered and sent together from the component loop. `set_output()` uses segment bit indices 0 through 7; `set_grid_data()` takes the raw byte. `set_alarm_indicator(bool)` controls the fourth large digit's decimal point when clock mode is active. The other raw outputs are overwritten by the clock's next frame, so use them only in raw-driver mode.

The large-digit mapping follows the documented grid order. On the tested panel, GRID13 segment bit 6 is the hourglass; bits 2-5 are °F, °C, week, and month. The hourglass mapping is hardwired and lights whenever the Wi-Fi callbacks report offline.

The glyph, PWM scaling, and clock-frame mapping have a host-side assert check:

```sh
c++ -std=c++17 components/tm1640/tests/test_display.cpp -o /tmp/test-display
/tmp/test-display
```
