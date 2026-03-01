#include <doctest/doctest.h>
#include <sprawn/frontend/decoration_compositor.h>

using namespace sprawn;

TEST_CASE("flatten: empty input gives single default span") {
    LineDecoration deco;
    auto result = DecorationCompositor::flatten(deco, 10);
    REQUIRE(result.size() == 1);
    CHECK(result[0].byte_start == 0);
    CHECK(result[0].byte_end == 10);
    CHECK(result[0].style.fg.r == 220);
    CHECK(result[0].style.fg.g == 220);
    CHECK(result[0].style.fg.b == 220);
    CHECK(result[0].style.fg.a == 255);
    CHECK(result[0].style.bg.a == 0);
    CHECK(result[0].style.underline == false);
}

TEST_CASE("flatten: zero line_byte_len gives empty result") {
    LineDecoration deco;
    deco.spans.push_back({0, 5, TextStyle{}, 0});
    auto result = DecorationCompositor::flatten(deco, 0);
    CHECK(result.empty());
}

TEST_CASE("flatten: single full-coverage span") {
    LineDecoration deco;
    TextStyle style;
    style.fg = {255, 0, 0, 255};
    deco.spans.push_back({0, 10, style, 5});

    auto result = DecorationCompositor::flatten(deco, 10);
    REQUIRE(result.size() == 1);
    CHECK(result[0].byte_start == 0);
    CHECK(result[0].byte_end == 10);
    CHECK(result[0].style.fg.r == 255);
    CHECK(result[0].style.fg.g == 0);
}

TEST_CASE("flatten: non-overlapping spans with gaps get default fill") {
    LineDecoration deco;
    TextStyle red;
    red.fg = {255, 0, 0, 255};
    TextStyle blue;
    blue.fg = {0, 0, 255, 255};

    deco.spans.push_back({0, 3, red, 1});
    deco.spans.push_back({7, 10, blue, 1});

    auto result = DecorationCompositor::flatten(deco, 10);
    REQUIRE(result.size() == 3);

    // [0,3) = red
    CHECK(result[0].byte_start == 0);
    CHECK(result[0].byte_end == 3);
    CHECK(result[0].style.fg.r == 255);

    // [3,7) = default
    CHECK(result[1].byte_start == 3);
    CHECK(result[1].byte_end == 7);
    CHECK(result[1].style.fg.r == 220);

    // [7,10) = blue
    CHECK(result[2].byte_start == 7);
    CHECK(result[2].byte_end == 10);
    CHECK(result[2].style.fg.b == 255);
}

TEST_CASE("flatten: overlapping spans, higher priority fg wins") {
    LineDecoration deco;
    TextStyle low;
    low.fg = {100, 100, 100, 255};
    TextStyle high;
    high.fg = {255, 0, 0, 255};

    deco.spans.push_back({0, 10, low, 1});
    deco.spans.push_back({3, 7, high, 5});

    auto result = DecorationCompositor::flatten(deco, 10);
    REQUIRE(result.size() == 3);

    // [0,3) = low
    CHECK(result[0].style.fg.r == 100);

    // [3,7) = high wins
    CHECK(result[1].byte_start == 3);
    CHECK(result[1].byte_end == 7);
    CHECK(result[1].style.fg.r == 255);

    // [7,10) = low
    CHECK(result[2].style.fg.r == 100);
}

TEST_CASE("flatten: bg only applied when bg.a > 0") {
    LineDecoration deco;
    TextStyle s1;
    s1.bg = {0, 0, 0, 0};  // transparent bg
    TextStyle s2;
    s2.bg = {255, 255, 0, 128};  // visible bg, lower priority

    deco.spans.push_back({0, 10, s1, 10});
    deco.spans.push_back({0, 10, s2, 1});

    auto result = DecorationCompositor::flatten(deco, 10);
    REQUIRE(result.size() == 1);
    // s2 has visible bg even though lower priority — it's the highest with bg.a > 0
    CHECK(result[0].style.bg.r == 255);
    CHECK(result[0].style.bg.g == 255);
    CHECK(result[0].style.bg.a == 128);
}

TEST_CASE("flatten: underline union semantics") {
    LineDecoration deco;
    TextStyle s1;
    s1.underline = true;
    s1.underline_color = {255, 0, 0, 255};
    TextStyle s2;
    s2.underline = false;

    deco.spans.push_back({0, 10, s1, 1});
    deco.spans.push_back({0, 10, s2, 5});  // higher priority, but no underline

    auto result = DecorationCompositor::flatten(deco, 10);
    REQUIRE(result.size() == 1);
    // underline is union: any span enables it
    CHECK(result[0].style.underline == true);
    CHECK(result[0].style.underline_color.r == 255);
}

TEST_CASE("flatten: underline_color from highest-priority underlined span") {
    LineDecoration deco;
    TextStyle s1;
    s1.underline = true;
    s1.underline_color = {255, 0, 0, 255};
    TextStyle s2;
    s2.underline = true;
    s2.underline_color = {0, 255, 0, 255};

    deco.spans.push_back({0, 10, s1, 1});
    deco.spans.push_back({0, 10, s2, 5});

    auto result = DecorationCompositor::flatten(deco, 10);
    REQUIRE(result.size() == 1);
    CHECK(result[0].style.underline == true);
    // s2 has higher priority
    CHECK(result[0].style.underline_color.g == 255);
    CHECK(result[0].style.underline_color.r == 0);
}

TEST_CASE("flatten: spans clamped to line bounds") {
    LineDecoration deco;
    TextStyle style;
    style.fg = {255, 0, 0, 255};
    // Span extends beyond line
    deco.spans.push_back({-5, 20, style, 1});

    auto result = DecorationCompositor::flatten(deco, 10);
    REQUIRE(result.size() == 1);
    CHECK(result[0].byte_start == 0);
    CHECK(result[0].byte_end == 10);
    CHECK(result[0].style.fg.r == 255);
}

TEST_CASE("flatten: zero-length spans ignored") {
    LineDecoration deco;
    TextStyle style;
    style.fg = {255, 0, 0, 255};
    deco.spans.push_back({5, 5, style, 1});  // zero-length

    auto result = DecorationCompositor::flatten(deco, 10);
    REQUIRE(result.size() == 1);
    // Should be default since zero-length span is ignored
    CHECK(result[0].byte_start == 0);
    CHECK(result[0].byte_end == 10);
    CHECK(result[0].style.fg.r == 220);
}

TEST_CASE("flatten: custom default_style applied to gaps") {
    TextStyle custom;
    custom.fg = {128, 128, 128, 255};
    custom.bg = {10, 10, 10, 255};

    LineDecoration deco;
    TextStyle red;
    red.fg = {255, 0, 0, 255};
    deco.spans.push_back({3, 7, red, 1});

    auto result = DecorationCompositor::flatten(deco, 10, custom);
    REQUIRE(result.size() == 3);

    // [0,3) = custom default
    CHECK(result[0].style.fg.r == 128);
    CHECK(result[0].style.bg.r == 10);

    // [3,7) = red
    CHECK(result[1].style.fg.r == 255);

    // [7,10) = custom default
    CHECK(result[2].style.fg.r == 128);
    CHECK(result[2].style.bg.r == 10);
}
