#include "gamepad.h"
#include "keypress.h"
#include "terminal.h"
#include <chrono>
#include <exception>
#include <iostream>
#include <thread>

int main()
{
    try {
        gamepad::Gamepad gamepad;
        terminal::Terminal term(false);
        for (;;) {
            term.cursorOff();
            term.printAt(1, 1, "Hello world");
            term.printAt(3, 1, "Press Escape to quit");
            term.render();
            int keyPress = term.getChar();
            if (keyPress == keyPress::ESC) {
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } catch (const std::exception& e) {
        std::cerr << e.what();
        return 1;
    }
    return 0;
}