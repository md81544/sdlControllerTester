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
    float sign  = input < 0.f ? -1.f : 1.f;
    float abs_x = std::abs(input);
    if (abs_x < deadzone)
        return 0.f;
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
        default:
            return ButtonType::Unknown;
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
        auto id = SDL_GetGamepadID(pad);
        m_gamepads[id] = pad;
        mgo::Log::debug(
            std::format(
                "Connected {}\nType: {}\nId: {}",
                SDL_GetGamepadName(pad),
                gamepadTypeToString(SDL_GetGamepadType(pad)),
                id));
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

[[nodiscard]] std::vector<Event> Gamepad::getEvents()
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
                    m_gamepads[id] = pad;
                    mgo::Log::debug(
                        std::format(
                            "Connected {}\nType: {}\nId: {}",
                            SDL_GetGamepadName(pad),
                            gamepadTypeToString(SDL_GetGamepadType(pad)),
                            id));
                    Event evt;
                    evt.eventType = EventType::Connect;
                    evt.joystickId = id;
                    events.push_back(evt);
                    break;
                }

            case SDL_EVENT_GAMEPAD_REMOVED:
                {
                    auto id = sdlEvent.gdevice.which;
                    mgo::Log::debug(std::format("Disconnected gamepad id {}", id));
                    Event evt;
                    evt.eventType = EventType::Disconnect;
                    evt.joystickId = id;
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
                    mgo::Log::debug(std::format("{} pressed", buttonName(btn)));
                    Event evt;
                    evt.eventType = EventType::ButtonPressed;
                    evt.joystickId = id;
                    evt.buttonType = sdlToButtonType(btn);
                    events.push_back(evt);
                    break;
                }

            case SDL_EVENT_GAMEPAD_BUTTON_UP:
                {
                    auto id = sdlEvent.gdevice.which;
                    auto btn = static_cast<SDL_GamepadButton>(sdlEvent.gbutton.button);
                    mgo::Log::debug(std::format("{} released", buttonName(btn)));
                    Event evt;
                    evt.eventType = EventType::ButtonReleased;
                    evt.joystickId = id;
                    evt.buttonType = sdlToButtonType(btn);
                    events.push_back(evt);
                    break;
                }

            case SDL_EVENT_GAMEPAD_AXIS_MOTION:
                // Note that analogue axis events are collated so the user
                // gets a snapshot of the final values of all sticks
                // at every call to getEvents(). Otherwise there would be many
                // events for each individual axis.
                // TODO we're not caring about gamepad id here, so if multiple gamepads
                // are attached they will affect the output.
                {
                    float value = joystickCurve(normaliseAxis(sdlEvent.gaxis.value));
                    switch (sdlEvent.gaxis.axis) {
                        case SDL_GAMEPAD_AXIS_LEFTX:
                            m_analogueStatus.leftX = value;
                            break;
                        case SDL_GAMEPAD_AXIS_LEFTY:
                            m_analogueStatus.leftY = -value;
                            break;
                        case SDL_GAMEPAD_AXIS_RIGHTX:
                            m_analogueStatus.rightX = value;
                            break;
                        case SDL_GAMEPAD_AXIS_RIGHTY:
                            m_analogueStatus.rightY = -value;
                            break;
                        case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
                            m_analogueStatus.rightTrigger = value;
                            break;
                        case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
                            m_analogueStatus.leftTrigger = value;
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
    if (m_analogueStatus.leftTrigger != m_previousAnalogueStatus.leftTrigger
        || m_analogueStatus.rightTrigger != m_previousAnalogueStatus.rightTrigger
        || m_analogueStatus.leftX != m_previousAnalogueStatus.leftX
        || m_analogueStatus.rightX != m_previousAnalogueStatus.rightX
        || m_analogueStatus.leftY != m_previousAnalogueStatus.leftY
        || m_analogueStatus.rightY != m_previousAnalogueStatus.rightY) {
        Event evt;
        evt.eventType = EventType::Analogue;
        evt.analogue = m_analogueStatus;
        events.push_back(evt);
        m_previousAnalogueStatus = m_analogueStatus;
    }
    return events;
}

bool Gamepad::isGamePadAttached()
{
    return !m_gamepads.empty();
}

void Gamepad::rumble(uint16_t lowFreqIntensity, uint16_t highFreqIntensity, uint32_t durationMs)
{
    if (m_gamepads.empty()) {
        return;
    }
    auto gamepad = m_gamepads.begin()->second;
    SDL_RumbleGamepad(gamepad, lowFreqIntensity, highFreqIntensity, durationMs);
}

} // namespace gamepad