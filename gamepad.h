#pragma once

// Note that we are using SDL, not SFML for gamepad input
// SFML's handling of gamepads is a bit erratic, whereas
// SDL normalises all gamepads such that right stick is always reported
// rather SFML's method of presenting axes that may or may not exist depending
// the gamepad type. All SDL code is encapsulated within this class and init
// and quit is handled via RAII.

// Note all axes are reported in the range -1.f to 1.f

#include <SDL3/SDL.h>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace gamepad {

enum class ButtonType {
    South, // A or cross
    East, // B or circle
    West, // X or square
    North, // Y or triangle
    Start,
    Back,
    LeftShoulder,
    RightShoulder,
    DPadDown,
    DPadLeft,
    DPadRight,
    DPadUp,
    Unknown,
    NotApplicable,
};

enum class ButtonAction {
    Pressed,
    Released,
    NotApplicable,
};

enum class EventType {
    ButtonPressed,
    ButtonReleased,
    Analogue,
    Connect,
    Disconnect,
    Unknown,
};

struct AnalogueStatus {
    float leftX {0.f};
    float leftY {0.f};
    float rightX {0.f};
    float rightY {0.f};
    float leftTrigger {0.f};
    float rightTrigger {0.f};
};

struct Event {
    uint32_t joystickId { 0 };
    EventType eventType { EventType::Unknown };
    ButtonType buttonType { ButtonType::NotApplicable };
    ButtonAction buttonAction { ButtonAction::NotApplicable };
    AnalogueStatus analogue;
};

class Gamepad {
public:
    Gamepad();
    ~Gamepad();
    // This should be called once per game frame. It returns a vector
    // of events that have happened since the last call.
    std::vector<Event> getEvents();
    bool isGamePadAttached();
    void rumble(uint16_t lowFreqIntensity, uint16_t highFreqIntensity, uint32_t durationMs);

private:
    std::unordered_map<SDL_JoystickID, SDL_Gamepad*> m_gamepads;
    AnalogueStatus m_analogueStatus;
    AnalogueStatus m_previousAnalogueStatus;
};

} // namespace gamepad
