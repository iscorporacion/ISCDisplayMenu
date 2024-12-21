#ifndef ISCDISPLAYMENU_H
#define ISCDISPLAYMENU_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Encoder.h>
#include <Wire.h>
#include <string>
#include <vector>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SSD1306_I2C_ADDRESS 0x3C

class ISCDisplayMenu
{
public:
    enum MenuType
    {
        YES_NO,
        CHANGE_TYPE,
        NORMAL,
        SUBMENU
    };
    struct MenuItem
    {
        MenuType type;
        std::string name;
        int value;                 // Valor actual para CHANGE_TYPE
        int minValue;              // Valor mínimo permitido (solo para CHANGE_TYPE)
        int maxValue;              // Valor máximo permitido (solo para CHANGE_TYPE)
        std::string yesText;       // Texto personalizado para YES
        std::string noText;        // Texto personalizado para NO
        void (*yesAction)();       // Puntero a función para YES_NO
        void (*noAction)();        // Puntero a función para YES_NO
        void (*changeAction)(int); // Puntero a función para CHANGE_TYPE
    };

    ISCDisplayMenu(uint8_t clkPin, uint8_t dtPin, uint8_t swPin, uint8_t screenWidth, uint8_t screenHeight, int i2cAddress = SSD1306_I2C_ADDRESS);

    void begin(const std::string &menuTitle, const std::string &idleScreenText, const std::string &adjustValueTitle = "Ajustar Valor",
               const std::string &adjustValueSubtitle = "Valor: ", const std::string &yesNoTitle = "Selecciona: ");
    void addYesNoMenuItem(const std::string &name, void (*yesAction)(), void (*noAction)() = nullptr, const std::string &yesText = "YES", const std::string &noText = "NO");
    void addChangeMenuItem(const std::string &name, int value, int minValue, int maxValue, void (*changeAction)(int));
    void addNormalMenuItem(const std::string &name, void (*action)());
    void showMainContent(const std::string &text, uint8_t textSize = 1);
    void setIdleScreenTitle(const std::string &newTitle);
    void setIdleScreenSubtext(const std::string &subtext, const std::string &position = "top");

    void handleEncoderMovement(int movement);
    bool isMenuActive() const;
    void handleButtonPress();
    void renderMenu();
    void toggleMenu();
    void closeMenu();
    void closeMenuAction();
    void loop();

private:
    Adafruit_SSD1306 *display;
    ESP32Encoder encoder;
    std::vector<MenuItem> menuItems;
    std::string menuTitle;           // Título del menú principal
    std::string idleScreenText;      // Título de la pantalla de reposo
    std::string idleContent;         // Contenido adicional en la pantalla de reposo
    uint8_t idleContentTextSize;     // Tamaño del texto actual en la pantalla de reposo
    std::string adjustValueTitle;    // Título para "Ajustar Valor"
    std::string adjustValueSubtitle; // Subtítulo para "Valor:"
    std::string yesNoTitle;          // Título para "Selecciona:"
    std::string idleSubtext;         // Subtexto adicional en la pantalla de reposo
    std::string idleSubtextPosition; // Posición del subtexto: "top" o "bottom"

    uint8_t clkPin, dtPin, swPin;
    int i2cAddress;
    int sensitivity;
    int currentOption;
    int scrollOffset;
    int lastPosition;
    bool inSubMenu;
    bool buttonHandled;

    unsigned long lastDebounceTime;
    const unsigned long debounceDelay;

    void renderYesNoMenu();
    void handleChange(MenuItem &item);
    bool isButtonPressed();
    bool menuActive;
};

#endif // ISCDISPLAYMENU_H
