#include <Arduino.h>
#include <unity.h>
#include <Wire.h>
#include "LittleFS.h"
#include "SH1106Wire.h"
SH1106Wire display(0x3c, SDA, SCL);
// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }
void testDisplay()
{
    display.init();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    // display.drawString(32, 2, "           ");
    display.clear();
    display.drawString(32, 2, "22:22:21");
    display.display();
    display.drawString(32,10,"33333");
    delay(2000);
    display.setColor(BLACK);
    display.drawStringf(32,2,"%s","22:22:21");
    display.display();
}
void testI2c()
{
    byte error, address;
    int nDevices;

    Serial.println("Scanning...");

    nDevices = 0;
    for (address = 1; address < 127; address++)
    {

        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0)
        {
            Serial.print("I2C device found at address 0x");
            if (address < 16)
                Serial.print("0");
            Serial.print(address, HEX);
            Serial.println("  !");

            nDevices++;
        }
        else if (error == 4)
        {
            Serial.print("Unknow error at address 0x");
            if (address < 16)
                Serial.print("0");
            Serial.println(address, HEX);
        }
    }
    if (nDevices == 0)
        Serial.println("No I2C devices found\n");
    else
        Serial.println("done\n");
}
void test_led_builtin_pin_number(void)
{
    TEST_ASSERT_EQUAL(13, LED_BUILTIN);
}

void testOpenfile()
{
    TEST_ASSERT_TRUE(LittleFS.begin());
}

void testWritefile()
{
    File file = LittleFS.open("/data/test", "w");
    file.print("test");
    file.close();

    File f = LittleFS.open("/data/test", "r");
    String test = f.readString();

    TEST_ASSERT_TRUE(test.equals("test"));

    file = LittleFS.open("/data/test1", "w");
    file.print("test 2");
    file.close();

    file = LittleFS.open("/data/test2", "w");
    file.print("test 2");
    file.close();

    file = LittleFS.open("/data/1234567890010", "w");
    file.print("test long name");
    file.close();
}
void testOpenDir()
{
    // Dir dir = SPIFFS.openDir("/data");
    Dir dir = LittleFS.openDir("/data");
    Serial.println("");
    int count = 0;
    while (dir.next())
    {
        count++;
        Serial.print(dir.fileName());
        Serial.print(" ");
        if (dir.fileSize())
        {
            File f = dir.openFile("r");
            Serial.println(f.size());
        }
    }
    TEST_ASSERT_TRUE(count == 4);
}
void test_led_state_high(void)
{
    digitalWrite(LED_BUILTIN, HIGH);
    TEST_ASSERT_EQUAL(HIGH, digitalRead(LED_BUILTIN));
}

void test_led_state_low(void)
{
    digitalWrite(LED_BUILTIN, LOW);
    TEST_ASSERT_EQUAL(LOW, digitalRead(LED_BUILTIN));
}

void setup()
{
    Wire.begin();
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    delay(2000);

    UNITY_BEGIN(); // IMPORTANT LINE!
    // RUN_TEST(test_led_builtin_pin_number);
    // RUN_TEST(testI2c);
    // RUN_TEST(testOpenfile);
    // RUN_TEST(testWritefile);
    // RUN_TEST(testOpenDir);
    RUN_TEST(testDisplay);
    pinMode(LED_BUILTIN, OUTPUT);
    UNITY_END();
}

uint8_t i = 0;
uint8_t max_blinks = 5;

void loop()
{
    // if (i < max_blinks)
    // {
    //     RUN_TEST(test_led_state_high);
    //     delay(500);
    //     RUN_TEST(test_led_state_low);
    //     delay(500);
    //     i++;
    // }
    // else if (i == max_blinks)
    // {
    //     UNITY_END(); // stop unit testing
    // }
}