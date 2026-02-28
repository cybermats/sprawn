#include <doctest/doctest.h>

#include "../src/backend/line_index.h"
#include "../src/backend/piece_table.h"

#include <stdexcept>

using namespace sprawn;

static PieceTable make_table(const char* text) {
    PieceTable pt;
    pt.insert(0, text);
    return pt;
}

TEST_CASE("LineIndex: single line no newline") {
    auto pt = make_table("Hello");
    LineIndex idx;
    idx.rebuild(pt);

    CHECK(idx.line_count() == 1);
    auto span = idx.line_span(0);
    CHECK(span.offset == 0);
    CHECK(span.length == 5);
}

TEST_CASE("LineIndex: single line with newline") {
    auto pt = make_table("Hello\n");
    LineIndex idx;
    idx.rebuild(pt);

    CHECK(idx.line_count() == 2);
    auto span0 = idx.line_span(0);
    CHECK(span0.offset == 0);
    CHECK(span0.length == 5);

    // Second line is empty
    auto span1 = idx.line_span(1);
    CHECK(span1.offset == 6);
    CHECK(span1.length == 0);
}

TEST_CASE("LineIndex: multiple lines") {
    auto pt = make_table("line1\nline2\nline3");
    LineIndex idx;
    idx.rebuild(pt);

    CHECK(idx.line_count() == 3);

    auto s0 = idx.line_span(0);
    CHECK(pt.text(s0.offset, s0.length) == "line1");

    auto s1 = idx.line_span(1);
    CHECK(pt.text(s1.offset, s1.length) == "line2");

    auto s2 = idx.line_span(2);
    CHECK(pt.text(s2.offset, s2.length) == "line3");
}

TEST_CASE("LineIndex: empty text") {
    PieceTable pt;
    LineIndex idx;
    idx.rebuild(pt);

    CHECK(idx.line_count() == 1);
    auto span = idx.line_span(0);
    CHECK(span.length == 0);
}

TEST_CASE("LineIndex: to_offset") {
    auto pt = make_table("abc\ndef\nghi");
    LineIndex idx;
    idx.rebuild(pt);

    CHECK(idx.to_offset(0, 0) == 0);
    CHECK(idx.to_offset(0, 2) == 2);
    CHECK(idx.to_offset(1, 0) == 4);
    CHECK(idx.to_offset(1, 1) == 5);
    CHECK(idx.to_offset(2, 0) == 8);
}

TEST_CASE("LineIndex: out of range throws") {
    auto pt = make_table("abc");
    LineIndex idx;
    idx.rebuild(pt);

    CHECK_THROWS_AS(idx.line_span(1), std::out_of_range);
    CHECK_THROWS_AS(idx.to_offset(1, 0), std::out_of_range);
}

TEST_CASE("LineIndex: windows line endings") {
    auto pt = make_table("abc\r\ndef\r\n");
    LineIndex idx;
    idx.rebuild(pt);

    CHECK(idx.line_count() == 3);

    auto s0 = idx.line_span(0);
    CHECK(pt.text(s0.offset, s0.length) == "abc");

    auto s1 = idx.line_span(1);
    CHECK(pt.text(s1.offset, s1.length) == "def");

    auto s2 = idx.line_span(2);
    CHECK(s2.length == 0);
}

TEST_CASE("LineIndex: classic mac line endings") {
    auto pt = make_table("abc\rdef\rghi");
    LineIndex idx;
    idx.rebuild(pt);

    CHECK(idx.line_count() == 3);

    auto s0 = idx.line_span(0);
    CHECK(pt.text(s0.offset, s0.length) == "abc");

    auto s1 = idx.line_span(1);
    CHECK(pt.text(s1.offset, s1.length) == "def");

    auto s2 = idx.line_span(2);
    CHECK(pt.text(s2.offset, s2.length) == "ghi");
}

TEST_CASE("LineIndex: col validation") {
    auto pt = make_table("abc\ndef");
    LineIndex idx;
    idx.rebuild(pt);

    CHECK(idx.to_offset(0, 3) == 3);  // col at end of line is valid
    CHECK_THROWS_AS(idx.to_offset(0, 4), std::out_of_range);  // past end
    CHECK_THROWS_AS(idx.to_offset(1, 4), std::out_of_range);  // past end
}
