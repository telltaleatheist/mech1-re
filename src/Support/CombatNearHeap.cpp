#include "mechwarrior/combat_heap.h"
#include "mechwarrior/combat_sound.h"

/* The sound code's walk over its allocation nodes after a release (1ADF; B5, docs/phase5/NO_EMULATION_PLAN.md). The
 * original could read a released node's bytes after another owner reused them; step 0 measured the traces and every
 * sound script and saw none, so a released node keeps its own last link (the heap keeps it alive until the combat's
 * heap is discarded). Callers stay as they are until S8's combat sound commit, then call these directly or not at all. */
BtechSoundAllocationNode *BtechHostResolveReleasedSoundNode(BtechSoundAllocationNode *node) {return node;}

void BtechHostStoreSoundAllocationNext(BtechSoundAllocationNode *node,BtechSoundAllocationNode *next) {node->next=next;}
