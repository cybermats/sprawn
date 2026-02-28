#include <sprawn/frontend/input_handler.h>

namespace sprawn {

std::optional<EditorCommand> InputHandler::translate(const SDL_Event& ev) const {
    switch (ev.type) {

    case SDL_TEXTINPUT:
        return InsertText{std::string(ev.text.text)};

    case SDL_KEYDOWN: {
        const auto& k = ev.key.keysym;
        const bool ctrl  = (k.mod & KMOD_CTRL)  != 0;
        const bool shift = (k.mod & KMOD_SHIFT) != 0;
        (void)shift;

        if (ctrl) {
            switch (k.sym) {
            case SDLK_c: return Copy{};
            case SDLK_v: return Paste{};
            case SDLK_x: return Cut{};
            case SDLK_q: return Quit{};
            default: break;
            }
        }

        switch (k.sym) {
        case SDLK_LEFT:      return MoveCursor{-1,  0};
        case SDLK_RIGHT:     return MoveCursor{ 1,  0};
        case SDLK_UP:        return MoveCursor{ 0, -1};
        case SDLK_DOWN:      return MoveCursor{ 0,  1};
        case SDLK_HOME:      return MoveHome{};
        case SDLK_END:       return MoveEnd{};
        case SDLK_PAGEUP:    return MovePgUp{};
        case SDLK_PAGEDOWN:  return MovePgDn{};
        case SDLK_BACKSPACE: return DeleteBackward{};
        case SDLK_DELETE:    return DeleteForward{};
        case SDLK_RETURN:    [[fallthrough]];
        case SDLK_KP_ENTER:  return NewLine{};
        default: break;
        }
        break;
    }

    case SDL_MOUSEBUTTONDOWN:
        if (ev.button.button == SDL_BUTTON_LEFT)
            return ClickPosition{ev.button.x, ev.button.y};
        break;

    case SDL_MOUSEWHEEL: {
        // SDL_MOUSEWHEEL: y > 0 means scroll up (content moves down → first_line_ decreases)
        // We define ScrollLines positive = scroll down (advance first_line_)
        float dy = -static_cast<float>(ev.wheel.y) * 3.0f;
        return ScrollLines{dy};
    }

    default:
        break;
    }

    return std::nullopt;
}

} // namespace sprawn
