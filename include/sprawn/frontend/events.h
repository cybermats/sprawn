#pragma once

#include <cstddef>
#include <string>
#include <variant>

namespace sprawn {

// Typed editor commands produced by InputHandler from raw SDL events.

struct MoveCursor   { int dx; int dy; };  // lines (+down) / columns (+right)
struct MoveHome     {};
struct MoveEnd      {};
struct MovePgUp     {};
struct MovePgDn     {};
struct InsertText   { std::string text; };
struct DeleteBackward {};
struct DeleteForward  {};
struct NewLine      {};
struct ScrollLines  { float dy; };         // positive = scroll down
struct ClickPosition { int x_px; int y_px; };
struct Copy         {};
struct Paste        {};
struct Cut          {};
struct Quit         {};

using EditorCommand = std::variant<
    MoveCursor, MoveHome, MoveEnd, MovePgUp, MovePgDn,
    InsertText, DeleteBackward, DeleteForward, NewLine,
    ScrollLines, ClickPosition, Copy, Paste, Cut, Quit
>;

} // namespace sprawn
