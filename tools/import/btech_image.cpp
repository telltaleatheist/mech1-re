// Writes BTECH.EXE's load image, unpacked and rebased into the reference
// layout (src/Support/BtechImage.cpp, as the game and the importer read it),
// to stdout; the revision goes to stderr. make_import_table.py runs it, so
// the table hashes the same bytes the importer checks. The image stays in
// memory (a pipe), never on disk.
//
//     btech_image GAME/BTECH.EXE > image
#include "mechwarrior/combat_installation.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: btech_image BTECH.EXE > image\n");
        return 2;
    }
    std::ifstream in(argv[1], std::ios::binary);
    const std::vector<std::uint8_t> packed((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (!in.eof() && in.fail()) {
        std::fprintf(stderr, "btech_image: can't read %s\n", argv[1]);
        return 1;
    }
    std::uint8_t* image = nullptr;
    BtechRevision revision = BTECH_REVISION_REFERENCE;
    if (!BtechDecodeImage(packed.data(), packed.size(), &image, &revision)) {
        std::fprintf(stderr, "btech_image: %s is not a known BTECH.EXE revision\n", argv[1]);
        return 1;
    }
    const bool written = std::fwrite(image, 1, BTECH_IMAGE_BYTES, stdout) == BTECH_IMAGE_BYTES && std::fflush(stdout) == 0;
    std::free(image);
    std::fprintf(stderr, "btech_image: revision %s\n", revision == BTECH_REVISION_REFERENCE ? "reference" : "1990-01-22");
    return written ? 0 : 1;
}
