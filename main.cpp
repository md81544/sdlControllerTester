#include "gamepad.h"
#include "keypress.h"
#include "log.h"
#include "terminal.h"
#include <chrono>
#include <exception>
#include <format>
#include <iostream>
#include <thread>

int main()
{
    try {
        mgo::Log::init(
            "debug.log", mgo::Log::Level::Debug, false); // false = don't append (i.e. overwrite)
        gamepad::Gamepad gamepad;
        terminal::Terminal term(false);
        for (;;) {
            term.cursorOff();
            auto evts = gamepad.getEvents();
            float lx = 0.f;
            float ly = 0.f;
            float rx = 0.f;
            float ry = 0.f;
            if (!gamepad.isGamePadAttached()) {
                term.printAt(1, 1, "Please attach a gamepad!");
            } else {
                term.printAt(1, 1, "SDL Gamepad Tester");
                term.printAt(3, 1, "Press Escape to quit");
                term.printAt(5, 1, "Left stick  X :");
                term.printAt(6, 1, "Left stick  Y :");
                term.printAt(7, 1, "Right stick X :");
                term.printAt(8, 1, "Right stick Y :");
                for (const auto& e : evts) {
                    if (e.eventType == gamepad::EventType::Analogue) {
                        lx = e.analogue.leftX;
                        ly = e.analogue.leftY;
                        rx = e.analogue.rightX;
                        ry = e.analogue.rightY;
                    }
                }
                term.printAt(5, 17, std::format("{:>6.3f}", lx));
                term.printAt(6, 17, std::format("{:>6.3f}", ly));
                term.printAt(7, 17, std::format("{:>6.3f}", rx));
                term.printAt(8, 17, std::format("{:>6.3f}", ry));
            }
            term.render();
            // Non-blocking key press check:
            std::optional<int> key = keyPress::getKeyPress(false);
            if (key.has_value()) {
                if (*key == keyPress::ESC) {
                    break;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } catch (const std::exception& e) {
        std::cerr << e.what();
        return 1;
    }
    return 0;
}