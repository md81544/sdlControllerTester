#include "gamepad.h"
#include "keypress.h"
#include "log.h"
#include "terminal.h"
#include <chrono>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <exception>
#include <format>
#include <iostream>
#include <thread>

template <typename T>
concept FloatOrBool = std::same_as<T, float> || std::same_as<T, bool>;

template <FloatOrBool T>
void highlight(terminal::Terminal& term, std::size_t row, std::size_t col, T value)
{
    terminal::ColourGuard cg(&term);
    if constexpr (std::same_as<T, float>) {
        float v = value;
        if (std::abs(v) < 0.01f) {
            v = 0.f;
        } else {
            term.setFgColour(terminal::Colour::BrightYellow);
        }
        term.printAt(row, col, std::format("{:>6.2f}", v));
    } else {
        if (value) {
            term.setFgColour(terminal::Colour::BrightYellow);
            term.printAt(row, col, "  Pressed");
        }
    }
}

struct GamepadStatus {
    float leftX { 0.f };
    float leftY { 0.f };
    float rightX { 0.f };
    float rightY { 0.f };
    float leftTrigger { 0.f };
    float rightTrigger { 0.f };
    bool leftStickPress { false };
    bool rightStickPress { false };
    bool south { false };
    bool east { false };
    bool west { false };
    bool north { false };
    bool start { false };
    bool back { false };
    bool leftShoulder { false };
    bool rightShoulder { false };
    bool dPadDown { false };
    bool dPadLeft { false };
    bool dPadRight { false };
    bool dPadUp { false };
};

float analogueRound(float value)
{
    return std::round(value * 100.f) / 100.f;
}

// Consume all gamepad events since last call and populate status
// as a snapshot of current state (doing this way may miss a button
// press and subsequent release within the time window of a frame as
// the release will overwrite the press status, but it's fine for this
// test).
//
// Note! events contain gamepadId to distinguish between gamepads if
// more than one is connected at once. We ignore this here and
// simply report on all gamepad events. In real use we might want to
// ask the user to press a button to start to determine which gamepad
// is in use if more than one is detected.
void collateGamepadEvents(gamepad::Gamepad& gamepad, GamepadStatus& status)
{
    auto evts = gamepad.getEvents();
    for (const auto& e : evts) {
        if (e.eventType == gamepad::EventType::Analogue) {
            status.leftX = analogueRound(e.analogue.leftX);
            status.leftY = analogueRound(e.analogue.leftY);
            status.rightX = analogueRound(e.analogue.rightX);
            status.rightY = analogueRound(e.analogue.rightY);
            status.rightTrigger = analogueRound(e.analogue.rightTrigger);
            status.leftTrigger = analogueRound(e.analogue.leftTrigger);
        }
        if (e.eventType == gamepad::EventType::ButtonPressed) {
            switch (e.buttonType) {
                case gamepad::ButtonType::South:
                    status.south = true;
                    break;
                case gamepad::ButtonType::North:
                    status.north = true;
                    break;
                case gamepad::ButtonType::East:
                    status.east = true;
                    break;
                case gamepad::ButtonType::West:
                    status.west = true;
                    break;
                case gamepad::ButtonType::Start:
                    status.start = true;
                    break;
                case gamepad::ButtonType::Back:
                    status.back = true;
                    break;
                case gamepad::ButtonType::LeftShoulder:
                    status.leftShoulder = true;
                    break;
                case gamepad::ButtonType::RightShoulder:
                    status.rightShoulder = true;
                    break;
                case gamepad::ButtonType::DPadDown:
                    status.dPadDown = true;
                    break;
                case gamepad::ButtonType::DPadLeft:
                    status.dPadLeft = true;
                    break;
                case gamepad::ButtonType::DPadRight:
                    status.dPadRight = true;
                    break;
                case gamepad::ButtonType::DPadUp:
                    status.dPadUp = true;
                    break;
                case gamepad::ButtonType::LeftStickPress:
                    status.leftStickPress = true;
                    break;
                case gamepad::ButtonType::RightStickPress:
                    status.rightStickPress = true;
                    break;
                default:
            }
        }
        if (e.eventType == gamepad::EventType::ButtonReleased) {
            switch (e.buttonType) {
                case gamepad::ButtonType::South:
                    status.south = false;
                    break;
                case gamepad::ButtonType::North:
                    status.north = false;
                    break;
                case gamepad::ButtonType::East:
                    status.east = false;
                    break;
                case gamepad::ButtonType::West:
                    status.west = false;
                    break;
                case gamepad::ButtonType::Start:
                    status.start = false;
                    break;
                case gamepad::ButtonType::Back:
                    status.back = false;
                    break;
                case gamepad::ButtonType::LeftShoulder:
                    status.leftShoulder = false;
                    break;
                case gamepad::ButtonType::RightShoulder:
                    status.rightShoulder = false;
                    break;
                case gamepad::ButtonType::DPadDown:
                    status.dPadDown = false;
                    break;
                case gamepad::ButtonType::DPadLeft:
                    status.dPadLeft = false;
                    break;
                case gamepad::ButtonType::DPadRight:
                    status.dPadRight = false;
                    break;
                case gamepad::ButtonType::DPadUp:
                    status.dPadUp = false;
                    break;
                case gamepad::ButtonType::LeftStickPress:
                    status.leftStickPress = false;
                    break;
                case gamepad::ButtonType::RightStickPress:
                    status.rightStickPress = false;
                    break;
                default:
            }
        }
    }
}

int main()
{
    try {
        mgo::Log::init(
            "debug.log",
            mgo::Log::Level::Debug,
            false); // false = don't append (i.e. overwrite)
        gamepad::Gamepad gamepad;
        terminal::Terminal term(false);
        GamepadStatus status;
        for (;;) {
            std::size_t row = 5;
            term.cursorOff();
            // This calls gamepad.getEvents():
            collateGamepadEvents(gamepad, status);
            if (gamepad.getGamepadCount() == 0) {
                terminal::MessageBoxOptions opts;
                opts.message = "Please attach a gamepad!";
                opts.col = 1;
                opts.row = 1;
                opts.mode = terminal::OutputMode::render;
                term.messageBox(opts);
            } else {
                constexpr std::size_t col = 17;
                term.printAt(1, 1, "SDL Gamepad Tester");
                term.printAt(2, 1, "------------------");
                term.printAt(4, 1, std::format("Gamepad type   :  {}", gamepad.getGamepadType()));
                ++row;
                term.printAt(row, 1, "Left stick  X  :");
                highlight(term, row++, col, status.leftX);
                term.printAt(row, 1, "Left stick  Y  :");
                highlight(term, row++, col, status.leftY);
                term.printAt(row, 1, "Right stick X  :");
                highlight(term, row++, col, status.rightX);
                term.printAt(row, 1, "Right stick Y  :");
                highlight(term, row++, col, status.rightY);
                term.printAt(row, 1, "R stick press  :");
                highlight(term, row++, col, status.rightStickPress);
                term.printAt(row, 1, "L stick press  :");
                highlight(term, row++, col, status.leftStickPress);
                term.printAt(row, 1, "Right trigger  :");
                highlight(term, row++, col, status.rightTrigger);
                term.printAt(row, 1, "Left trigger   :");
                highlight(term, row++, col, status.leftTrigger);
                term.printAt(row, 1, "Right shoulder :");
                highlight(term, row++, col, status.rightShoulder);
                term.printAt(row, 1, "Left shoulder  :");
                highlight(term, row++, col, status.leftShoulder);
                term.printAt(row, 1, "South button   :");
                highlight(term, row++, col, status.south);
                term.printAt(row, 1, "North button   :");
                highlight(term, row++, col, status.north);
                term.printAt(row, 1, "East button    :");
                highlight(term, row++, col, status.east);
                term.printAt(row, 1, "West button    :");
                highlight(term, row++, col, status.west);
                term.printAt(row, 1, "DPad down      :");
                highlight(term, row++, col, status.dPadDown);
                term.printAt(row, 1, "DPad up        :");
                highlight(term, row++, col, status.dPadUp);
                term.printAt(row, 1, "Dpad left      :");
                highlight(term, row++, col, status.dPadLeft);
                term.printAt(row, 1, "Dpad right     :");
                highlight(term, row++, col, status.dPadRight);
                term.printAt(row, 1, "Start          :");
                highlight(term, row++, col, status.start);
                term.printAt(row, 1, "Back           :");
                highlight(term, row++, col, status.back);

                ++row; // bit of space before Esc message
            }
            term.printAt(row, 1, "Press Escape to quit");
            term.render();
            // Non-blocking key press check:
            std::optional<int> key = keyPress::getKeyPress(false);
            if (key.has_value()) {
                if (*key == keyPress::ESC) {
                    break;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    } catch (const std::exception& e) {
        std::cerr << e.what();
        return 1;
    }
    return 0;
}