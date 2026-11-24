# Checking gameplay presentation

Screenshots complement gameplay checks; they do not prove that repairs, payment,
damage or mission results were applied correctly. Compare the same situation in
the original and port, recording company state and actions as well as images.

## Campaign coverage

Check distinct planet hubs, Bar/recruitment, crew assignments, contracts and
negotiation, damaged/functional Mechs, component/all repairs, rearming, trading,
travel/arrival and save/load. Compare the visible information with changes to
C-bills, ammunition, Mech condition, wages, assignments and the calendar.

## Combat coverage

Check briefing, cockpit, target selection, movement, torso rotation, weapons,
heat, damage, command screen and map. Include different chassis and landscapes,
jump-capable Mechs, impacts, destruction, retreat and mission completion.
Compare motion/effects over sequences, not just a stationary frame.

## A refused start on Windows

On a copy of the unzipped Windows package, in PowerShell in that folder:

    $env:MECHWARRIOR_TIMING_CONFIG = "absent.ini"; $env:MECHWARRIOR_SAVE_DIRECTORY = "$env:TEMP\mw1r-box"; .\mw1r.exe "no game here"

A box titled MechWarrior should appear, with the reason and where the log is,
and the program should end when it is closed. No game, save or setting of the
player's is touched. Close that PowerShell afterwards, as it keeps both
variables.

## Captures and their limits

F12 records actual SDL output with supporting state. Keep raw comparison sets
under ignored local capture directories. Do not upload assets or saves. The
documentation gallery contains selected real captures, not matched-original
goldens or proof that all acceptance cases passed.

For exact graphics comparison use equal native framebuffer dimensions and
palettes without filtering, CRT effects or reference-image resizing. A deliberately
different simulation state cannot be fixed with a generous pixel tolerance.
Audio needs timing/listening checks as well as recorded samples.

Ask before taking control of the shared desktop for a live session. Detached
off-screen UI checks and analysis of existing captures do not take user input.
Current playable limits are in [runtime status](RUNTIME_STATUS.md); detailed
previous capture records are historical evidence.
