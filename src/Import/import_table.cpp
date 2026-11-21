// Written by tools/import/make_import_table.py from pieces.txt; don't edit.
#include "ImportTable.h"

namespace mw1o::import {

namespace {
constexpr ImportPiece kTable[] = {
    {ImportPiece::Kind::String, "intro.caption.1.left", "MW_MAIN.EXE", 0x1090, 0x1C37, 32, "8e9559af32365a09"},
    {ImportPiece::Kind::String, "intro.caption.1.right", "MW_MAIN.EXE", 0x1090, 0x1C5B, 37, "aa9f7ab6c331b3dc"},
    {ImportPiece::Kind::String, "intro.caption.2.left", "MW_MAIN.EXE", 0x1090, 0x1C84, 37, "feb137c742b03215"},
    {ImportPiece::Kind::String, "intro.caption.2.right", "MW_MAIN.EXE", 0x1090, 0x1CAD, 34, "d24f21963dc62d74"},
    {ImportPiece::Kind::String, "intro.caption.3.left", "MW_MAIN.EXE", 0x1090, 0x1CD3, 76, "8fb7c254b101ee20"},
    {ImportPiece::Kind::String, "intro.caption.3.right", "MW_MAIN.EXE", 0x1090, 0x1D23, 48, "a0791932efe5b977"},
    {ImportPiece::Kind::String, "intro.caption.4.left", "MW_MAIN.EXE", 0x1090, 0x1D57, 24, "4d3d5c7c9a9bce6c"},
    {ImportPiece::Kind::String, "intro.caption.4.right", "MW_MAIN.EXE", 0x1090, 0x1D73, 27, "ed64626bfe184634"},
    {ImportPiece::Kind::String, "intro.caption.5.left", "MW_MAIN.EXE", 0x1090, 0x1D92, 61, "e0d2c52886238c71"},
    {ImportPiece::Kind::String, "intro.caption.5.right", "MW_MAIN.EXE", 0x1090, 0x1DDC, 27, "3d6ddf21a0e785cd"},
    {ImportPiece::Kind::String, "intro.caption.6.left", "MW_MAIN.EXE", 0x1090, 0x1DFB, 29, "9b799cf98e5a4151"},
    {ImportPiece::Kind::String, "intro.caption.6.right", "MW_MAIN.EXE", 0x1090, 0x1E1C, 36, "9055b0ac9ed50647"},
    {ImportPiece::Kind::String, "intro.caption.7.left", "MW_MAIN.EXE", 0x1090, 0x1E44, 116, "f26a6bdf000fd142"},
    {ImportPiece::Kind::String, "intro.caption.7.right", "MW_MAIN.EXE", 0x1090, 0x1EBC, 94, "98b4a0fa9ba7b257"},
};
}  // namespace

std::span<const ImportPiece> import_table() { return kTable; }

}  // namespace mw1o::import
