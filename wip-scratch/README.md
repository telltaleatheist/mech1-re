Slice 6a work in progress (removed before the real commit).

- probe.patch + Probe.h: the scratch probe (dumps __DATA before/after each
  exe loader to $MW1O_PROBE_DIR). Build with
  CMAKE_CXX_FLAGS/CMAKE_C_FLAGS="-mllvm -enable-global-merge=false".
- probe-diff.py: which globals each loader changes (sweep).
- probe-eq.py: old vs new build, symbol by symbol, per dump point.
- s6a-edit.py: the class A transformer; s6-sweep.txt its input.

State: class A zeros applied (370; randomStateWord0/1 reverted, they are
nonzero: class B). Dispatch/directory checks moved to check.* import pieces.
Proof done on both folders (1990 and 1989): combat_cockpit trace identical,
no loader introduces a difference. Still to do: comments/doc pass, PLAN
record, INVENTORY regen, fresh build, gate in both setups, break check.
