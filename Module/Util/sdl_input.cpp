#include "sdl_input.h"

#ifdef ENABLESDL3
#include <SDL3/SDL.h>


#include "Module/Container/unorderedmap.h"
#include "module_log.h"

namespace pf::input::sdlinput
{
    pf::input::KeyboardState keyboard;
    pf::input::MouseState mouse;

    struct Internal_ControllerState
    {
        SDL_JoystickID internalID;
        SDL_Gamepad* controller; 
        Uint16 rumble_l, rumble_r = 0;
        pf::input::ControllerState state;
    };

    pf::vector<Internal_ControllerState> controllers;
    pf::unordered_map<SDL_JoystickID, size_t> controller_mapped;
    pf::vector<SDL_Event> events;
 
    int to_wicked(const SDL_Scancode& scan, const SDL_Keycode& sym);
    void controller_to_wicked(uint32_t* current, Uint8 button, bool pressed);
    void controller_map_rebuild();

    void Initialize() {
       
        if (SDL_AddGamepadMappingsFromFile("gamecontrollerdb.txt") < 0) {
            pf::backlogger::postin("[SDL Input] No controller config loaded...");
        }
    }

    void ProcessEvent(const SDL_Event& event) {
        events.push_back(event);
    }

    void Update()
    {
        mouse.delta_wheel = 0;
        mouse.delta_position = XMFLOAT2(0, 0);

        for (auto& event : events) {
            switch (event.type) {
               
            case SDL_EVENT_KEY_DOWN: 
            {
               
                int converted = to_wicked(event.key.scancode, event.key.key);
                if (converted >= 0) {
                    keyboard.buttons[converted] = true;
                }
                break;
            }
            case SDL_EVENT_KEY_UP:
            {
                int converted = to_wicked(event.key.scancode, event.key.key);
                if (converted >= 0) {
                    keyboard.buttons[converted] = false;
                }
                break;
            }

            
            case SDL_EVENT_MOUSE_MOTION:
                mouse.position.x = event.motion.x;
                mouse.position.y = event.motion.y;
                mouse.delta_position.x += event.motion.xrel;
                mouse.delta_position.y += event.motion.yrel;
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                switch (event.button.button) {
                case SDL_BUTTON_LEFT:   mouse.left_button_press = true; break;
                case SDL_BUTTON_RIGHT:  mouse.right_button_press = true; break;
                case SDL_BUTTON_MIDDLE: mouse.middle_button_press = true; break;
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                switch (event.button.button) {
                case SDL_BUTTON_LEFT:   mouse.left_button_press = false; break;
                case SDL_BUTTON_RIGHT:  mouse.right_button_press = false; break;
                case SDL_BUTTON_MIDDLE: mouse.middle_button_press = false; break;
                }
                break;
            case SDL_EVENT_MOUSE_WHEEL:
            {
               
                float delta = event.wheel.y;
                if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
                    delta *= -1;
                }
                mouse.delta_wheel += delta;
                break;
            }

           
            case SDL_EVENT_GAMEPAD_AXIS_MOTION:
            {
                auto controller_get = controller_mapped.find(event.gaxis.which);
                if (controller_get != controller_mapped.end()) {
                    float raw = event.gaxis.value / 32767.0f;
                    const float deadzone = 0.2f;
                    float deadzoned = (raw < -deadzone || raw > deadzone) ? raw : 0;

                    switch (event.gaxis.axis) {
                    case SDL_GAMEPAD_AXIS_LEFTX:
                        controllers[controller_get->second].state.thumbstick_L.x = deadzoned;
                        break;
                    case SDL_GAMEPAD_AXIS_LEFTY:
                        controllers[controller_get->second].state.thumbstick_L.y = -deadzoned;
                        break;
                    case SDL_GAMEPAD_AXIS_RIGHTX:
                        controllers[controller_get->second].state.thumbstick_R.x = deadzoned;
                        break;
                    case SDL_GAMEPAD_AXIS_RIGHTY:
                        controllers[controller_get->second].state.thumbstick_R.y = deadzoned;
                        break;
                    case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
                        controllers[controller_get->second].state.trigger_L = (deadzoned > 0.f) ? deadzoned : 0;
                        break;
                    case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
                        controllers[controller_get->second].state.trigger_R = (deadzoned > 0.f) ? deadzoned : 0;
                        break;
                    }
                }
                break;
            }
            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            case SDL_EVENT_GAMEPAD_BUTTON_UP:
            {
                auto find = controller_mapped.find(event.gbutton.which);
                if (find != controller_mapped.end()) {
                    controller_to_wicked(&controllers[find->second].state.buttons,
                        event.gbutton.button,
                        event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
                }
                break;
            }
            case SDL_EVENT_GAMEPAD_ADDED:
            {
                SDL_Gamepad* gp = SDL_OpenGamepad(event.gdevice.which);
                if (gp) {
                    auto& controller = controllers.emplace_back();
                    controller.controller = gp;
                    controller.internalID = event.gdevice.which;
                    controller_map_rebuild();
                }
                break;
            }
            case SDL_EVENT_GAMEPAD_REMOVED:
            {
                auto find = controller_mapped.find(event.gdevice.which);
                if (find != controller_mapped.end()) {
                    SDL_CloseGamepad(controllers[find->second].controller);
                    controllers[find->second] = std::move(controllers.back());
                    controllers.pop_back();
                    controller_map_rebuild();
                }
                break;
            }
            default: break;
            }
        }

        
        for (auto& controller : controllers) {
            // SDL3: SDL_RumbleGamepad
            SDL_RumbleGamepad(controller.controller, controller.rumble_l, controller.rumble_r, 60);
        }
        events.clear();
    }
    
    int GetMaxControllerCount() { return controllers.size(); }

    bool GetControllerState(pf::input::ControllerState* state, int index) {
        if (index < controllers.size()) {
            if (state != nullptr)
            {
                *state = controllers[index].state;
            }
            return true;
        }
        return false;
    }

    void GetKeyboardState(pf::input::KeyboardState* state) {
        *state = keyboard;
    }
    void GetMouseState(pf::input::MouseState* state) {
        *state = mouse;
    }


    int to_wicked(const SDL_Scancode& scan, const SDL_Keycode& sym) {
        if (sym >= SDLK_A && sym <= SDLK_Z) {
            return (sym - SDLK_A) + CHARACTER_RANGE_START;
        }
       
        if (sym >= SDLK_0 && sym <= SDLK_9) { // 1 to 9
            return (sym - SDLK_0) + DIGIT_RANGE_START;
        }
        if (scan >= 58 && scan <= 69) { // F1 to F12
            return (scan - 58) + KEYBOARD_BUTTON_F1;
        }
        if (scan >= 79 && scan <= 82) { // Keyboard directional buttons
            return (82 - scan) + KEYBOARD_BUTTON_UP;
        }
        switch (scan) { // Individual scancode key conversion
        case SDL_SCANCODE_SPACE:
            return pf::input::KEYBOARD_BUTTON_SPACE;
        case SDL_SCANCODE_LSHIFT:
            return pf::input::KEYBOARD_BUTTON_LSHIFT;
        case SDL_SCANCODE_RSHIFT:
            return pf::input::KEYBOARD_BUTTON_RSHIFT;
        case SDL_SCANCODE_RETURN:
            return pf::input::KEYBOARD_BUTTON_ENTER;
        case SDL_SCANCODE_ESCAPE:
            return pf::input::KEYBOARD_BUTTON_ESCAPE;
        case SDL_SCANCODE_HOME:
            return pf::input::KEYBOARD_BUTTON_HOME;
        case SDL_SCANCODE_RCTRL:
            return pf::input::KEYBOARD_BUTTON_RCONTROL;
        case SDL_SCANCODE_LCTRL:
            return pf::input::KEYBOARD_BUTTON_LCONTROL;
        case SDL_SCANCODE_DELETE:
            return pf::input::KEYBOARD_BUTTON_DELETE;
        case SDL_SCANCODE_BACKSPACE:
            return pf::input::KEYBOARD_BUTTON_BACKSPACE;
        case SDL_SCANCODE_PAGEDOWN:
            return pf::input::KEYBOARD_BUTTON_PAGEDOWN;
        case SDL_SCANCODE_PAGEUP:
            return pf::input::KEYBOARD_BUTTON_PAGEUP;
        case SDL_SCANCODE_KP_0:
            return pf::input::KEYBOARD_BUTTON_NUMPAD0;
        case SDL_SCANCODE_KP_1:
            return pf::input::KEYBOARD_BUTTON_NUMPAD1;
        case SDL_SCANCODE_KP_2:
            return pf::input::KEYBOARD_BUTTON_NUMPAD2;
        case SDL_SCANCODE_KP_3:
            return pf::input::KEYBOARD_BUTTON_NUMPAD3;
        case SDL_SCANCODE_KP_4:
            return pf::input::KEYBOARD_BUTTON_NUMPAD4;
        case SDL_SCANCODE_KP_5:
            return pf::input::KEYBOARD_BUTTON_NUMPAD5;
        case SDL_SCANCODE_KP_6:
            return pf::input::KEYBOARD_BUTTON_NUMPAD6;
        case SDL_SCANCODE_KP_7:
            return pf::input::KEYBOARD_BUTTON_NUMPAD7;
        case SDL_SCANCODE_KP_8:
            return pf::input::KEYBOARD_BUTTON_NUMPAD8;
        case SDL_SCANCODE_KP_9:
            return pf::input::KEYBOARD_BUTTON_NUMPAD9;
        case SDL_SCANCODE_KP_MULTIPLY:
            return pf::input::KEYBOARD_BUTTON_MULTIPLY;
        case SDL_SCANCODE_KP_PLUS:
            return pf::input::KEYBOARD_BUTTON_ADD;
        case SDL_SCANCODE_SEPARATOR:
            return pf::input::KEYBOARD_BUTTON_SEPARATOR;
        case SDL_SCANCODE_KP_MINUS:
            return pf::input::KEYBOARD_BUTTON_SUBTRACT;
        case SDL_SCANCODE_KP_DECIMAL:
            return pf::input::KEYBOARD_BUTTON_DECIMAL;
        case SDL_SCANCODE_KP_DIVIDE:
            return pf::input::KEYBOARD_BUTTON_DIVIDE;
        case SDL_SCANCODE_INSERT:
            return pf::input::KEYBOARD_BUTTON_INSERT;
        case SDL_SCANCODE_TAB:
            return pf::input::KEYBOARD_BUTTON_TAB;
        case SDL_SCANCODE_GRAVE:
            return pf::input::KEYBOARD_BUTTON_TILDE;
        case SDL_SCANCODE_LALT:
            return pf::input::KEYBOARD_BUTTON_ALT;
        case SDL_SCANCODE_RALT:
            return pf::input::KEYBOARD_BUTTON_ALTGR;
        default:
            break;
        }


        // Keycode Conversion Segment

        if (sym >= 91 && sym <= 126) {
            return sym;
        }

        return -1;
    }

    void controller_to_wicked(uint32_t* current, Uint8 button, bool pressed) {
        uint32_t btnenum;
        switch (button) {
        case SDL_GAMEPAD_BUTTON_DPAD_UP:    btnenum = pf::input::GAMEPAD_BUTTON_UP; break;
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:  btnenum = pf::input::GAMEPAD_BUTTON_LEFT; break;
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:  btnenum = pf::input::GAMEPAD_BUTTON_DOWN; break;
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: btnenum = pf::input::GAMEPAD_BUTTON_RIGHT; break;
        case SDL_GAMEPAD_BUTTON_WEST:       btnenum = pf::input::GAMEPAD_BUTTON_1; break; // SDL3 使用方位名词
        case SDL_GAMEPAD_BUTTON_SOUTH:      btnenum = pf::input::GAMEPAD_BUTTON_2; break;
        case SDL_GAMEPAD_BUTTON_EAST:       btnenum = pf::input::GAMEPAD_BUTTON_3; break;
        case SDL_GAMEPAD_BUTTON_NORTH:      btnenum = pf::input::GAMEPAD_BUTTON_4; break;
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:  btnenum = pf::input::GAMEPAD_BUTTON_5; break;
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: btnenum = pf::input::GAMEPAD_BUTTON_6; break;
        case SDL_GAMEPAD_BUTTON_LEFT_STICK:     btnenum = pf::input::GAMEPAD_BUTTON_7; break;
        case SDL_GAMEPAD_BUTTON_RIGHT_STICK:    btnenum = pf::input::GAMEPAD_BUTTON_8; break;
        case SDL_GAMEPAD_BUTTON_BACK:       btnenum = pf::input::GAMEPAD_BUTTON_9; break;
        case SDL_GAMEPAD_BUTTON_START:      btnenum = pf::input::GAMEPAD_BUTTON_10; break;
        default: assert(0); return;
        }

         btnenum = 1 << (btnenum - pf::input::GAMEPAD_RANGE_START - 1);
        if (pressed) { *current |= btnenum; }
        else { *current &= ~btnenum; }
    }

    void SetControllerFeedback(const pf::input::ControllerFeedback& data, int index) {
        if (index < controllers.size()) {
            
            SDL_SetGamepadLED(controllers[index].controller,
                data.led_color.getR(),
                data.led_color.getG(),
                data.led_color.getB());

            controllers[index].rumble_l = (Uint16)(data.vibration_left * 0xFFFF);
            controllers[index].rumble_r = (Uint16)(data.vibration_right * 0xFFFF);
        }
    }
    void controller_map_rebuild() {
        
        controller_mapped.clear();

        for (size_t index = 0; index < controllers.size(); ++index) {
         
            controller_mapped[controllers[index].internalID] = index;
        }
    }
}

#else
namespace pf::input::sdlinput
{
    void Initialize() {}
    void Update() {}
    void GetKeyboardState(pf::input::KeyboardState* state) {}
    void GetMouseState(pf::input::MouseState* state) {}
    int GetMaxControllerCount() { return 0; }
    bool GetControllerState(pf::input::ControllerState* state, int index) { return false; }
    void SetControllerFeedback(const pf::input::ControllerFeedback& data, int index) {}
}
#endif // _WIN32
