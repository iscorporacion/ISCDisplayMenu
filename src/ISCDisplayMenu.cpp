#include "ISCDisplayMenu.h"

ISCDisplayMenu::ISCDisplayMenu(uint8_t clkPin, uint8_t dtPin, uint8_t swPin, uint8_t screenWidth, uint8_t screenHeight, int i2cAddress)
    : clkPin(clkPin), dtPin(dtPin), swPin(swPin), sensitivity(2), currentOption(0), scrollOffset(0), lastPosition(0), inSubMenu(false), buttonHandled(false), lastDebounceTime(0),
      debounceDelay(50), i2cAddress(i2cAddress), menuActive(false), idleContent(""), idleContentTextSize(1)
{
    display = new Adafruit_SSD1306(screenWidth, screenHeight, &Wire, -1);
}

void ISCDisplayMenu::begin(const std::string &menuTitle, const std::string &idleScreenText, const std::string &adjustValueTitle, const std::string &adjustValueSubtitle,
                           const std::string &yesNoTitle)
{
    pinMode(swPin, INPUT_PULLUP);

    this->menuTitle = menuTitle;
    this->idleScreenText = idleScreenText;
    this->adjustValueTitle = adjustValueTitle;
    this->adjustValueSubtitle = adjustValueSubtitle;
    this->yesNoTitle = yesNoTitle;

    // Configuración del encoder
    encoder.attachHalfQuad(dtPin, clkPin);
    encoder.clearCount();
    encoder.setCount(0);

    // Configuración del display
    if (!display->begin(SSD1306_SWITCHCAPVCC, i2cAddress))
    {
        Serial.print(F("Fallo al inicializar OLED en dirección I2C: 0x"));
        Serial.println(i2cAddress, HEX);
        while (true)
        {
            // Bloquea el programa si el display falla
        }
    }

    Serial.print(F("OLED inicializado correctamente en dirección I2C: 0x"));
    Serial.println(i2cAddress, HEX);

    // Configuración inicial
    menuActive = false; // Asegurar que inicia en reposo
    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE);
    display->setCursor(0, 0);
    display->println(idleScreenText.c_str());
    display->setCursor(0, 16);
    display->println(F("Presiona para iniciar"));
    display->display();
}

void ISCDisplayMenu::toggleMenu()
{
    if (inSubMenu)
        return; // No alternar si estamos en un submenú

    menuActive = !menuActive;
    display->clearDisplay();

    if (!menuActive)
    {
        // Dibujar la pantalla de reposo con el título y el contenido almacenado
        display->setCursor(0, 0);
        display->setTextSize(1); // Tamaño normal para el título
        display->setTextColor(SSD1306_WHITE);
        display->println(idleScreenText.c_str()); // Título de la pantalla de reposo

        // Restaurar el contenido actual de la pantalla de reposo con el tamaño configurado
        showMainContent(idleContent, idleContentTextSize);
    }
    else
    {
        renderMenu(); // Mostrar el menú principal
    }

    display->display(); // Reflejar los cambios
}

void ISCDisplayMenu::closeMenu()
{
    if (menuActive)
    {
        menuActive = false; // Desactiva el menú
        display->clearDisplay();
        display->setCursor(0, 0);
        display->setTextSize(1); // Tamaño normal para el título
        display->setTextColor(SSD1306_WHITE);
        display->println(idleScreenText.c_str()); // Título de la pantalla de reposo

        // Redibujar el contenido actual de la pantalla de reposo
        showMainContent(idleContent, idleContentTextSize);

        display->display(); // Reflejar los cambios
    }
}

void ISCDisplayMenu::addYesNoMenuItem(const std::string &name, void (*yesAction)(), void (*noAction)(), const std::string &yesText, const std::string &noText)
{
    MenuItem item = {YES_NO, name, 0, 0, 0, yesText, noText, yesAction, noAction, nullptr};
    menuItems.push_back(item);
}

void ISCDisplayMenu::addChangeMenuItem(const std::string &name, int value, int minValue, int maxValue, void (*changeAction)(int))
{
    MenuItem item = {CHANGE_TYPE, name, value, minValue, maxValue, "", "", nullptr, nullptr, changeAction};

    menuItems.push_back(item);
}

void ISCDisplayMenu::addNormalMenuItem(const std::string &name, void (*action)())
{
    MenuItem item = {NORMAL, name, 0, 0, 0, "", "", action, nullptr, nullptr};
    menuItems.push_back(item);
}

void ISCDisplayMenu::handleEncoderMovement(int movement)
{
    int menuSize = menuItems.size();
    currentOption = (currentOption + movement + menuSize) % menuSize;

    // Ajustar el scroll si excede el límite visible
    if (currentOption < scrollOffset)
    {
        scrollOffset = currentOption;
    }
    else if (currentOption >= scrollOffset + 4)
    {
        scrollOffset = currentOption - 3;
    }
}

void ISCDisplayMenu::handleButtonPress()
{
    if (!menuActive)
        return; // Salir si el menú está inactivo

    MenuItem &item = menuItems[currentOption];
    switch (item.type)
    {
    case YES_NO:
        inSubMenu = true;
        renderYesNoMenu();
        break;
    case CHANGE_TYPE:
        inSubMenu = true;
        handleChange(item);
        break;
    case NORMAL:
        if (item.yesAction)
        {
            item.yesAction();
        }
        break;
    default:
        break;
    }
}

void ISCDisplayMenu::renderMenu()
{
    display->clearDisplay(); // Limpia toda la pantalla antes de dibujar

    display->setTextSize(1); // Asegurar tamaño de texto normal
    display->setTextColor(SSD1306_WHITE);
    display->setCursor(0, 0);
    display->println(menuTitle.c_str()); // Mostrar el título del menú principal

    for (int i = 0; i < 4 && (i + scrollOffset) < menuItems.size(); i++)
    {
        int index = i + scrollOffset;
        display->setCursor(0, 16 + i * 12);
        display->print((index == currentOption) ? "> " : "  ");
        display->print(menuItems[index].name.c_str());
        if (menuItems[index].type == CHANGE_TYPE)
        {
            std::string valueStr = std::to_string(menuItems[index].value);
            int valueWidth = valueStr.length() * 6;       // Ancho estimado del valor (6px por carácter)
            int position = SCREEN_WIDTH - valueWidth - 2; // 2px de margen derecho

            // Mover el cursor para imprimir el valor
            display->setCursor(position, 16 + i * 12);
            display->print(valueStr.c_str());
        }
    }
    display->display(); // Reflejar los cambios en la pantalla
}

void ISCDisplayMenu::loop()
{
    if (isButtonPressed() && !buttonHandled)
    {
        buttonHandled = true;

        if (!menuActive)
        {
            // Activar el menú desde la pantalla de reposo
            toggleMenu();
        }
        else if (!inSubMenu)
        {
            // Manejar acciones del menú
            handleButtonPress();
        }
    }

    if (digitalRead(swPin) == HIGH)
    {
        buttonHandled = false;
    }

    if (menuActive && !inSubMenu)
    {
        // Actualizar la posición del encoder y redibujar el menú
        int newPosition = encoder.getCount() / sensitivity;
        int movement = newPosition - lastPosition;

        if (movement != 0)
        {
            lastPosition = newPosition;
            handleEncoderMovement(movement);
        }

        renderMenu();
    }
}

bool ISCDisplayMenu::isButtonPressed()
{
    if (digitalRead(swPin) == LOW)
    {
        unsigned long currentTime = millis();
        if (currentTime - lastDebounceTime > debounceDelay)
        {
            lastDebounceTime = currentTime;
            return true;
        }
    }
    return false;
}

void ISCDisplayMenu::closeMenuAction()
{
    Serial.println(F("Regresando a pantalla de reposo"));
    toggleMenu(); // Alternar el estado del menú
}

void ISCDisplayMenu::showMainContent(const std::string &text, uint8_t textSize)
{
    if (textSize > 3)
        textSize = 3; // Limitar tamaño máximo del texto principal

    idleContent = text;             // Guardar el contenido actual
    idleContentTextSize = textSize; // Guardar el tamaño del texto

    display->clearDisplay();
    display->setTextColor(SSD1306_WHITE);

    // Mostrar título de pantalla de reposo
    display->setTextSize(1);
    display->setCursor(0, 0);
    display->println(idleScreenText.c_str());

    // Calcular posición del texto principal
    int16_t charWidth = 6 * textSize;
    int16_t charHeight = 8 * textSize;
    int16_t x = (SCREEN_WIDTH - (text.length() * charWidth)) / 2;
    int16_t y = ((SCREEN_HEIGHT - 16) - charHeight) / 2 + 16;

    // Dibujar texto principal
    display->setTextSize(textSize);
    display->setCursor(x, y);
    display->print(text.c_str());

    // Dibujar subtexto si está configurado
    if (!idleSubtext.empty())
    {
        display->setTextSize(1); // Subtexto siempre en tamaño base
        if (idleSubtextPosition == "above")
        {
            display->setCursor((SCREEN_WIDTH - (idleSubtext.length() * 6)) / 2, y - 12);
        }
        else
        { // below
            display->setCursor((SCREEN_WIDTH - (idleSubtext.length() * 6)) / 2, y + charHeight + 2);
        }
        display->print(idleSubtext.c_str());
    }

    display->display();
}

void ISCDisplayMenu::setIdleScreenSubtext(const std::string &subtext, const std::string &position)
{
    idleSubtext = subtext;
    idleSubtextPosition = (position == "top" || position == "bottom") ? position : "bottom";
    if (!menuActive)
    {
        // Actualizar pantalla de reposo si el menú no está activo
        showMainContent(idleContent, idleContentTextSize);
    }
}

void ISCDisplayMenu::handleChange(MenuItem &item)
{
    int newValue = item.value;
    lastPosition = encoder.getCount() / sensitivity; // Sincronizar posición del encoder

    while (inSubMenu)
    {
        int position = encoder.getCount() / sensitivity;
        newValue += position - lastPosition;
        lastPosition = position;

        // Asegurarse de que el nuevo valor esté dentro de los límites
        if (newValue < item.minValue)
            newValue = item.minValue;
        if (newValue > item.maxValue)
            newValue = item.maxValue;

        display->clearDisplay();
        display->setCursor(0, 0);
        display->println(adjustValueTitle.c_str());
        display->setCursor(0, 20);
        display->print(adjustValueSubtitle.c_str());

        std::string valueStr = std::to_string(newValue);
        int valueWidth = valueStr.length() * 6;        // 6px por carácter
        int positionX = SCREEN_WIDTH - valueWidth - 2; // 2px de margen derecho

        // Dibujar el valor justificado
        display->setCursor(positionX, 20);
        display->print(valueStr.c_str());

        display->display();

        if (isButtonPressed() && !buttonHandled)
        {
            buttonHandled = true;
            if (item.changeAction)
            {
                item.changeAction(newValue); // Llama a la acción con el nuevo valor
            }
            item.value = newValue; // Actualiza el valor almacenado
            inSubMenu = false;
        }

        if (digitalRead(swPin) == HIGH)
        {
            buttonHandled = false;
        }
    }
    renderMenu(); // Volver al menú principal
}

void ISCDisplayMenu::renderYesNoMenu()
{
    bool selection = true;
    lastPosition = encoder.getCount() / sensitivity; // Sincronizar posición del encoder
    int selectionOffset = 0;

    while (inSubMenu)
    {
        int position = encoder.getCount() / sensitivity;
        selectionOffset += position - lastPosition;
        lastPosition = position;

        // Asegurar que selectionOffset se mantenga entre 0 y 1
        if (selectionOffset < 0)
            selectionOffset = 0;
        if (selectionOffset > 1)
            selectionOffset = 1;

        selection = (selectionOffset == 1);

        display->clearDisplay();
        display->setCursor(0, 0);
        display->println(yesNoTitle.c_str());

        // Mostrar las opciones con los textos personalizados
        display->setCursor(0, 20);
        display->print(selection ? "> " : "  ");
        display->print(menuItems[currentOption].yesText.c_str());

        display->setCursor(0, 30);
        display->print(!selection ? "> " : "  ");
        display->print(menuItems[currentOption].noText.c_str());

        display->display();

        if (isButtonPressed() && !buttonHandled)
        {
            buttonHandled = true;
            if (selection && menuItems[currentOption].yesAction)
            {
                menuItems[currentOption].yesAction();
            }
            else if (!selection && menuItems[currentOption].noAction)
            {
                menuItems[currentOption].noAction();
            }
            inSubMenu = false;
        }

        if (digitalRead(swPin) == HIGH)
        {
            buttonHandled = false;
        }
    }

    renderMenu(); // Volver al menú principal
}

void ISCDisplayMenu::setIdleScreenTitle(const std::string &newTitle)
{
    idleScreenText = newTitle; // Actualiza el título

    // Si el menú no está activo, actualiza inmediatamente la pantalla de reposo
    if (!menuActive)
    {
        display->clearDisplay();
        display->setCursor(0, 0);
        display->setTextSize(1); // Tamaño normal para el título
        display->setTextColor(SSD1306_WHITE);
        display->println(idleScreenText.c_str()); // Título actualizado

        // Redibujar el contenido actual de la pantalla de reposo
        showMainContent(idleContent, idleContentTextSize);

        display->display(); // Reflejar los cambios
    }
}

bool ISCDisplayMenu::isMenuActive() const
{
    return menuActive; // Devuelve el estado del menú
}