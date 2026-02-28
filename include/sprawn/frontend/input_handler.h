#pragma once

#include "events.h"

#include <SDL2/SDL.h>
#include <optional>

namespace sprawn {

// Stateless translator: raw SDL event → typed EditorCommand.
class InputHandler {
public:
    std::optional<EditorCommand> translate(const SDL_Event& ev) const;
};

} // namespace sprawn
