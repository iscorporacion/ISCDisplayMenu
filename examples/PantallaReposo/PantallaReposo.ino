#include <ISCDisplayMenu.h>

#define ENCODER_PIN_CLK 27
#define ENCODER_PIN_DT 26
#define ENCODER_PIN_SW 25

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SSD1306_I2C_ADDRESS 0x3C

ISCDisplayMenu menu(ENCODER_PIN_CLK, ENCODER_PIN_DT, ENCODER_PIN_SW, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_I2C_ADDRESS);

void setup()
{
    Serial.begin(115200);
    menu.begin("Menu Principal", "Pantalla Inicial");

    menu.showMainContent("Bienvenido", 2); // Mostrar mensaje inicial

    delay(5000); // Espera 5 segundos

    menu.setIdleScreenTitle("Nuevo Título"); // Cambiar el título de la pantalla de reposo
    menu.showMainContent("Listo", 1);        // Mostrar nuevo contenido
    menu.setIdleScreenSubtext("60", "bottom");
}

void loop()
{
    menu.loop();
}