#include "gamepad.h"
#include "log.h"
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_joystick.h>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <stdexcept>

namespace gamepad {

namespace {

float normaliseAxis(int16_t value)
{
    if (value >= 0) {
        return value / 32767.0f;
    }

    return value / 32768.0f;
}

// Allows for fine control near the middle of the stick's input
// and much larger input when pushed further
float joystickCurve(float input, float curve = 6.f, float deadzone = 0.05f)
{
    input = std::clamp(input, -1.f, 1.f);
    float sign = input < 0.f ? -1.f : 1.f;
    float abs_x = std::abs(input);
    if (abs_x < deadzone) {
        return 0.f;
    }
    float scaled = (abs_x - deadzone) / (1.f - deadzone); // [0, 1]
    float result = (std::exp(curve * scaled) - 1.f) / (std::exp(curve) - 1.f);
    return std::clamp(sign * result, -1.f, 1.f);
}

[[maybe_unused]]
const char* gamepadTypeToString(SDL_GamepadType type)
{
    switch (type) {
        case SDL_GAMEPAD_TYPE_STANDARD:
            return "Standard";
        case SDL_GAMEPAD_TYPE_XBOX360:
            return "Xbox 360";
        case SDL_GAMEPAD_TYPE_XBOXONE:
            return "Xbox One";
        case SDL_GAMEPAD_TYPE_PS3:
            return "PlayStation 3";
        case SDL_GAMEPAD_TYPE_PS4:
            return "PlayStation 4";
        case SDL_GAMEPAD_TYPE_PS5:
            return "PlayStation 5";
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO:
            return "Nintendo Switch Pro";
        default:
            return "Unknown";
    }
}

const char* buttonName(SDL_GamepadButton button)
{
    switch (button) {
        case SDL_GAMEPAD_BUTTON_SOUTH:
            return "South (A / Cross)";
        case SDL_GAMEPAD_BUTTON_EAST:
            return "East (B / Circle)";
        case SDL_GAMEPAD_BUTTON_WEST:
            return "West (X / Square)";
        case SDL_GAMEPAD_BUTTON_NORTH:
            return "North (Y / Triangle)";
        case SDL_GAMEPAD_BUTTON_START:
            return "Start";
        case SDL_GAMEPAD_BUTTON_BACK:
            return "Back";
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
            return "Left Shoulder";
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
            return "Right Shoulder";
        case SDL_GAMEPAD_BUTTON_LEFT_STICK:
            return "Left Stick";
        case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
            return "Right Stick";
        case SDL_GAMEPAD_BUTTON_DPAD_UP:
            return "D-pad Up";
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
            return "D-pad Down";
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
            return "D-pad Left";
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
            return "D-pad Right";
        case SDL_GAMEPAD_BUTTON_GUIDE:
            return "Guide";
        case SDL_GAMEPAD_BUTTON_MISC1:
            return "Misc 1";
        case SDL_GAMEPAD_BUTTON_MISC2:
            return "Misc 2";
        case SDL_GAMEPAD_BUTTON_MISC3:
            return "Misc 3";
        case SDL_GAMEPAD_BUTTON_MISC4:
            return "Misc 4";
        case SDL_GAMEPAD_BUTTON_MISC5:
            return "Misc 5";
        case SDL_GAMEPAD_BUTTON_MISC6:
            return "Misc 6";
        case SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1:
            return "Right paddle 1";
        case SDL_GAMEPAD_BUTTON_LEFT_PADDLE1:
            return "Left paddle 1";
        case SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2:
            return "Right paddle 2";
        case SDL_GAMEPAD_BUTTON_LEFT_PADDLE2:
            return "Left paddle 2";
        case SDL_GAMEPAD_BUTTON_TOUCHPAD:
            return "Touchpad";
        default:
            return "Other";
    }
}

ButtonType sdlToButtonType(SDL_GamepadButton button)
{

    switch (button) {
        case SDL_GAMEPAD_BUTTON_SOUTH:
            return ButtonType::South;
        case SDL_GAMEPAD_BUTTON_EAST:
            return ButtonType::East;
        case SDL_GAMEPAD_BUTTON_WEST:
            return ButtonType::West;
        case SDL_GAMEPAD_BUTTON_NORTH:
            return ButtonType::North;
        case SDL_GAMEPAD_BUTTON_START:
            return ButtonType::Start;
        case SDL_GAMEPAD_BUTTON_BACK:
            return ButtonType::Back;
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
            return ButtonType::LeftShoulder;
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
            return ButtonType::RightShoulder;
        case SDL_GAMEPAD_BUTTON_DPAD_UP:
            return ButtonType::DPadUp;
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
            return ButtonType::DPadDown;
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
            return ButtonType::DPadLeft;
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
            return ButtonType::DPadRight;
        case SDL_GAMEPAD_BUTTON_LEFT_STICK:
            return ButtonType::LeftStickPress;
        case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
            return ButtonType::RightStickPress;
        case SDL_GAMEPAD_BUTTON_GUIDE:
            return ButtonType::Guide;
        case SDL_GAMEPAD_BUTTON_MISC1:
            return ButtonType::Misc1;
        case SDL_GAMEPAD_BUTTON_MISC2:
            return ButtonType::Misc2;
        case SDL_GAMEPAD_BUTTON_MISC3:
            return ButtonType::Misc3;
        case SDL_GAMEPAD_BUTTON_MISC4:
            return ButtonType::Misc4;
        case SDL_GAMEPAD_BUTTON_MISC5:
            return ButtonType::Misc5;
        case SDL_GAMEPAD_BUTTON_MISC6:
            return ButtonType::Misc6;
        case SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1:
            return ButtonType::RightPaddle1;
        case SDL_GAMEPAD_BUTTON_LEFT_PADDLE1:
            return ButtonType::LeftPaddle1;
        case SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2:
            return ButtonType::RightPaddle2;
        case SDL_GAMEPAD_BUTTON_LEFT_PADDLE2:
            return ButtonType::LeftPaddle2;
        case SDL_GAMEPAD_BUTTON_TOUCHPAD:
            return ButtonType::Touchpad;
        default:
            return ButtonType::Unknown;
    }
}

SDL_GamepadButton buttonTypeToSdl(ButtonType button)
{

    switch (button) {
        case ButtonType::South:
            return SDL_GAMEPAD_BUTTON_SOUTH;
        case ButtonType::East:
            return SDL_GAMEPAD_BUTTON_EAST;
        case ButtonType::West:
            return SDL_GAMEPAD_BUTTON_WEST;
        case ButtonType::North:
            return SDL_GAMEPAD_BUTTON_NORTH;
        case ButtonType::Start:
            return SDL_GAMEPAD_BUTTON_START;
        case ButtonType::Back:
            return SDL_GAMEPAD_BUTTON_BACK;
        case ButtonType::LeftShoulder:
            return SDL_GAMEPAD_BUTTON_LEFT_SHOULDER;
        case ButtonType::RightShoulder:
            return SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER;
        case ButtonType::DPadUp:
            return SDL_GAMEPAD_BUTTON_DPAD_UP;
        case ButtonType::DPadDown:
            return SDL_GAMEPAD_BUTTON_DPAD_DOWN;
        case ButtonType::DPadLeft:
            return SDL_GAMEPAD_BUTTON_DPAD_LEFT;
        case ButtonType::DPadRight:
            return SDL_GAMEPAD_BUTTON_DPAD_RIGHT;
        case ButtonType::LeftStickPress:
            return SDL_GAMEPAD_BUTTON_LEFT_STICK;
        case ButtonType::RightStickPress:
            return SDL_GAMEPAD_BUTTON_RIGHT_STICK;
        case ButtonType::Guide:
            return SDL_GAMEPAD_BUTTON_GUIDE;
        case ButtonType::Misc1:
            return SDL_GAMEPAD_BUTTON_MISC1;
        case ButtonType::Misc2:
            return SDL_GAMEPAD_BUTTON_MISC2;
        case ButtonType::Misc3:
            return SDL_GAMEPAD_BUTTON_MISC3;
        case ButtonType::Misc4:
            return SDL_GAMEPAD_BUTTON_MISC4;
        case ButtonType::Misc5:
            return SDL_GAMEPAD_BUTTON_MISC5;
        case ButtonType::Misc6:
            return SDL_GAMEPAD_BUTTON_MISC6;
        case ButtonType::RightPaddle1:
            return SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1;
        case ButtonType::LeftPaddle1:
            return SDL_GAMEPAD_BUTTON_LEFT_PADDLE1;
        case ButtonType::RightPaddle2:
            return SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2;
        case ButtonType::LeftPaddle2:
            return SDL_GAMEPAD_BUTTON_LEFT_PADDLE2;
        case ButtonType::Touchpad:
            return SDL_GAMEPAD_BUTTON_TOUCHPAD;
        default:
            return SDL_GAMEPAD_BUTTON_INVALID;
    }
}

} // anonymous namespace

Gamepad::Gamepad()
{
    if (!SDL_Init(SDL_INIT_GAMEPAD)) {
        throw std::runtime_error("Could not initialise SDL for joystick");
    }
    int count = 0;
    // Any gamepads already connected?
    SDL_JoystickID* ids = SDL_GetGamepads(&count);
    for (int n = 0; n < count; ++n) {
        SDL_Gamepad* pad = SDL_OpenGamepad(ids[n]);
        if (!pad) {
            continue;
        }
        unsigned id = SDL_GetGamepadID(pad);
        m_gamepads[id] = pad;
        logConnection(pad, id);
    }
    SDL_free(ids);
}

Gamepad::~Gamepad()
{
    for (auto& [id, pad] : m_gamepads) {
        SDL_CloseGamepad(pad);
    }
    SDL_Quit();
}

std::vector<Event> Gamepad::getEvents()
{
    std::vector<Event> events;
    SDL_Event sdlEvent;
    while (SDL_PollEvent(&sdlEvent)) {
        switch (sdlEvent.type) {
            case SDL_EVENT_GAMEPAD_ADDED:
                {
                    SDL_Gamepad* pad = SDL_OpenGamepad(sdlEvent.gdevice.which);
                    if (!pad) {
                        break;
                    }
                    auto id = SDL_GetGamepadID(pad);
                    mgo::Log::debug("Gamepad id {} connected", id);
                    m_gamepads[id] = pad;
                    logConnection(pad, id);
                    Event evt;
                    evt.eventType = EventType::Connect;
                    evt.gamepadId = id;
                    events.push_back(evt);
                    break;
                }

            case SDL_EVENT_GAMEPAD_REMOVED:
                {
                    auto id = sdlEvent.gdevice.which;
                    mgo::Log::debug("Disconnected gamepad id {}", id);
                    Event evt;
                    evt.eventType = EventType::Disconnect;
                    evt.gamepadId = id;
                    events.push_back(evt);
                    auto it = m_gamepads.find(id);
                    if (it != m_gamepads.end()) {
                        SDL_CloseGamepad(it->second);
                        m_gamepads.erase(it);
                    }
                    break;
                }

            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
                {
                    auto id = sdlEvent.gdevice.which;
                    auto btn = static_cast<SDL_GamepadButton>(sdlEvent.gbutton.button);
                    mgo::Log::debug("{} pressed", buttonName(btn));
                    Event evt;
                    evt.eventType = EventType::ButtonPressed;
                    evt.gamepadId = id;
                    evt.buttonType = sdlToButtonType(btn);
                    events.push_back(evt);
                    break;
                }

            case SDL_EVENT_GAMEPAD_BUTTON_UP:
                {
                    auto id = sdlEvent.gdevice.which;
                    auto btn = static_cast<SDL_GamepadButton>(sdlEvent.gbutton.button);
                    mgo::Log::debug("{} released", buttonName(btn));
                    Event evt;
                    evt.eventType = EventType::ButtonReleased;
                    evt.gamepadId = id;
                    evt.buttonType = sdlToButtonType(btn);
                    events.push_back(evt);
                    break;
                }

            case SDL_EVENT_GAMEPAD_AXIS_MOTION:
                // Note that analogue axis events are collated so the user
                // gets a snapshot of the final values of all sticks
                // at every call to getEvents(). Otherwise there would be many
                // events for each individual axis.
                {
                    auto id = sdlEvent.gdevice.which;
                    if (!m_previousAnalogueStatus.contains(id)) {
                        m_previousAnalogueStatus[id] = AnalogueStatus {};
                    }
                    float value = joystickCurve(normaliseAxis(sdlEvent.gaxis.value));
                    switch (sdlEvent.gaxis.axis) {
                        case SDL_GAMEPAD_AXIS_LEFTX:
                            m_analogueStatus[id].leftX = value;
                            break;
                        case SDL_GAMEPAD_AXIS_LEFTY:
                            m_analogueStatus[id].leftY = -value;
                            break;
                        case SDL_GAMEPAD_AXIS_RIGHTX:
                            m_analogueStatus[id].rightX = value;
                            break;
                        case SDL_GAMEPAD_AXIS_RIGHTY:
                            m_analogueStatus[id].rightY = -value;
                            break;
                        case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
                            m_analogueStatus[id].rightTrigger = value;
                            break;
                        case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
                            m_analogueStatus[id].leftTrigger = value;
                            break;
                        default:
                            // do nothing
                            break;
                    }
                }
            default:
                // do nothing
                break;
        }
    }
    for (const auto& as : m_analogueStatus) {
        if (as.second.leftTrigger != m_previousAnalogueStatus[as.first].leftTrigger
            || as.second.rightTrigger != m_previousAnalogueStatus[as.first].rightTrigger
            || as.second.leftX != m_previousAnalogueStatus[as.first].leftX
            || as.second.rightX != m_previousAnalogueStatus[as.first].rightX
            || as.second.leftY != m_previousAnalogueStatus[as.first].leftY
            || as.second.rightY != m_previousAnalogueStatus[as.first].rightY) {
            Event evt;
            evt.gamepadId = as.first;
            evt.eventType = EventType::Analogue;
            evt.analogue = as.second;
            events.push_back(evt);
            m_previousAnalogueStatus[as.first] = m_analogueStatus[as.first];
        }
    }
    return events;
}

void Gamepad::rumble(
    uint32_t gamepadId,
    uint16_t lowFreqIntensity,
    uint16_t highFreqIntensity,
    uint32_t durationMs)
{
    if (m_gamepads.empty() || m_gamepads.find(gamepadId) == m_gamepads.end()) {
        return;
    }
    auto gamepad = m_gamepads[gamepadId];
    SDL_RumbleGamepad(gamepad, lowFreqIntensity, highFreqIntensity, durationMs);
}

std::string Gamepad::getGamepadType(uint32_t gamepadId)
{
    if (m_gamepads.empty() || m_gamepads.find(gamepadId) == m_gamepads.end()) {
        return {};
    }
    auto gamepad = m_gamepads[gamepadId];
    return gamepadTypeToString(SDL_GetGamepadType(gamepad));
}

void Gamepad::logConnection(SDL_Gamepad* pad, unsigned id)
{
    mgo::Log::debug(
        std::format(
            "Connected {}\nType: {}\nId: {}",
            SDL_GetGamepadName(pad),
            gamepadTypeToString(SDL_GetGamepadType(pad)),
            id));

    SDL_GUID guid = SDL_GetGamepadGUIDForID(id);
    char guid_str[33]; // GUIDs are 16 bytes -> 32 hex chars + null terminator
    SDL_GUIDToString(guid, guid_str, sizeof(guid_str));
    char* mapping = SDL_GetGamepadMappingForID(id);
    if (mapping) {
        mgo::Log::debug("Guid: {}, mapping: {}", guid_str, mapping);
        SDL_free(mapping); // mapping string is heap-allocated, we own it
    }
}

std::size_t Gamepad::getGamepadCount()
{
    return m_gamepads.size();
}

std::vector<uint32_t> Gamepad::getGamePadIds()
{
    std::vector<uint32_t> rc;
    for (const auto& pr : m_gamepads) {
        rc.push_back(pr.first);
    }
    return rc;
}

std::string Gamepad::buttonTypeToString(ButtonType btn)
{
    auto sdlBtn = buttonTypeToSdl(btn);
    return buttonName(sdlBtn);
}

} // namespace gamepad
