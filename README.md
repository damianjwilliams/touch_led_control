ESP-IDF code to control 3 LEDs connected to an ESP32-P4 dev board using the touch screen that is included in the dev kit. The LEDs can be toggled on and off with a button, and the brightness controlled by sliders. 

GUI code uses the ESP32 port of the LVGL graphics library, and contains code from the LVGL v9 example in ESP-IDF. Brightness is controlled using PWM. 

The ESP32-P4 dev kit including touch-screen can be found at [AliExpress](https://www.aliexpress.us/item/3256807624599045.html). LVGL information can be found in the package [documentation](https://docs.lvgl.io/master/)
