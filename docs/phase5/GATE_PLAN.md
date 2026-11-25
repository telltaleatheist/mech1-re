# The release gate

Run once, when everything that goes into the release is on main and the project owner has said go. Nothing here has
been run for this page. Times are measured on earlier commits unless marked as an estimate.

**The candidate** is one commit of main. Every step below is on fresh builds of it. If it changes afterwards, the
steps that change can affect are run again. B4f's campaign and the setup's file check
are on main (a97a6d5).

**A pass** is the count in the table. A skipped test isn't a pass. A log or golden that moves stops the gate until
it is attributed.

## What is covered, and by what

| What | Last covered | Since then on main |
| --- | --- | --- |
| Traces, events, frames, 1990 and 1989, and audio; ASan on the 8 traces (1990) and audio | 4632030 (B4g), Mac | B4f's combat (checked in its own slice at 92729fb: tests, 1990 traces, events on both revisions, the sanitizer on two traces), r-windows, r-story. Frames, 1989's traces and audio: not since |
| Mac ctest | a981d92: 7 of 7 of the tests it touched, fresh build, 0 warnings; 0de343e: 18 of 18, exe_free among them | the rest of the tests not on one commit |
| Windows ctest | 0de343e: 35 of 37 (exe_free and events skipped); r-log 3cf061f: 6 of 6 of the tests it touched; 7d9c5d9: 8 of 8 targeted and two 1990 traces (company, combat_cockpit); a97a6d5: setup, status_setup, notice | the other tests, and nothing of the opt-ins |
| Windows traces | a run from before R's preparation; its commit isn't recorded here | treat as uncovered |
| Linux ctest | c4f217c: 28 of 28 on both revisions, events run, exe_free skipped | 10 tests added since; treat as uncovered |
| Packages, first run | 8122a17, all three platforms (before the story merge; no packaging change since) | the story and the rest of the merge |
| Message box's text | r-log 3cf061f on the Windows package 8122a17 with the new program: exact command, desktop session, notice file | the box itself was never seen by anyone |

## The steps, in order

Each needs the game (1990) and, where named, the 1989 copy; the game folder is only read. Hidden, dummy audio, no more
than four jobs, one suite at a time. Opt-ins are set together with the rest: `MW1O_EXE_FREE=1 MW1O_EVENTS=1`.

| # | Step | Where | Expected | Time |
| --- | --- | --- | --- | --- |
| 1 | Fresh release build, warnings read | Mac, Linux, Windows | 0 warnings of ours | Mac and Linux estimate 3 min; Windows 16 s at r-log 3cf061f |
| 2 | Full ctest, 1990 game, both BTECH revisions given, both opt-ins on | Mac | 38 of 38, none skipped (count at a97a6d5; new tests raise it) | estimate 8 min |
| 3 | On the 1989 copy, only the tests that use the game's files (list below), both opt-ins on | Mac | 13 of 13, none skipped (exe_free compares its two runs with each other, so 1989's two known traces don't fail it) | estimate 7 min, nearly all of it events and exe_free |
| 4 | Full ctest as step 2, 1990; then step 3's list, 1989 | Linux | 38 of 38, then 13 of 13 (at c4f217c events alone took 349 s, the whole 375 s) | estimate 8 min each |
| 5 | Full ctest, 1990 game | Windows | 36 of 38, skipped: exe_free and events. With both opt-ins on: 38 of 38, if Windows passes them; the first time. Every opt-in failure on Windows: stop and tell the lead | 55 s measured; opt-ins estimate 15 min |
| 6 | Traces, `check.py` | Mac, Linux, Windows | 1990: 8 of 8. 1989: 6 of 8, the failures combat_cockpit and combat_retreat (its tick is one ahead; see "For R" in the plan). Any other name is new. On the Mac, the runs' frames, both revisions, compared with the frames kept from 4632030 (864 files, byte for byte the same at 17b8c2a): identical | Mac estimate 2 min per revision |
| 7 | `audio.py` | Mac | 2 of 2 | estimate 1 min |
| 8 | Sanitizer build (address, and bounds for combat) on the 8 traces and the sound logs | Mac | 0 reports | estimate 4 min each |
| 9 | Packages from the local tag, the build's stamp and the release-label checks on | all three | zip and Setup.exe; dmg, both architectures; tar.gz and AppImage; smoke test and first run each; `sums.py` over all | Windows about 1 min, Linux 1 min, Mac about 2 min |
| 10 | `combat_cockpit` on the Intel half of the Mac package, under Rosetta | Mac | the trace matches | 10 s measured |
| 11 | The kit's non-ASCII name case, its home away from the program's folder, with and without the manifest | Windows | no abort. At 7d9c5d9 (kit 2f0feb1): none, with the manifest and without it; on 9341f31 four of the ten cases abort, all in the program's own folders | 5 min |
| 12 | The setup's file check on both copies, 1990 and 1989 (and on the served setup, which status_setup and every package's smoke test ask) | Mac, Linux, Windows | each copy accepted as its revision; its own failures named as its branch says | Windows: 13 to 14 ms for the 1990 folder |

Steps 6 (both revisions), 7 and 8's address half, and the events of both revisions that steps 2 and 3 now run, took about 15 minutes together at 4632030 on the Mac, builds included.

Step 3's list (with `MW1O_GAME` the 1989 copy and both BTECH revisions given) is `btech_revisions`, `combat_records`, `words_exe`, `rules_exe`, `ui_exe`, `start_exe`, `import_exe`,
`text_audit`, `status_setup`, `read_only_install`, `feed_calls`, `events` and `exe_free`:

    ctest -j1 -R '^(btech_revisions|combat_records|words_exe|rules_exe|ui_exe|start_exe|import_exe|text_audit|status_setup|read_only_install|feed_calls|events|exe_free)$'

`setup` isn't in it: it wants a whole copy in `MW1O_GAME` and the 1989 one's PHAPCK.TBL is known to be damaged, so two
of its checks fail there by design. It checks the 1989 copy through `MW1O_BTECH_REFERENCE`, in steps 2, 4 and 5.

Sum, an estimate: about an hour of Mac time and 45 minutes of PC time. Steps 2 to 5 can be skipped only if a later slice's end set
on the same commit already did them; then name it.

Not repeated, because no path to them changed since they were checked: the soundfont's subset and bank goldens
(`soundfont_subset`, `soundfont_bank` run in step 2 anyway), and the controller path (`joystick`, `sdl_joystick`,
same). MW2's recognise rules for DATABASE.MW2 and MECH2 were never tried on real data; nothing here covers them.

## For the project owner

| What | How | Notes |
| --- | --- | --- |
| The listening list | the rendered songs and the thirteen effects, on the packaged build's sound | signed off as it stood in S4; the packaged build's sound is the new part |
| The visual acceptance list | [VISUAL_ACCEPTANCE.md](../VISUAL_ACCEPTANCE.md), on the packaged builds | includes the message box of a refused start: its text was checked, the box itself was not |
| The licence page | the draft in [LICENCE_PAGE_DRAFT.md](LICENCE_PAGE_DRAFT.md) | wording to settle; no change to LICENSE or NOTICE until then |
| The AppImage's runtime | libfuse (LGPL 2.1) and its notice in the image | answered: it can ship as an AppImage |
| The release | tag, notes, upload | only on the owner's go |
