#pragma once

#include <cstddef>
#include <string>
#include <variant>

namespace sprawn {

// Typed editor commands produced by InputHandler from raw SDL events.

struct MoveCursor   { int dx; int dy; bool shift{false}; };
struct MoveHome     { bool shift{false}; };
struct MoveEnd      { bool shift{false}; };
struct MovePgUp     { bool shift{false}; };
struct MovePgDn     { bool shift{false}; };
struct InsertText   { std::string text; };
struct DeleteBackward {};
struct DeleteForward  {};
struct NewLine      {};
struct ScrollLines  { float dy; };         // positive = scroll down
struct ClickPosition { int x_px; int y_px; bool shift{false}; };
struct Copy         {};
struct Paste        {};
struct Cut          {};
struct SelectAll    {};
struct Quit         {};

using EditorCommand = std::variant<
    MoveCursor, MoveHome, MoveEnd, MovePgUp, MovePgDn,
    InsertText, DeleteBackward, DeleteForward, NewLine,
    ScrollLines, ClickPosition, Copy, Paste, Cut, SelectAll, Quit
>;

} // namespace sprawn
