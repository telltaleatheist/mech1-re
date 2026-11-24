#include "mechwarrior/problem.h"
#include <stdio.h>

static char lastProblem[1024];

void MwNoteProblem(const char *text) {snprintf(lastProblem,sizeof(lastProblem),"%s",text);}
void MwReportProblem(const char *text)
{
    fprintf(stderr,"MechWarrior: %s\n",text);
    MwNoteProblem(text);
}
const char *MwLastProblem(void) {return lastProblem;}
