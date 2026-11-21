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
    {ImportPiece::Kind::String, "intro.narration.1", "MW_MAIN.EXE", 0x1090, 0x129C, 37, "397d73b46f90663d"},
    {ImportPiece::Kind::String, "intro.narration.2", "MW_MAIN.EXE", 0x1090, 0x12C1, 36, "57b43fd7aa9f2966"},
    {ImportPiece::Kind::String, "intro.narration.3", "MW_MAIN.EXE", 0x1090, 0x12E5, 34, "700091be990530db"},
    {ImportPiece::Kind::String, "intro.narration.4", "MW_MAIN.EXE", 0x1090, 0x1307, 35, "42a9222ef3dac26a"},
    {ImportPiece::Kind::String, "intro.narration.5", "MW_MAIN.EXE", 0x1090, 0x132A, 37, "7146a92d9054d394"},
    {ImportPiece::Kind::String, "intro.narration.6", "MW_MAIN.EXE", 0x1090, 0x134F, 40, "5702089f471d86d7"},
    {ImportPiece::Kind::String, "intro.narration.7", "MW_MAIN.EXE", 0x1090, 0x1377, 39, "19279b5d26a37b41"},
    {ImportPiece::Kind::String, "intro.narration.8", "MW_MAIN.EXE", 0x1090, 0x139E, 36, "739c137d372376b1"},
    {ImportPiece::Kind::String, "intro.narration.9", "MW_MAIN.EXE", 0x1090, 0x13C2, 38, "5c2d0b6e4779049f"},
    {ImportPiece::Kind::String, "intro.narration.10", "MW_MAIN.EXE", 0x1090, 0x13E8, 35, "348e7b5c5449e912"},
    {ImportPiece::Kind::String, "deployment.uneventful", "MW_MAIN.EXE", 0x1090, 0x140D, 86, "29f052d25a064008"},
    {ImportPiece::Kind::String, "after_action.failure.kurita", "MW_MAIN.EXE", 0x1090, 0x149B, 141, "1684af330959a832"},
    {ImportPiece::Kind::String, "after_action.failure.liao", "MW_MAIN.EXE", 0x1090, 0x1528, 115, "bee731090e08e7cc"},
    {ImportPiece::Kind::String, "after_action.failure.general", "MW_MAIN.EXE", 0x1090, 0x159B, 145, "3062a4465f1f73ff"},
    {ImportPiece::Kind::String, "after_action.success.kurita", "MW_MAIN.EXE", 0x1090, 0x1674, 98, "6c14916f2849bd72"},
    {ImportPiece::Kind::String, "after_action.success.liao", "MW_MAIN.EXE", 0x1090, 0x16D6, 108, "1ece42345b47cc27"},
    {ImportPiece::Kind::String, "after_action.success.general", "MW_MAIN.EXE", 0x1090, 0x1742, 68, "0e9ddf9ee0b83613"},
    {ImportPiece::Kind::String, "settlement.deduction.supply", "MW_MAIN.EXE", 0x1090, 0x178C, 168, "f26b49b9a54b130b"},
    {ImportPiece::Kind::String, "settlement.deduction.performance", "MW_MAIN.EXE", 0x1090, 0x1834, 172, "ff78e7aba099f5cf"},
    {ImportPiece::Kind::String, "settlement.deduction.transport", "MW_MAIN.EXE", 0x1090, 0x18E0, 152, "aba7e5c3d4836ded"},
    {ImportPiece::Kind::String, "settlement.no_payment", "MW_MAIN.EXE", 0x1090, 0x19A2, 149, "befb4286b934a5d1"},
    {ImportPiece::Kind::String, "dark_wing.victory", "MW_MAIN.EXE", 0x1090, 0x1A37, 388, "62dee5fddc02c14e"},
    {ImportPiece::Kind::String, "dark_wing.attack_failed", "MW_MAIN.EXE", 0x1090, 0x1BBC, 73, "a459440341ba4363"},
    {ImportPiece::Kind::String, "contract.narrative", "MW_MAIN.EXE", 0x1090, 0x4890, 316, "3add0af80e6e8650"},
    {ImportPiece::Kind::String, "contract.refusal.house", "MW_MAIN.EXE", 0x1090, 0x4C87, 95, "e87f933362b21f1f"},
};
}  // namespace

std::span<const ImportPiece> import_table() { return kTable; }

}  // namespace mw1o::import
