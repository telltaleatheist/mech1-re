#ifndef MECHWARRIOR_PROBLEM_H
#define MECHWARRIOR_PROBLEM_H
/* Why the program can't start or go on, for the player: the SDL host shows the
 * last one (src/SDL/SDLNotice.h). Support/Problem.cpp holds nothing else, so a
 * test with its own MwAbortWithReason can link it. */
void MwNoteProblem(const char *text);
/* Prints "MechWarrior: <text>" to stderr and notes it. */
void MwReportProblem(const char *text);
/* The last one noted, "" when there is none. */
const char *MwLastProblem(void);
#endif
