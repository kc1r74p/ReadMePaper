#include <WiFi.h>
#include <FS.h>
#include <aREST.h>
#include <HTTPClient.h>
#include "epd5in65f/epd5in65f.h"
#include "bmp24.h"
#include "SPIFFS.h"

// USING Adafruit ESP32 Feather PINS
#define LED_PIN 13

// https://github.com/espressif/arduino-esp32/blob/master/variants/feather_esp32/pins_arduino.h
/*
static const uint8_t SDA = 23;
static const uint8_t SCL = 22;

static const uint8_t MOSI = 18;
static const uint8_t MISO = 19;
static const uint8_t SCK = 5;

static const uint8_t SS = 33; // CS
static const uint8_t DC = 15;
static const uint8_t RST = 32;
static const uint8_t BUSY = 14;
*/

// WiFi credentials.
const char *WIFI_SSID = "***";
const char *WIFI_PASS = "***";

WiFiClient client = NULL;
WiFiServer server(80);
aREST rest = aREST();
Epd epd;

//out vars
unsigned long runtime = 0;
uint32_t freeHeap = 0;
ulong dtime = 0;

const bool DEBUG_FLAG = true;

void WiFiEvent(WiFiEvent_t event)
{
    if (DEBUG_FLAG)
        Serial.printf("[WiFi-event] event: %d\n", event);

    switch (event)
    {
    case SYSTEM_EVENT_STA_GOT_IP:
        if (DEBUG_FLAG)
            Serial.println("WiFi connected");
        if (DEBUG_FLAG)
            Serial.println("IP address: ");
        if (DEBUG_FLAG)
            Serial.println(WiFi.localIP());
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        if (DEBUG_FLAG)
            Serial.println("WiFi lost connection");
        delay(2000);
        WiFi.disconnect(true);
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        break;
    }
}

String mac2String(const uint64_t mac)
{
    byte ar[6];
    uint8_t *p = ar;

    for (int8_t i = 0; i <= 5; i++)
    {
        *p++ = mac >> (CHAR_BIT * i);
    }

    String s;
    for (int8_t i = 0; i < 6; i++)
    {
        char buf[2];
        sprintf(buf, "%02X", ar[i]);
        s += buf;
        if (i < 5)
            s += ':';
    }
    s += '\0';
    return s;
}

// https://forum.arduino.cc/index.php?topic=565603.0
void downloadAndSaveFile(String fileName, String url)
{
    HTTPClient http;
    http.begin(url);

    int httpCode = http.GET();
    if (httpCode > 0)
    {
        File file = SPIFFS.open(fileName, FILE_WRITE);
        // file found at server
        if (httpCode == HTTP_CODE_OK)
        {
            int len = http.getSize();
            int buff_size = 8128;
            unsigned char *buff = (unsigned char *)malloc(buff_size);
            WiFiClient *stream = http.getStreamPtr();
            while (http.connected() && (len > 0 || len == -1))
            {
                // Available limited to 16328 bytes. Might be TLS segementation.
                size_t size = stream->available();
                if (size)
                {
                    int c = stream->readBytes(buff, ((size > buff_size) ? buff_size : size));
                    file.write(buff, c);
                    if (len > 0)
                    {
                        len -= c;
                    }
                }
                delay(1);
            }
            file.close();
            free(buff);
        }
    }
    http.end();
    delay(1500);
}

const unsigned char epd_palette[8][3] = {
    {0, 0, 0},       /*BLACK*/
    {255, 255, 255}, /*WHITE*/
    {0, 255, 0},     /*GREEN*/
    {0, 0, 255},     /*BLUE*/
    {255, 0, 0},     /*RED*/
    {255, 247, 0},   /*YELLOW*/
    {255, 123, 0},   /*ORANGE*/
    {255, 126, 221}  /*MAGENTA*/
};

unsigned char findClosestEPD(unsigned char r, unsigned char g, unsigned char b)
{
    unsigned char closest = 0;
    unsigned long closest_dist = 99999999;
    for (int i = 0; i < 8; i++)
    {
        unsigned char p_r = epd_palette[i][0];
        unsigned char p_g = epd_palette[i][1];
        unsigned char p_b = epd_palette[i][2];
        unsigned long temp_dist =
            (r - p_r) * (r - p_r) +
            (g - p_g) * (g - p_g) +
            (b - p_b) * (b - p_b);

        if (temp_dist < closest_dist)
        {
            closest_dist = temp_dist;
            closest = i;
        }
    }
    return closest;
}

int setImageFromFS(String path)
{
    File f = SPIFFS.open((const char *)path.c_str());
    BMP *bitmap = BMP_ReadFile(f);

    if (!bitmap)
    {
        Serial.printf("BMP ReadFile error!");
        return 1;
    }

    int img_x0 = 0;
    int img_y0 = 0;

    int img_h = BMP_GetHeight(bitmap);
    int img_w = BMP_GetWidth(bitmap);
    Serial.printf("Image H: %d W: %d \n", img_h, img_w);

    if (img_w > EPD_WIDTH || img_h > EPD_HEIGHT)
    {
        Serial.printf("Image too wide or tall!");
        BMP_Free(bitmap);
        f.close();
        return 1;
    }

    //auto center image
    img_x0 = (EPD_WIDTH - img_w) / 2;
    img_y0 = (EPD_HEIGHT - img_h) / 2;

    Serial.print("EPD Display... \n");

    unsigned long i, j;
    unsigned char r = 0, g = 0, b = 0;
    unsigned char r2 = 0, g2 = 0, b2 = 0;
    epd.SendCommand(0x61); //Set Resolution setting
    epd.SendData(0x02);
    epd.SendData(0x58);
    epd.SendData(0x01);
    epd.SendData(0xC0);
    epd.SendCommand(0x10);
    for (i = 0; i < EPD_HEIGHT; i++)
    {
        for (j = 0; j < EPD_WIDTH / 2; j++)
            if (i < img_h + img_y0 && i >= img_y0 && j < (img_w + img_x0) / 2 && j >= img_x0 / 2)
            {
                int x = j * 2 - img_x0;
                int y = i - img_y0;

                BMP_GetPixelRGB(f, bitmap, x, y, &r, &g, &b);
                BMP_GetPixelRGB(f, bitmap, x + 1, y, &r2, &g2, &b2);

                //1 byte -> sets 2 pixels -> x and x+1
                epd.SendData((findClosestEPD(r, g, b) << 4) | findClosestEPD(r2, g2, b2));
            }
            else
            {
                epd.SendData(0x11);
            }
    }

    epd.SendCommand(0x04); //0x04
    epd.EPD_5IN65F_BusyHigh();
    epd.SendCommand(0x12); //0x12
    epd.EPD_5IN65F_BusyHigh();
    epd.SendCommand(0x02); //0x02
    epd.EPD_5IN65F_BusyLow();
    delay(200);

    BMP_Free(bitmap);
    f.close();
    return 0;
}

int setColors(String command)
{
    epd.EPD_5IN65F_Show7Block();
    return 0;
}

int clear(String command)
{
    epd.Clear(EPD_5IN65F_WHITE);
    return 0;
}

// https://github.com/zenmanenergy/ESP8266-Arduino-Examples/blob/master/helloWorld_urlencoded/urlencode.ino
unsigned char h2int(char c)
{
    if (c >= '0' && c <= '9')
    {
        return ((unsigned char)c - '0');
    }
    if (c >= 'a' && c <= 'f')
    {
        return ((unsigned char)c - 'a' + 10);
    }
    if (c >= 'A' && c <= 'F')
    {
        return ((unsigned char)c - 'A' + 10);
    }
    return (0);
}

String urldecode(String str)
{
    String encodedString = "";
    char c;
    char code0;
    char code1;
    for (int i = 0; i < str.length(); i++)
    {
        c = str.charAt(i);
        if (c == '+')
        {
            encodedString += ' ';
        }
        else if (c == '%')
        {
            i++;
            code0 = str.charAt(i);
            i++;
            code1 = str.charAt(i);
            c = (h2int(code0) << 4) | h2int(code1);
            encodedString += c;
        }
        else
        {
            encodedString += c;
        }
        yield();
    }
    return encodedString;
}

void debugFS()
{
    Serial.printf("SPIFFS usage %d/%d heap free: %d/%d\n", SPIFFS.usedBytes(), SPIFFS.totalBytes(), ESP.getFreeHeap(), ESP.getHeapSize());
}

int loadImageFromWeb(String command)
{
    if (command.length() < 1)
        return -2;

    downloadAndSaveFile("/tmp.bmp", urldecode(command).c_str());
    setImageFromFS("/tmp.bmp");
    SPIFFS.remove("/tmp.bmp");
    debugFS();
    return 0;
}

void setup()
{
    if (DEBUG_FLAG)
        Serial.begin(115200);

    if (epd.Init() != 0)
    {
        Serial.println("e-Paper init failed");
        return;
    }

    sleep(1);

    if (!SPIFFS.begin(true))
    {
        Serial.println("SPIFFS initialisation failed!");
    }
    debugFS();

    // Connect to Wifi.
    if (DEBUG_FLAG)
        Serial.println();
    if (DEBUG_FLAG)
        Serial.println();
    if (DEBUG_FLAG)
        Serial.print("Connecting to ");
    if (DEBUG_FLAG)
        Serial.println(WIFI_SSID);

    delay(100);

    WiFi.onEvent(WiFiEvent);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    if (DEBUG_FLAG)
        Serial.println("Connecting...");

    rest.set_name(mac2String(ESP.getEfuseMac()).c_str());
    rest.set_id("epaper_esp32");
    rest.variable("runtime", &runtime);
    rest.variable("freeHeap", &freeHeap);

    rest.function("colors", setColors);
    rest.function("clear", clear);
    rest.function("load", loadImageFromWeb);

    server.begin();
}

void handleRequests()
{
    client = server.available();
    freeHeap = ESP.getFreeHeap();
    if (client && client.available())
    {
        if (DEBUG_FLAG)
            Serial.print("Processing req from ");
        if (DEBUG_FLAG)
            Serial.println(client.remoteIP().toString().c_str());
        ulong start_ms = millis();

        rest.handle(client);

        if (DEBUG_FLAG)
            Serial.print(" in ");
        if (DEBUG_FLAG)
            Serial.print(millis() - start_ms);
        if (DEBUG_FLAG)
            Serial.println(" ms");
    }

    delay(10);
    client.stop();
}

void loop()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        handleRequests();
    }
    else
    {
        if (DEBUG_FLAG)
            Serial.println("WiFi connection lost ... reconnecting... ");

        WiFi.disconnect(true);
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        int WLcount = 0;
        while (WiFi.status() != WL_CONNECTED && WLcount < 200)
        {
            delay(100);
            if (DEBUG_FLAG)
                Serial.printf(".");
            ++WLcount;
        }
    }

    //prevent random hangup ...
    //15min DC to auto reconnect
    if (millis() > (dtime + (1000 * 60 * 15)))
    {
        if (DEBUG_FLAG)
            Serial.println("[schedule] dc wifi to auto reconnect...");
        WiFi.disconnect(true);
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        dtime = millis();
    }
}
