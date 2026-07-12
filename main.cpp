#include "gamepad.h"
#include "keypress.h"
#include "log.h"
#include "terminal.h"
#include <chrono>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <format>
#include <iostream>
#include <thread>
#include <unordered_map>

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
    std::string gamepadType;
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
void collateGamepadEvents(
    gamepad::Gamepad& gamepad,
    std::unordered_map<uint32_t, GamepadStatus>& status)
{
    auto evts = gamepad.getEvents();
    for (const auto& e : evts) {
        if (e.eventType == gamepad::EventType::Disconnect) {
            auto it = status.find(e.gamepadId);
            if(it != status.end()){
                status.erase(it);
            }
            continue;
        }
        status[e.gamepadId].gamepadType = gamepad.getGamepadType(e.gamepadId);
        if (e.eventType == gamepad::EventType::Analogue) {
            status[e.gamepadId].leftX = analogueRound(e.analogue.leftX);
            status[e.gamepadId].leftY = analogueRound(e.analogue.leftY);
            status[e.gamepadId].rightX = analogueRound(e.analogue.rightX);
            status[e.gamepadId].rightY = analogueRound(e.analogue.rightY);
            status[e.gamepadId].rightTrigger = analogueRound(e.analogue.rightTrigger);
            status[e.gamepadId].leftTrigger = analogueRound(e.analogue.leftTrigger);
        }
        if (e.eventType == gamepad::EventType::ButtonPressed) {
            switch (e.buttonType) {
                case gamepad::ButtonType::South:
                    status[e.gamepadId].south = true;
                    break;
                case gamepad::ButtonType::North:
                    status[e.gamepadId].north = true;
                    break;
                case gamepad::ButtonType::East:
                    status[e.gamepadId].east = true;
                    break;
                case gamepad::ButtonType::West:
                    status[e.gamepadId].west = true;
                    break;
                case gamepad::ButtonType::Start:
                    status[e.gamepadId].start = true;
                    break;
                case gamepad::ButtonType::Back:
                    status[e.gamepadId].back = true;
                    break;
                case gamepad::ButtonType::LeftShoulder:
                    status[e.gamepadId].leftShoulder = true;
                    break;
                case gamepad::ButtonType::RightShoulder:
                    status[e.gamepadId].rightShoulder = true;
                    break;
                case gamepad::ButtonType::DPadDown:
                    status[e.gamepadId].dPadDown = true;
                    break;
                case gamepad::ButtonType::DPadLeft:
                    status[e.gamepadId].dPadLeft = true;
                    break;
                case gamepad::ButtonType::DPadRight:
                    status[e.gamepadId].dPadRight = true;
                    break;
                case gamepad::ButtonType::DPadUp:
                    status[e.gamepadId].dPadUp = true;
                    break;
                case gamepad::ButtonType::LeftStickPress:
                    status[e.gamepadId].leftStickPress = true;
                    break;
                case gamepad::ButtonType::RightStickPress:
                    status[e.gamepadId].rightStickPress = true;
                    break;
                default:
            }
        }
        if (e.eventType == gamepad::EventType::ButtonReleased) {
            switch (e.buttonType) {
                case gamepad::ButtonType::South:
                    status[e.gamepadId].south = false;
                    break;
                case gamepad::ButtonType::North:
                    status[e.gamepadId].north = false;
                    break;
                case gamepad::ButtonType::East:
                    status[e.gamepadId].east = false;
                    break;
                case gamepad::ButtonType::West:
                    status[e.gamepadId].west = false;
                    break;
                case gamepad::ButtonType::Start:
                    status[e.gamepadId].start = false;
                    break;
                case gamepad::ButtonType::Back:
                    status[e.gamepadId].back = false;
                    break;
                case gamepad::ButtonType::LeftShoulder:
                    status[e.gamepadId].leftShoulder = false;
                    break;
                case gamepad::ButtonType::RightShoulder:
                    status[e.gamepadId].rightShoulder = false;
                    break;
                case gamepad::ButtonType::DPadDown:
                    status[e.gamepadId].dPadDown = false;
                    break;
                case gamepad::ButtonType::DPadLeft:
                    status[e.gamepadId].dPadLeft = false;
                    break;
                case gamepad::ButtonType::DPadRight:
                    status[e.gamepadId].dPadRight = false;
                    break;
                case gamepad::ButtonType::DPadUp:
                    status[e.gamepadId].dPadUp = false;
                    break;
                case gamepad::ButtonType::LeftStickPress:
                    status[e.gamepadId].leftStickPress = false;
                    break;
                case gamepad::ButtonType::RightStickPress:
                    status[e.gamepadId].rightStickPress = false;
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
        std::unordered_map<uint32_t, GamepadStatus> status;
        constexpr std::size_t statusStartRow = 4;
        std::size_t row = statusStartRow;
        for (;;) {
            term.cursorOff();
            // This calls gamepad.getEvents()
            // getEvents() should be called each iteration to clear out
            // any unwanted events, and watch for new connections.
            collateGamepadEvents(gamepad, status);
            // Get a list of gamepads attached:
            auto vecIds = gamepad.getGamePadIds();
            if (vecIds.empty()) {
                terminal::MessageBoxOptions opts;
                opts.message = "Please attach a gamepad!";
                opts.col = 1;
                opts.row = 1;
                opts.mode = terminal::OutputMode::render;
                term.messageBox(opts);
            } else {
                term.printAt(1, 1, "SDL Gamepad Tester");
                term.printAt(2, 1, "------------------");
                row = statusStartRow;
                term.printAt(row++, 1, "Gamepad type   :");
                term.printAt(row++, 1, "Gamepad Id     :");
                term.printAt(row++, 1, "Left stick  X  :");
                term.printAt(row++, 1, "Left stick  Y  :");
                term.printAt(row++, 1, "Right stick X  :");
                term.printAt(row++, 1, "Right stick Y  :");
                term.printAt(row++, 1, "R stick press  :");
                term.printAt(row++, 1, "L stick press  :");
                term.printAt(row++, 1, "Right trigger  :");
                term.printAt(row++, 1, "Left trigger   :");
                term.printAt(row++, 1, "Right shoulder :");
                term.printAt(row++, 1, "Left shoulder  :");
                term.printAt(row++, 1, "South button   :");
                term.printAt(row++, 1, "North button   :");
                term.printAt(row++, 1, "East button    :");
                term.printAt(row++, 1, "West button    :");
                term.printAt(row++, 1, "DPad down      :");
                term.printAt(row++, 1, "DPad up        :");
                term.printAt(row++, 1, "Dpad left      :");
                term.printAt(row++, 1, "Dpad right     :");
                term.printAt(row++, 1, "Start          :");
                term.printAt(row++, 1, "Back           :");

                // Now iterate over all gamepads' statuses:
                std::size_t col = 17;
                for (const auto& s : status) {
                    row = statusStartRow;
                    term.printAt(row++, col + 2, std::format("{:.19}", s.second.gamepadType));
                    term.printAt(row++, col + 2, std::format("{}", s.first));
                    highlight(term, row++, col, s.second.leftX);
                    highlight(term, row++, col, s.second.leftY);
                    highlight(term, row++, col, s.second.rightX);
                    highlight(term, row++, col, s.second.rightY);
                    highlight(term, row++, col, s.second.rightStickPress);
                    highlight(term, row++, col, s.second.leftStickPress);
                    highlight(term, row++, col, s.second.rightTrigger);
                    highlight(term, row++, col, s.second.leftTrigger);
                    highlight(term, row++, col, s.second.rightShoulder);
                    highlight(term, row++, col, s.second.leftShoulder);
                    highlight(term, row++, col, s.second.south);
                    highlight(term, row++, col, s.second.north);
                    highlight(term, row++, col, s.second.east);
                    highlight(term, row++, col, s.second.west);
                    highlight(term, row++, col, s.second.dPadDown);
                    highlight(term, row++, col, s.second.dPadUp);
                    highlight(term, row++, col, s.second.dPadLeft);
                    highlight(term, row++, col, s.second.dPadRight);
                    highlight(term, row++, col, s.second.start);
                    highlight(term, row++, col, s.second.back);
                    col += 20;
                }

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