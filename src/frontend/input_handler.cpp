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

        if (ctrl) {
            switch (k.sym) {
            case SDLK_a: return SelectAll{};
            case SDLK_c: return Copy{};
            case SDLK_v: return Paste{};
            case SDLK_x: return Cut{};
            case SDLK_q: return Quit{};
            default: break;
            }
        }

        switch (k.sym) {
        case SDLK_LEFT:      return MoveCursor{-1,  0, shift};
        case SDLK_RIGHT:     return MoveCursor{ 1,  0, shift};
        case SDLK_UP:        return MoveCursor{ 0, -1, shift};
        case SDLK_DOWN:      return MoveCursor{ 0,  1, shift};
        case SDLK_HOME:      return MoveHome{shift};
        case SDLK_END:       return MoveEnd{shift};
        case SDLK_PAGEUP:    return MovePgUp{shift};
        case SDLK_PAGEDOWN:  return MovePgDn{shift};
        case SDLK_BACKSPACE: return DeleteBackward{};
        case SDLK_DELETE:    return DeleteForward{};
        case SDLK_RETURN:    [[fallthrough]];
        case SDLK_KP_ENTER:  return NewLine{};
        default: break;
        }
        break;
    }

    case SDL_MOUSEBUTTONDOWN:
        if (ev.button.button == SDL_BUTTON_LEFT) {
            bool shift = (SDL_GetModState() & KMOD_SHIFT) != 0;
            return ClickPosition{ev.button.x, ev.button.y, shift};
        }
        break;

    case SDL_MOUSEMOTION:
        // Drag-to-select: extend selection while left button held
        if (ev.motion.state & SDL_BUTTON_LMASK)
            return ClickPosition{ev.motion.x, ev.motion.y, true};
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
