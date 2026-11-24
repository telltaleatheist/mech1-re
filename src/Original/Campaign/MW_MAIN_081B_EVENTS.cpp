#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/campaign_continuations.h"
#include "mechwarrior/campaign_events.h"
#include "mechwarrior/campaign_economy.h"
#include <stdlib.h>

uint16_t (*campaignEventChoice)(const MwEventCommand *command)=MwEventOpenChoiceContinuation;
void (*campaignEventPlanetSubstitution)(const MwEventCommand *command)=MwEventSubstitutePlanetName;
/* Host continuation transport for the native PUSH SI/CALL639B/POP SI frame.
 * Not new1090 state, an event manager or a second script/memory image. */
static MwCampaignEvent **dispatchListCursor;
uint16_t campaignEventReturnContinuation;
uint16_t campaignEventDispatchReturnContinuation;

/* MW_MAIN081B:6236..6292. The directory suffixes share one list owner. */
uint16_t MwDispatchCampaignEvents(uint16_t eventClass,uint16_t entryOffset)
{
    if (entryOffset==CAMPAIGN_CONTINUE_NEXT_EVENT) {campaignEventReturnContinuation=0;goto nextEvent;}
    if (entryOffset!=CAMPAIGN_CALL_DISPATCH_EVENTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: entryOffset!=CAMPAIGN_CALL_DISPATCH_EVENTS");
    campaignEvents.campaignEventClass=eventClass;
    if (campaignEvents.eventDirectoryIndex>=CAMPAIGN_EVENT_LIST_COUNT || !campaignEvents.eventLists[campaignEvents.eventDirectoryIndex]) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: eventDirectoryIndex>=CAMPAIGN_EVENT_LIST_COUNT || !eventLists[eventDirectoryIndex]"); /* Unrepresented native list/DS0000 mapping, not a null-list success. */
    dispatchListCursor=campaignEvents.eventLists[campaignEvents.eventDirectoryIndex];
    campaignEvents.dispatchedEventCount=0;
    for (;;)
    {
        const MwCampaignEvent *event;event=*dispatchListCursor;
        if (!event) break; /* Decoded native zero list terminator. */
        if ((uint8_t)campaignEvents.campaignEventClass==event->classCode)
        {
            campaignEvents.currentEventId=event->eventId;
            bool evaluate=campaignEvents.campaignEventClass==EVENT_CLASS_REPEATABLE;
            if (!evaluate)
            {
                if (campaignEvents.currentEventId>=CAMPAIGN_EVENT_SEEN_FLAGS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: currentEventId>=CAMPAIGN_EVENT_SEEN_FLAGS");
                evaluate=campaignEvents.eventSeenFlags[campaignEvents.currentEventId]!=1;
            }
            if (evaluate)
            {
                bool carry;
                campaignEventReturnContinuation=CAMPAIGN_CONTINUE_NEXT_EVENT;
                uint16_t transfer=MwInterpretCampaignEvent(event,&carry);
                if (transfer) return transfer; /* Keep saved list cursor for choice return6290. */
                campaignEventReturnContinuation=0;
            }
        }
nextEvent:
        ++dispatchListCursor; /* Native ADD SI,2 after POP SI. Reread live class. */
    }
    if (campaignEvents.campaignEventClass==2 && campaignEvents.dispatchedEventCount==0)
    {
        if (campaignEvents.campaignEventClass>=7 || !campaignEvents.eventClassHandlers[campaignEvents.campaignEventClass]) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: campaignEventClass>=7 || !eventClassHandlers[campaignEventClass]");
        campaignEvents.eventClassHandlers[campaignEvents.campaignEventClass](0x0071);
    }
    return 0;
}

/* MW_MAIN081B:6293..629F. Class5E62 is inherited, NOT changed to record class06.
 *629F transports the suspended interpreter return when event76 opens choices. */
uint16_t MwDispatchFixedCampaignEvent(void)
{
    campaignEvents.currentEventId=EVENT_CONTINUE_OR_QUIT;
    if (!campaignEvents.fixedCampaignEvent) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !fixedCampaignEvent");
    campaignEventReturnContinuation=CAMPAIGN_CONTINUE_VICTORY_EVENT_RETURN;
    bool carry;
    uint16_t continuation=MwInterpretCampaignEvent(campaignEvents.fixedCampaignEvent,&carry);
    if (!continuation) campaignEventReturnContinuation=0;
    return continuation;
}

/* Test a story event's conditions, then execute its accepted actions or suspend
 * for the commander's choice. One-shot events are marked only on acceptance.
 * MW_MAIN081B:639B..6424. Typed next references replace only the serialized
 * cursor encoding. The original alternation, live state and handler order stay.
 * A choice transfer leaves before the seen/count acceptance tail. */
uint16_t MwInterpretCampaignEvent(const MwCampaignEvent *event,bool *carry)
{
    campaignEvents.eventOperandCursor=event->script;
    campaignEvents.eventConditionResult=0;campaignEvents.eventCombineWithAnd=0;
    *carry=false;
    while (campaignEvents.eventOperandCursor)
    {
        const MwEventCommand *command=campaignEvents.eventOperandCursor;
        switch (command->opcode)
        {
        case EVENT_CONDITION_RANDOM_THRESHOLD:MwEventTestRandomThreshold(command);break;
        case EVENT_CONDITION_SCHEDULED_DATE:MwEventTestScheduledDate(command);break;
        case EVENT_CONDITION_CURRENT_PLANET:MwEventTestCurrentPlanet(command);break;
        case EVENT_CONDITION_SEEN_EVENT:MwEventTestSeenEvent(command);break;
        case EVENT_CONDITION_HOUSE_STANDING:MwEventTestControllingHouseStanding(command);break;
        case EVENT_CONDITION_DIFFERENT_PLANET:MwEventTestDifferentPlanet(command);break;
        case EVENT_CONDITION_UNCHANGED:campaignEvents.eventOperandCursor=command->next;break; /* Native6561 RET. */
        case EVENT_CONDITION_UNRESOLVED_TARGET:/* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Native table target0000 is NOT a no-op."); /* Native table target0000 is NOT a no-op. */
        case EVENT_CONDITION_PLAYER_COMBAT_LOSS:campaignEvents.eventOperandCursor=command->next;MwEventTestPlayerCombatLoss();break;
        case EVENT_CONDITION_CHOICES:
            if (campaignEvents.eventConditionResult==1)
            {
                campaignEvents.eventOperandCursor=command;
                if (!campaignEventChoice) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !campaignEventChoice");
                return campaignEventChoice(command);
            }
            campaignEvents.eventOperandCursor=command->next;
            break; /* Decoded count*4+1 false-path operand skip. */
        default:/* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Unrepresented out-of-table target."); /* Unrepresented out-of-table target. */
        }
        if (!campaignEvents.eventOperandCursor) break;
        command=campaignEvents.eventOperandCursor;
        switch (command->opcode)
        {
        case EVENT_ACTION_UNCHANGED_FIRST:case EVENT_ACTION_UNCHANGED_SECOND:case EVENT_ACTION_UNCHANGED_THIRD:campaignEvents.eventOperandCursor=command->next;break; /* Genuine RET entries. */
        case EVENT_ACTION_QUEUE_RESTART:campaignEvents.eventOperandCursor=command->next;MwEventQueueCampaignRestart();break;
        case EVENT_ACTION_DATE_WINDOW:MwEventRestrictToDateWindow(command);break;
        case EVENT_ACTION_ADJUST_FUNDS:MwEventAdjustFunds(command);break;
        case EVENT_ACTION_USE_AND:campaignEvents.eventOperandCursor=command->next;MwEventUseAnd();break;
        case EVENT_ACTION_USE_OR:campaignEvents.eventOperandCursor=command->next;MwEventUseOr();break;
        case EVENT_ACTION_SUBSTITUTE_PLANET:
            if (campaignEvents.eventConditionResult==1)
            {
                if (!campaignEventPlanetSubstitution) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !campaignEventPlanetSubstitution");
                campaignEventPlanetSubstitution(command);
            }
            campaignEvents.eventOperandCursor=command->next;
            break;
        case EVENT_ACTION_CHOICES:
            if (campaignEvents.eventConditionResult==1)
            {
                campaignEvents.eventOperandCursor=command;
                if (!campaignEventChoice) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !campaignEventChoice");
                return campaignEventChoice(command);
            }
            campaignEvents.eventOperandCursor=command->next;
            break;
        default:/* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Campaign event has an unsupported action opcode.");
        }
    }
    if (campaignEvents.eventConditionResult!=1) return 0;
    if (campaignEvents.currentEventId==EVENT_PORT_ARREST && campaignEvents.darkWingBaseVictoryRecorded) return 0;
    if ((int16_t)campaignEvents.currentEventId>=EVENT_FIRST_HOUSE_REPUTATION_ARRIVAL && campaignEvents.currentEventId<=EVENT_LAST_HOUSE_REPUTATION_ARRIVAL)
    {
        if (campaignEvents.planetArrivalEventsPending!=1) return 0;
    }
    else
    {
        if (campaignEvents.currentEventId>=CAMPAIGN_EVENT_SEEN_FLAGS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: currentEventId>=CAMPAIGN_EVENT_SEEN_FLAGS");
        campaignEvents.eventSeenFlags[campaignEvents.currentEventId]=1;
    }
    if (campaignEvents.campaignEventClass>=7 || !campaignEvents.eventClassHandlers[campaignEvents.campaignEventClass]) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: campaignEventClass>=7 || !eventClassHandlers[campaignEventClass]");
    campaignEvents.eventClassHandlers[campaignEvents.campaignEventClass](campaignEvents.currentEventId);
    ++campaignEvents.dispatchedEventCount;
    *carry=true;
    return 0;
}

/* MW_MAIN081B:6570..6586;6569/656E are its false/true labels. */
void MwCombineEventCondition(uint8_t condition)
{
    if (!campaignEvents.eventCombineWithAnd) campaignEvents.eventConditionResult|=condition;
    else campaignEvents.eventConditionResult&=condition;
}
/* MW_MAIN081B:6425..643C. Strictly less, one inclusive0..100 RNG draw. */
void MwEventTestRandomThreshold(const MwEventCommand *command)
{
    uint8_t threshold=command->operand.randomThreshold;
    campaignEvents.eventOperandCursor=command->next;
    MwCombineEventCondition(MwRandomUpTo(0x0064)<threshold);
}
/* MW_MAIN081B:643D..64BA. The expiry submonth scratch really stores MONTH. */
void MwEventTestScheduledDate(const MwEventCommand *command)
{
    uint16_t month=(uint8_t)(command->operand.date.monthCode-1);
    campaignEvents.eventScheduledMonthScratch=month;
    uint16_t submonth=(uint8_t)((uint8_t)(command->operand.date.submonthCode-1)<<1);
    campaignEvents.eventExpirySubmonthScratch=month;
    uint16_t year=command->operand.date.yearOffset;
    campaignEvents.eventOperandCursor=command->next;
    year+=EVENT_DATE_YEAR_BASE;
    campaignEvents.eventScheduledYearScratch=year;
    bool arrived=year<campaignCore.calendarYear || (year==campaignCore.calendarYear
        && (month<campaignCore.calendarMonthIndex || (month==campaignCore.calendarMonthIndex && submonth<=campaignEconomy.calendarSubmonthUnits)));
    if (arrived && campaignEvents.campaignEventClass==EVENT_CLASS_REPEATABLE)
    {
        month=campaignEvents.eventScheduledMonthScratch+6;
        if (month>=CAMPAIGN_MONTH_COUNT) {++campaignEvents.eventScheduledYearScratch;month-=CAMPAIGN_MONTH_COUNT;}
        submonth=campaignEvents.eventExpirySubmonthScratch;year=campaignEvents.eventScheduledYearScratch;
        arrived=year>campaignCore.calendarYear || (year==campaignCore.calendarYear
            && (month>campaignCore.calendarMonthIndex || (month==campaignCore.calendarMonthIndex && submonth>=campaignEconomy.calendarSubmonthUnits)));
    }
    MwCombineEventCondition(arrived);
}
/* MW_MAIN081B:64BB..64EC /652F..6560. Signed BYTE planet references use
 * complement then a one-based selector; zero becomes WORD FFFF. */
void MwEventTestCurrentPlanet(const MwEventCommand *command)
{
    int16_t selector=command->operand.planetSelector;
    campaignEvents.eventOperandCursor=command->next;
    uint16_t planet;
    if (selector>=0) planet=(uint16_t)(selector-1);
    else
    {
        uint16_t reference=(uint16_t)selector^UINT16_MAX;
        if (reference>=6) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: reference>=6"); /* Unrepresented adjacent reference aliases. */
        planet=campaignStartup.campaignEventPlanetSelectors[reference]-1;
    }
    MwCombineEventCondition(planet==campaignCore.currentPlanetIndex);
}
void MwEventTestDifferentPlanet(const MwEventCommand *command)
{
    int16_t selector=command->operand.planetSelector;
    campaignEvents.eventOperandCursor=command->next;
    uint16_t planet;
    if (selector>=0) planet=(uint16_t)(selector-1);
    else
    {
        uint16_t reference=(uint16_t)selector^UINT16_MAX;
        if (reference>=6) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: reference>=6");
        planet=campaignStartup.campaignEventPlanetSelectors[reference]-1;
    }
    MwCombineEventCondition(planet!=campaignCore.currentPlanetIndex);
}
/* MW_MAIN081B:64ED..6502. ANY nonzero seen flag, not dispatcher equality01. */
void MwEventTestSeenEvent(const MwEventCommand *command)
{
    uint8_t eventId=command->operand.seenEventId;
    campaignEvents.eventOperandCursor=command->next;
    if (eventId>=CAMPAIGN_EVENT_SEEN_FLAGS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: eventId>=CAMPAIGN_EVENT_SEEN_FLAGS");
    MwCombineEventCondition(campaignEvents.eventSeenFlags[eventId]!=0);
}
/* MW_MAIN081B:6503..652E. Great House standing is signed LOW WORD only. */
void MwEventTestControllingHouseStanding(const MwEventCommand *command)
{
    uint16_t house=command->operand.houseStanding.houseSelector;
    int16_t threshold=(int16_t)command->operand.houseStanding.thresholdHalf*2;
    campaignEvents.eventOperandCursor=command->next;
    --house;
    bool matches=false;
    if (house==campaignCore.currentPlanetHouseIndex)
    {
        if (house>=CAMPAIGN_HOUSE_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: house>=CAMPAIGN_HOUSE_COUNT");
        int16_t standing=campaignEconomy.houseFavorPoints[house]-campaignEconomy.houseGrievancePoints[house];
        matches=standing<threshold;
    }
    MwCombineEventCondition(matches);
}
/* MW_MAIN081B:6562..6568, falls through false/true combine labels. */
void MwEventTestPlayerCombatLoss(void)
{
    MwCombineEventCondition(campaignMechanics.rosterStatus[0]==1);
}
/* MW_MAIN081B:6588..65B0. Queue choice-dependent restart, do not reset now. */
void MwEventQueueCampaignRestart(void)
{
    if (campaignEvents.eventConditionResult!=1) return;
    campaignEvents.campaignRestartPending=1;
    if (!campaignStartMusic) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !campaignStartMusic");
    campaignStartMusic(campaignStartup.campaignMusicBuffer,0x2328);
}
/* MW_MAIN081B:65B1..662B. Later end month STILL tests submonth, as shipped. */
void MwEventRestrictToDateWindow(const MwEventCommand *command)
{
    if (campaignEvents.eventConditionResult)
    {
        uint16_t month=(uint8_t)(command->operand.dateWindow.start.monthCode-1);
        uint16_t submonth=(uint8_t)((uint8_t)(command->operand.dateWindow.start.submonthCode-1)<<1);
        uint16_t year=EVENT_DATE_YEAR_BASE+command->operand.dateWindow.start.yearOffset;
        bool reached=year<campaignCore.calendarYear || (year==campaignCore.calendarYear
            && (month<campaignCore.calendarMonthIndex || (month==campaignCore.calendarMonthIndex && submonth<=campaignEconomy.calendarSubmonthUnits)));
        if (reached)
        {
            month=(uint8_t)(command->operand.dateWindow.end.monthCode-1);
            submonth=(uint8_t)((uint8_t)(command->operand.dateWindow.end.submonthCode-1)<<1);
            year=EVENT_DATE_YEAR_BASE+command->operand.dateWindow.end.yearOffset;
            if (year>campaignCore.calendarYear || (year==campaignCore.calendarYear
                && month>=campaignCore.calendarMonthIndex && submonth>=campaignEconomy.calendarSubmonthUnits))
            {campaignEvents.eventConditionResult=1;campaignEvents.eventOperandCursor=command->next;return;}
        }
    }
    campaignEvents.eventOperandCursor=command->next;
    campaignEvents.eventConditionResult=0;
}
/* MW_MAIN081B:662D..665A. Exact01 gate, signed-result clamp, not affordability. */
void MwEventAdjustFunds(const MwEventCommand *command)
{
    if (campaignEvents.eventConditionResult==1)
    {
        uint16_t oldLow=campaignEconomy.campaignFundsLow;
        campaignEconomy.campaignFundsLow+=(uint16_t)command->operand.cashDelta;
        campaignEconomy.campaignFundsHigh+=(uint16_t)(command->operand.cashDelta>>NATIVE_WORD_BITS)+(campaignEconomy.campaignFundsLow<oldLow);
        if (campaignEconomy.campaignFundsHigh&NATIVE_WORD_HIGH_BIT)
        {campaignEconomy.campaignFundsLow=0;campaignEconomy.campaignFundsHigh=0;}
    }
    campaignEvents.eventOperandCursor=command->next;
}
/* MW_MAIN081B:665B..6660 /6661..6666. Independent of current accumulator. */
void MwEventUseAnd(void) {campaignEvents.eventCombineWithAnd=1;}
void MwEventUseOr(void) {campaignEvents.eventCombineWithAnd=0;}
