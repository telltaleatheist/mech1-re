# Reviewed original executable revisions

SHA-256 hashes of the private DOS installation used for development and the
2026-10-18 transfer. These identify local inputs; no executables are included.
Do not substitute the separately unpacked Reko copies or the SDL executable.

| File | SHA-256 |
| --- | --- |
| BTECH.EXE | `1705b607e798ddfce93da73f999fa98e770e0359c739ec939d1cb7cefc5d3a4c` |
| MW_MAIN.EXE | `5143a95a95e767183894ee326c9112ac4998b536f9b13ff0f8d43e1291c7c808` |
| MW.EXE | `c9ea35f8247c46890279edfd99e7d08c1d2bcd84f855453259c78b5428c97089` |
| MW_EGA.EXE | `fbbeef961ab0308127c390a3e2b345e1d82b6fcebe5c487c5f949532d0abba16` |
| MW_TANDY.EXE | `2f8e1c74cf99fedc41f4bd322e510c7563d01a674ddced0601eeb2772efe4618` |

In this set BTECH.EXE is the 1989-11-03 revision and MW_MAIN.EXE the
1990 one; the trace goldens use a 1990-01-22 installation (both exes 1990).

## Revisions the importer accepts

The importer (`story/`, `docs/phase2/PLAN.md` sections 2 and 7) takes each
exe in either revision below. Nothing else differs in what it imports:

| File | 1989-11-03 | 1990-01-22 |
| --- | --- | --- |
| BTECH.EXE | `1705b607e798ddfce93da73f999fa98e770e0359c739ec939d1cb7cefc5d3a4c` (174,267 bytes) | `832c39b8859ec102b955ed6022ce93aa7aa2cf3d504b1e0d63dfe1cad6fb9409` (174,533 bytes) |
| MW_MAIN.EXE | `58bd9ebec6f987759a6d229d1f2b136a176aaf14f1bdb251023fee5cda8b1293` | `5143a95a95e767183894ee326c9112ac4998b536f9b13ff0f8d43e1291c7c808` |

- BTECH.EXE: every region the importer reads is byte-identical in both.
- MW_MAIN.EXE: the two differ in 11 bytes, all in the copy protection's code
  (081B:174A..1782), which the port drops; its data segment is identical.
- MW.EXE, MW_EGA.EXE and MW_TANDY.EXE are the same in both releases. The
  importer reads MW_EGA.EXE too (one 2-byte span); MW.EXE and MW_TANDY.EXE
  it doesn't need. After the import, play opens no exe.
- PHAPCK.TBL (the Phoenix Hawk's model bank) is damaged in the 1989 copy we
  have, the one the tests use. It has the 1990 one's length and differs from it
  in 18,235 of its 24,673 bytes: one byte at 0x166D, in one bit; and, from
  0x1800 to 0x5FFF, where it holds MARPCK.TBL's bytes at the same offsets.
  The rest is the same. How it came to be isn't known, nor whether another 1989
  copy has it: this is about that copy, not the release. A battle with a Phoenix
  Hawk stops with it: the bank doesn't unpack to the length its header gives,
  and the program says "phapck.tbl unpacks to less than its header says" and
  what to do (before B4e, only "Rejected condition: total!=allocation.byteCount").
  Nothing imports the file; the traces use the 1990 one.

The tests take the further revisions as `MW1O_BTECH_REFERENCE` (the 1989
BTECH.EXE) and `MW1O_MAIN_1989` (the 1989 MW_MAIN.EXE), besides the game folder
`MW1O_GAME`.

This is not a claim of compatibility with every release/language variant.
Executable-derived animation/chassis tables require a reviewed revision.
Separate native model/map browsing can still work without recognising an EXE.
Tandy's listed identity does not make the playable Tandy adapter release-qualified.
Retain the remaining installation resources; these hashes are not a minimal
list of files sufficient to play the game.
