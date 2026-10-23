# Repurpose a CAI-274 WiFi clock DIY kit

Based on the work of [Adrian Black's HU058D ESPHome](https://github.com/misterblack1/HU-058_ESPHome), I took a similar, but not the same WiFi clock DIY kit to modify for ESP Home: [Electronic DIY Soldering Loose Parts Digital Clock Kit with Sound Control Light Control Temperature Display 7 Color for Student](https://www.aliexpress.com/item/1005012977682148.html).

Instructions are coming on paper, with a QR code pointing to the [tutorial page](http://www.diyleyuan.com/JC/274s.html) (Chinese). Either way, the schematics is displayed on the [Chinese flyer](http://www.diyleyuan.com/JC/274/CAI274%E5%A3%B0%E6%8E%A7%E4%B8%83%E5%BD%A9%E6%97%B6%E9%92%9F%E8%AF%B4%E6%98%8E%E4%B9%A6.jpg) ("Instruction Manual for Sound-Activated Multicolor Clock").

## Different kits

There are many DIY clock kits out there, with different LED drivers:

* HU-058: driven by AiP33628
* CAI-201: driven by multi-chip shift register / latch matrix layout (SM5166P alongside two MBI5024 constant-current LED drivers)
* CAI-274: driven by [Titan Micro TM1640](https://www.lcsc.com/product-detail/C41327.html)

## CAI-274 details

### LED control

TM1640 can be controlled by 2-wire (DIN/CLK), so that our microcontroller will idle most of the time (unless other interesting tasks are given to them). While the chip is driven by 5V, it takes 0.7\*VCC as high signal by the datasheet. In practice, though, 0.66\*VCC (eg. 3.3V) is adequate.

### Display

The attached display is wired with RGB LEDs in a 7-segment display orientation for the four digits, and single-color LEDs for a 2.5-digit smaller display and other indicators:

- SEG1-8: segments a - g + dp
- GRID1-2-3: first digit R-G-B (dp: upper seconds dot)
- GRID4-5-6: second digit R-G-B (dp: lower seconds dot)
- GRID7-8-9: third digit R-G-B (dp: PM indicator)
- GRID10-11-12: fourth digit R-G-B (dp: alarm signal at seconds dot)
- GRID13:
  - segments a-b: first small digit (half)
  - segment c: ºF symbol
  - segment d: ºC symbol
  - segment e: week symbol
  - segment f: month symbol
  - segment g: hourglass symbol
- GRID14: second small digit (a-g; no dp segment)
- GRID15: third small digit (a-g; no dp segment)
- GRID16: unused

### Signals

Other than LED control, there are a few more signals available:

Input:

- 4 * buttons (floating, active low)
- Voice activation (5V)
- Thermistor (5V)
- Photoresistor (5V)

Output:

- 2 * LED  (DIN/CLK, 5V), as previously mentioned
- Buzzer (5V, active low)

### Alternative power sources

Apart from the USB-C power input, the board can use dry, NiMH, Li-Ion batteries. While they are great options, I haven't played with them too much. Unfortunately, the LiIon battery I have (2500mAh) doesn't fit the case.

Note: the CR1220 battery is solely for the RTC clock chip, which is largely useless if the STC8G chip is replaced with a dev board.

## ESP32-C3 SuperMini

I've chosen a cheap [ESP32-C3 SuperMini](https://www.nologo.tech/en/product/esp32/esp32c3SuperMini/esp32C3SuperMini.html) board from Aliexpress (sorry, no link), making sure the MCU code has an "F" in it, which designates the presence of Flash storage.

### 5V vs 3.3V

ESP32-C3 is running on 3.3V, and its GPIO pins are not 5V-resistant. Therefore, precausions are needed for connecting the two.

- LED control is easy: R01 and R02 are 1kΩ pull-up resistors on the way to TM1640. Replacing them with  the ESP32-C3's internal pull-ups is most likely adequate to drive the LED driver.
- Voice activation: while it's driven on the 5V rail, the sound activator is AC-coupled, so an input pull-up pin can deal with the spike.
- Thermistor, photoresistor: these are pulled to 5V. Possible solution would be not to solder the 5V legs of R1/R2 pull-up resistors, and wiring them to 3.3V. Then, Pin 3 and 4 on the original STC8G chip can be brought to ADC pins of the MCU, which then could handle them with 12dB attenuation.
- Buzzer: it is directly wired to the 5V, but the PCB design makes it easy to cut the trace, and the component can be replaced with a 3V version.

### GPIO wiring

Wire the various pins of the MCU dev board to the legs of the original STC8G chip:

| ESP32-C3 | Function | STC8G pin |
| :--- | :--- | :---: |
| GPIO 0 | thermistor (NTC) | 3 |
| GPIO 1 | photoresistor (LDR) | 4 |
| GPIO 2 | Button 1 | 12 |
| GPIO 3 | buzzer | 10 |
| GPIO 4 | voice activation | 16 |
| GPIO 5 | LED CLK | 1 |
| GPIO 6 | LED DIN | 2 |
| GPIO 7 | I2C SDA | - |
| GPIO 8 | Button 2 | 15 |
| GPIO 9 | (leave empty) | - |
| GPIO 10 | I2C SCL | - |
| GPIO 20 | Button 3 | 11 |
| GPIO 21 | Button 4 | 9 |
