#include <ISCDisplayMenu.h>

// Pines del encoder
#define ENCODER_PIN_CLK 27
#define ENCODER_PIN_DT 26
#define ENCODER_PIN_SW 25

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SSD1306_I2C_ADDRESS 0x3C

ISCDisplayMenu menu(ENCODER_PIN_CLK, ENCODER_PIN_DT, ENCODER_PIN_SW, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_I2C_ADDRESS);

void setup()
{
    Serial.begin(115200);
    menu.begin("Menu", "Home", "Adjust", "Value: ", "Select: ");
    menu.addNormalMenuItem("Wifi", []()
                           { Serial.println("Wifi selected"); });
    menu.addYesNoMenuItem("Reboot", []()
                          { ESP.restart(); }, []()
                          { Serial.println("Reboot canceled"); });
}

void loop()
{
    menu.loop();
}
