# Third-party notices

The C17 port uses SDL3 under its zlib licence. SDL3 is not vendored in this
source snapshot. Supply an installed SDL3 CMake package or an SDL3 source tree;
include SDL's own licence when distributing its runtime library.

Original game material has separate ownership: see [the rights notice](NOTICE.md).

The setup reads downloads (zip, 7z, RAR, tar) with libarchive 3.7.9 (BSD
2-clause), zlib 1.3.1 (zlib licence) and xz 5.8.1's liblzma (0BSD), built from
their pinned release sources and linked in statically
(`tools/deps/build_archive.sh`, `tools/windows/build_deps_archive.cmd`); their
licences are copied into the package's `licenses` folder, and a package
without them is refused.

The music soundfont is a subset of FluidR3 GM 3.1 (MIT, © 2000-2002, 2008
Frank Wen), cut at build time from the SF2 in Debian's fluid-soundfont 3.1
upstream tarball, pinned by its SHA-256 (`tools/soundfont/subset.py`). Its
`COPYING` and `README`, which credits the samples' sources, are kept verbatim
in `data/music` and copied into the package's `licenses` folder; a package
without them is refused. The music is played through TinySoundFont (MIT,
`extern/TinySoundFont`, with its licence and pinned commit).

The setup screen comes from re-setup-kit (MIT, `extern/re-setup-kit`), which
carries stb_truetype and stb_image (public domain or MIT, the reader's choice)
and miniz's inflate (MIT); their licences are in the kit's `third_party`
folders and are copied into the package's `licenses` folder. The screen's
face is Saira Condensed Bold (SIL Open Font License 1.1, "Saira" a reserved
name), unchanged from github.com/google/fonts `ofl/sairacondensed` at
`06bfe0d62a98375cb09c942fa18e68bd32cff057`; it ships with its licence in
`data/setup`.
