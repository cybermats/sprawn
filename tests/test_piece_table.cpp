#include <doctest/doctest.h>

#include "../src/backend/piece_table.h"

#include <stdexcept>

using namespace sprawn;

TEST_CASE("PieceTable: empty") {
    PieceTable pt;
    CHECK(pt.length() == 0);
    CHECK(pt.text().empty());
}

TEST_CASE("PieceTable: original buffer") {
    const char* data = "Hello, World!";
    auto span = std::span(reinterpret_cast<const std::byte*>(data), 13);
    PieceTable pt(span);

    CHECK(pt.length() == 13);
    CHECK(pt.text() == "Hello, World!");
}

TEST_CASE("PieceTable: insert at beginning") {
    const char* data = "World";
    auto span = std::span(reinterpret_cast<const std::byte*>(data), 5);
    PieceTable pt(span);

    pt.insert(0, "Hello, ");
    CHECK(pt.text() == "Hello, World");
    CHECK(pt.length() == 12);
}

TEST_CASE("PieceTable: insert at end") {
    const char* data = "Hello";
    auto span = std::span(reinterpret_cast<const std::byte*>(data), 5);
    PieceTable pt(span);

    pt.insert(5, ", World!");
    CHECK(pt.text() == "Hello, World!");
    CHECK(pt.length() == 13);
}

TEST_CASE("PieceTable: insert in middle") {
    const char* data = "Heo";
    auto span = std::span(reinterpret_cast<const std::byte*>(data), 3);
    PieceTable pt(span);

    pt.insert(2, "ll");
    CHECK(pt.text() == "Hello");
    CHECK(pt.length() == 5);
}

TEST_CASE("PieceTable: insert into empty") {
    PieceTable pt;
    pt.insert(0, "Hello");
    CHECK(pt.text() == "Hello");
    CHECK(pt.length() == 5);
}

TEST_CASE("PieceTable: multiple inserts") {
    PieceTable pt;
    pt.insert(0, "Hello");
    pt.insert(5, " World");
    pt.insert(5, ",");
    CHECK(pt.text() == "Hello, World");
}

TEST_CASE("PieceTable: erase from beginning") {
    const char* data = "Hello, World!";
    auto span = std::span(reinterpret_cast<const std::byte*>(data), 13);
    PieceTable pt(span);

    pt.erase(0, 7);
    CHECK(pt.text() == "World!");
    CHECK(pt.length() == 6);
}

TEST_CASE("PieceTable: erase from end") {
    const char* data = "Hello, World!";
    auto span = std::span(reinterpret_cast<const std::byte*>(data), 13);
    PieceTable pt(span);

    pt.erase(12, 1);
    CHECK(pt.text() == "Hello, World");
}

TEST_CASE("PieceTable: erase from middle") {
    const char* data = "Hello, World!";
    auto span = std::span(reinterpret_cast<const std::byte*>(data), 13);
    PieceTable pt(span);

    pt.erase(5, 7);
    CHECK(pt.text() == "Hello!");
}

TEST_CASE("PieceTable: erase across pieces") {
    PieceTable pt;
    pt.insert(0, "AAA");
    pt.insert(3, "BBB");
    pt.insert(6, "CCC");
    CHECK(pt.text() == "AAABBBCCC");

    pt.erase(2, 5);
    CHECK(pt.text() == "AACC");
}

TEST_CASE("PieceTable: interleaved insert and erase") {
    const char* data = "abcdef";
    auto span = std::span(reinterpret_cast<const std::byte*>(data), 6);
    PieceTable pt(span);

    pt.insert(3, "XYZ");    // abcXYZdef
    CHECK(pt.text() == "abcXYZdef");

    pt.erase(1, 4);         // aZdef
    CHECK(pt.text() == "aZdef");

    pt.insert(1, "!!!");    // a!!!Zdef
    CHECK(pt.text() == "a!!!Zdef");
}

TEST_CASE("PieceTable: text extraction with offset") {
    const char* data = "Hello, World!";
    auto span = std::span(reinterpret_cast<const std::byte*>(data), 13);
    PieceTable pt(span);

    CHECK(pt.text(0, 5) == "Hello");
    CHECK(pt.text(7, 5) == "World");
    CHECK(pt.text(7, 100) == "World!");
}

TEST_CASE("PieceTable: insert out of range throws") {
    PieceTable pt;
    CHECK_THROWS_AS(pt.insert(1, "x"), std::out_of_range);
}

TEST_CASE("PieceTable: erase out of range throws") {
    const char* data = "abc";
    auto span = std::span(reinterpret_cast<const std::byte*>(data), 3);
    PieceTable pt(span);

    CHECK_THROWS_AS(pt.erase(2, 5), std::out_of_range);
}

TEST_CASE("PieceTable: text with overflow clamps") {
    const char* data = "Hello";
    auto span = std::span(reinterpret_cast<const std::byte*>(data), 5);
    PieceTable pt(span);

    // pos + count would overflow size_t
    CHECK(pt.text(3, SIZE_MAX) == "lo");
    // pos beyond length
    CHECK(pt.text(10, 5) == "");
}

TEST_CASE("PieceTable: erase with count 0 is a no-op") {
    const char* data = "Hello";
    auto span = std::span(reinterpret_cast<const std::byte*>(data), 5);
    PieceTable pt(span);

    pt.erase(2, 0);
    CHECK(pt.text() == "Hello");
    CHECK(pt.length() == 5);
}
