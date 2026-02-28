#pragma once

#include <string_view>

namespace sprawn {

// High-level entry point: creates the window, font, atlas, and editor,
// runs the event loop, and returns when the user quits.
// filepath may be empty (start with an empty document).
// Returns false if initialisation fails.
bool run_application(std::string_view filepath);

} // namespace sprawn
