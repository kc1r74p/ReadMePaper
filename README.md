# ReadMePaper
Just some ESP32 - ePaper 7 Color project 🎉

### Parts used:
- ePaper screen: https://www.waveshare.com/5.65inch-e-paper-module-f.htm
- ESP32 Feather: https://www.adafruit.com/product/3405

### Examples of 24bit BMP ~700kb across WiFi to ESP32 SPIFFS to ePaper
![Example](epaper_example.png)
![Example2](perserverance_first.png)

### Set image via REST request
Using any REST client in the same Network
```
GET: http://<esp32_ip>/load?path=<urlencode(BMP_URL_FOR24BIT_NON_COMPRESSED_IMAGE)>
```
