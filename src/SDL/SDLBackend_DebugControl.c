/* Opt-in test instrument, not recovered game code. No state setters or game
 * calls: commands enter SDL's ordinary keyboard event path, telemetry only
 * reads canonical owners; a feed's random: token (a test aid, as a rig poke)
 * is the one writer. Files are private local IPC, never a network server. */
#include "SDLBackend.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_pose.h"
#include "mechwarrior/campaign_state.h"
#include "mechwarrior/campaign_ui.h"
#include "mechwarrior/campaign.h"
#include "mechwarrior/campaign_economy.h"
#include "mechwarrior/campaign_exchange.h"
#include <stdlib.h>
#include <string.h>

/* A jump can remain powered for tens of seconds at the configured CPU budget.
 * Keep real input bounded, but allow one continuous hold instead of introducing
 * artificial release/repress gaps into a flight comparison. */
enum { DEBUG_MAX_STEPS=32,DEBUG_MAX_KEYS=4,DEBUG_MAX_STEP_MS=30000,
    DEBUG_MAX_BATCH_MS=60000,DEBUG_POLL_MS=25,DEBUG_STATUS_MS=100,DEBUG_SETTLE_MS=200 };
typedef struct DebugKeyStep {uint32_t milliseconds;unsigned count;SDL_Scancode keys[DEBUG_MAX_KEYS];} DebugKeyStep;
static char debugDirectory[4096],commandPath[4096],statusPath[4096];
static DebugKeyStep steps[DEBUG_MAX_STEPS],held;
static unsigned stepCount,stepIndex;
static uint64_t activeId,completedId,nextStep,nextPoll,nextStatus;
static bool enabled,busy;
static const char *commandError="";
/* Campaign key reads since the current command began: the BIOS key word and
 * the random words as the game read it. The original draws once per pass of
 * its wait-for-a-key loop (081B:02FB), so these words, not the run's seed,
 * are what a replay must start each key from. */
enum { DEBUG_MAX_KEY_READS=32 };
static struct {uint16_t key,random0,random1;} keyReads[DEBUG_MAX_KEY_READS];
static unsigned keyReadCount;
static uint64_t inputPolls,totalKeyReads;

/* Poll-fed scripts (feed.txt), after crescent-hawks-rebuild's SNAP rig: each
 * key goes into the BIOS buffer only when the game is waiting for one, on a
 * blocking read or on the FEED_WAIT_POLLS-th empty "key waiting?" poll since
 * the last key read. A flush asks once and so never takes a key. Since the
 * original draws one random number per wait-loop pass (081B:02FB), feeding
 * at counted polls makes every draw independent of the host's speed. Tokens:
 *   SDL key names (Return, Escape, Up, A, F1...)  a key
 *   idle:N            N more empty polls before the next item
 *   @snap:NAME        capture NAME.* and NAME.state.json when the game next waits
 *   random:WWWWWWWW   set the two random words (hex, word 0 then word 1)
 * Status reports progress; the game then waits for keys as usual. */
enum { FEED_MAX_ITEMS=4096,FEED_MAX_BYTES=65536,FEED_WAIT_POLLS=4,FEED_NAME=48 };
typedef enum { FEED_KEY,FEED_IDLE,FEED_SNAP,FEED_RANDOM } FeedKind;
typedef struct { FeedKind kind;uint16_t word,word2;uint8_t shift;char name[FEED_NAME]; } FeedItem;
static FeedItem feed[FEED_MAX_ITEMS];
static unsigned feedCount,feedIndex,feedPolls,feedWait;
static bool feedActive;
static const char *feedError="";
static char feedPath[4096];
static void WriteStatus(const char *path);
static bool ParseFeed(char *text)
{
    feedCount=0;
    for (char *token=strtok(text," \t\r\n,");token;token=strtok(NULL," \t\r\n,"))
    {
        if (*token=='#') { strtok(NULL,"\n");continue; }
        if (feedCount==FEED_MAX_ITEMS) return false;
        FeedItem *item=&feed[feedCount];memset(item,0,sizeof(*item));
        char *end=NULL;
        if (!strncmp(token,"idle:",5))
        {
            unsigned long n=strtoul(token+5,&end,10);
            if (!*(token+5) || *end || n>1000000) return false;
            item->kind=FEED_IDLE;item->word=(uint16_t)(n>UINT16_MAX?UINT16_MAX:n);
        }
        else if (!strncmp(token,"@snap:",6))
        {
            const char *name=token+6;size_t length=strlen(name);
            if (!length || length>=FEED_NAME) return false;
            for (const char *ch=name;*ch;++ch)
                if (!((*ch>='a'&&*ch<='z')||(*ch>='A'&&*ch<='Z')||(*ch>='0'&&*ch<='9')||*ch=='_'||*ch=='-')) return false;
            item->kind=FEED_SNAP;memcpy(item->name,name,length+1);
        }
        else if (!strncmp(token,"random:",7))
        {
            unsigned long v=strtoul(token+7,&end,16);
            if (strlen(token+7)!=8 || *end) return false;
            item->kind=FEED_RANDOM;item->word=(uint16_t)(v>>16);item->word2=(uint16_t)v;
        }
        else
        {
            for (char *ch=token;*ch;++ch) if (*ch=='_') *ch=' ';
            SDL_Scancode scan=SDL_GetScancodeFromName(token);
            if (!scan || !MwSdlBiosKeyForScancode(scan,SDL_KMOD_NONE,&item->word,&item->shift)) return false;
            item->kind=FEED_KEY;
        }
        ++feedCount;
    }
    return feedCount>0;
}
static void LoadFeed(void)
{
    FILE *file=fopen(feedPath,"rb");if(!file)return;
    static char text[FEED_MAX_BYTES+1];
    size_t length=fread(text,1,FEED_MAX_BYTES+1,file);fclose(file);
    remove(feedPath); /* Consume our mailbox only. */
    text[length<=FEED_MAX_BYTES?length:0]=0;
    if (length>FEED_MAX_BYTES || !ParseFeed(text))
    {feedCount=0;feedActive=false;feedError="Rejected malformed or oversized feed";return;}
    feedIndex=0;feedPolls=0;feedWait=FEED_WAIT_POLLS;feedActive=true;feedError="";
}
static void Snap(const char *name)
{
    MwSdlPresentVideo(true);
    if (!MwSdlCapture(name)) {feedError="Snap failed (capture missing or name reused)";return;}
    char path[4200];
    /* The whole campaign as the save format encodes it: read-only from the
     * canonical owners, so traces can compare everything a save holds. */
    if (!mwSdlCombatActive && snprintf(path,sizeof(path),"%s/%s.exchange",MwSdlCaptureDirectory(),name)<(int)sizeof(path))
    {
        static uint8_t exchange[CAMPAIGN_EXCHANGE_BYTES];
        MwEncodeCampaignExchange(exchange);
        FILE *file=fopen(path,"wbx");
        if (!file || fwrite(exchange,1,sizeof(exchange),file)!=sizeof(exchange)) feedError="Snap failed (exchange)";
        if (file) fclose(file);
    }
    if (snprintf(path,sizeof(path),"%s/%s.state.json",MwSdlCaptureDirectory(),name)<(int)sizeof(path)) WriteStatus(path);
}
void MwSdlDebugNoteInputPoll(bool empty,bool blocking)
{
    if(!enabled)return;
    ++inputPolls;
    if(!feedActive || !empty || busy)return;
    ++feedPolls;
    while (feedActive && (blocking || feedPolls>=feedWait))
    {
        FeedItem *item=&feed[feedIndex];
        if (item->kind==FEED_IDLE) feedWait+=item->word;
        else if (item->kind==FEED_SNAP) Snap(item->name);
        else if (item->kind==FEED_RANDOM) {randomStateWord0=item->word;randomStateWord1=item->word2;}
        if (++feedIndex==feedCount) feedActive=false;
        if (item->kind==FEED_KEY) {MwSdlQueueBiosKey(item->word,item->shift);break;}
        if (item->kind==FEED_IDLE && !blocking) break; /* the extra polls start now */
    }
}
void MwSdlDebugNoteKeyRead(uint16_t key)
{
    if(!enabled)return;
    feedPolls=0;feedWait=FEED_WAIT_POLLS;
    ++totalKeyReads;
    if(keyReadCount<DEBUG_MAX_KEY_READS)
    {keyReads[keyReadCount].key=key;keyReads[keyReadCount].random0=randomStateWord0;keyReads[keyReadCount].random1=randomStateWord1;++keyReadCount;}
}

static void JsonString(FILE *file,const char *text)
{
    fputc('"',file);
    for (unsigned i=0;text && text[i] && i<512;++i)
    {
        unsigned char ch=(unsigned char)text[i];
        if (ch=='"' || ch=='\\') {fputc('\\',file);fputc(ch,file);}
        else if (ch<32) fprintf(file,"\\u%04x",ch);
        else fputc(ch,file);
    }
    fputc('"',file);
}
static void Words(FILE *file,const uint16_t *values,unsigned count)
{fputc('[',file);for(unsigned i=0;i<count;++i)fprintf(file,"%s%u",i?",":"",values[i]);fputc(']',file);}
static void Position(FILE *file,const BtechWorldObjectPrefix *object)
{
    fprintf(file,"\"position\":[%ld,%ld,%ld],\"orientation\":[%u,%u,%u],\"resource\":%u",
        (long)(int32_t)object->position.positionA,(long)(int32_t)object->position.positionB,
        (long)(int32_t)object->position.positionC,object->orientationA,
        object->orientationB,object->orientationC,object->resourceId);
}
static void WriteStatus(const char *path)
{
    /* Client retries concurrent/partial reads. Only this explicitly selected
     * private status file is replaced; screenshots remain exclusive files. */
    FILE *file=fopen(path,"w");if(!file)return;
    fprintf(file,"{\"protocol\":1,\"timeMs\":%llu,\"activeId\":%llu,\"completedId\":%llu,\"busy\":%s,\"error\":",
        (unsigned long long)SDL_GetTicks(),(unsigned long long)activeId,(unsigned long long)completedId,busy?"true":"false");
    JsonString(file,commandError);
    fprintf(file,",\"combat\":%s,\"campaign\":{\"planet\":%u,\"contractActive\":%u,\"missionCategory\":%u,\"scenario\":%u,\"destination\":%u,\"targetHouse\":%u,\"paymentThousands\":%u,\"result\":%u,\"selectedRow\":%u,\"random\":[%u,%u],\"funds\":%lu,\"months\":%u,\"menu\":[",
        mwSdlCombatActive?"true":"false",currentPlanetIndex,acceptedContractActive,missionCategoryOrMemoryTier,
        combatScenarioCode,combatPlanetSelector,acceptedContractTargetHouseIndex,
        acceptedContractPaymentThousands,combatTransitionResult,selectedUiRecordIndex,
        randomStateWord0,randomStateWord1,
        ((unsigned long)campaignFundsHigh<<16)|campaignFundsLow,elapsedBillingMonths);
    if (!mwSdlCombatActive && uiMenu)
        for(unsigned i=0;i<uiMenu->recordCount;++i)
        {
            const MwUiRecord *row=&uiMenu->records[i];
            fprintf(file,"%s{\"index\":%u,\"enabled\":%u,\"callback\":%u,\"picture\":%u,\"links\":[%u,%u,%u,%u],\"label\":",
                i?",":"",i,row->enabled,row->activateCodeOffset,row->pictureIndex,
                row->upRecordIndex,row->downRecordIndex,row->leftRecordIndex,row->rightRecordIndex);
            JsonString(file,row->textRecord?row->text:"");fputc('}',file);
        }
    fprintf(file,"]},\"input\":{\"polls\":%llu,\"reads\":%llu,\"commandReads\":[",(unsigned long long)inputPolls,(unsigned long long)totalKeyReads);
    for(unsigned i=0;i<keyReadCount;++i)
        fprintf(file,"%s[%u,%u,%u]",i?",":"",keyReads[i].key,keyReads[i].random0,keyReads[i].random1);
    fprintf(file,"]},\"feed\":{\"items\":%u,\"next\":%u,\"done\":%s,\"error\":",feedCount,feedIndex,feedActive||!feedCount?"false":"true");
    JsonString(file,feedError);
    fprintf(file,"},\"tick\":%u,\"screen\":%u,\"target\":%u,\"aimY\":%u,\"requestedStep\":%d,\"objectiveRules\":[%u,%u],\"units\":[",
        combatTickCounter,combatDispatchState,playerTargetSlot,aimReticleScreenY,
        (int16_t)requestedPlayerForwardStepScale,sideObjectiveRules[0],sideObjectiveRules[1]);
    bool first=true;
    if(mwSdlCombatActive) for(unsigned slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {
        const BtechUnitBinding *binding=&unitBindings[slot];
        const BtechMechState *mech=binding->mech;
        const BtechPooledWorldObject *body=binding->positionedLeftLeg;
        if(!mech || !body)continue;
        fprintf(file,"%s{\"slot\":%u,\"side\":%u,\"chassis\":%u,\"step\":%d,",first?"":",",slot,
            unitControls[slot].sideIndex,mech->templateIndex,body->forwardStepScale);first=false;
        Position(file,&body->object.prefix);
        fprintf(file,",\"movementMode\":%u,\"verticalStep\":%d,\"jumpCharge\":%u",
            unitControls[slot].movementMode,body->positionSteps[2],mech->jumpCharge);
        if (!slot && playerPoseObject)
        {
            fputs(",\"camera\":{",file);
            Position(file,&playerPoseObject->object.prefix);
            fputc('}',file);
        }
        fprintf(file,",\"torsoOrientation\":[%u,%u,%u],\"heat\":%u,\"engineHits\":%u,\"gyroHits\":%u,\"cockpitDisabled\":%u,\"armor\":",
            binding->torso?binding->torso->object.prefix.orientationA:0,
            binding->torso?binding->torso->object.prefix.orientationB:0,
            binding->torso?binding->torso->object.prefix.orientationC:0,
            mech->heatUnits,mech->engineHits,mech->gyroHits,mech->cockpitDisabled);
        Words(file,mech->currentArmor,COMBAT_ARMOR_LOCATIONS);fputs(",\"internal\":",file);
        Words(file,mech->currentInternalStructure,COMBAT_BODY_LOCATIONS);fputs(",\"ammo\":",file);
        Words(file,mech->ammoSalvosRemaining,COMBAT_AMMO_FAMILIES);fputs(",\"weapons\":[",file);
        for(unsigned mount=0;mount<COMBAT_WEAPON_MOUNTS;++mount)
            fprintf(file,"%s{\"operational\":%u,\"reload\":%u}",mount?",":"",mech->weapons[mount].operational,mech->weapons[mount].reloadTicksRemaining);
        fputs("]}",file);
    }
    fputs("],\"objective\":",file);
    if(mwSdlCombatActive && specialTargetObject)
    {fputc('{',file);Position(file,&specialTargetObject->object.prefix);fprintf(file,",\"damageRemaining\":%u}",specialTargetDamageRemaining);}
    else fputs("null",file);
    fputs(",\"worldObjects\":[",file);first=true;
    if(mwSdlCombatActive) for(unsigned i=0;i<worldObjectCount && i<COMBAT_WORLD_OBJECTS;++i)
    {if(!worldObjects[i])continue;fprintf(file,"%s{\"index\":%u,",first?"":",",i);first=false;Position(file,worldObjects[i]);fputc('}',file);}
    fprintf(file,"],\"radar\":{\"rangeShift\":%u,\"headingMode\":%u,\"origin\":[%u,%u],\"markers\":[",
        radarRangeShift,headingRadarSelected,radarMapOriginX,radarMapOriginY);
    unsigned page=radarCachePage;
    if(mwSdlCombatActive && page<COMBAT_RADAR_CACHE_PAGES)
        for(unsigned i=0;i<radarMarkerCounts[page] && i<COMBAT_RADAR_MARKERS;++i)
            fprintf(file,"%s[%u,%u]",i?",":"",radarMarkerX[i][page],radarMarkerY[i][page]);
    fputs("]}}\n",file);fclose(file);
}
static void PublishStatus(void) {WriteStatus(statusPath);}
static void SendStep(const DebugKeyStep *step,bool down)
{
    SDL_Keymod modifiers=SDL_KMOD_NONE;
    for(unsigned i=0;i<step->count;++i)
    {
        if(step->keys[i]==SDL_SCANCODE_LSHIFT || step->keys[i]==SDL_SCANCODE_RSHIFT)modifiers|=SDL_KMOD_SHIFT;
        if(step->keys[i]==SDL_SCANCODE_LCTRL || step->keys[i]==SDL_SCANCODE_RCTRL)modifiers|=SDL_KMOD_CTRL;
        if(step->keys[i]==SDL_SCANCODE_LALT || step->keys[i]==SDL_SCANCODE_RALT)modifiers|=SDL_KMOD_ALT;
    }
    for(unsigned i=0;i<step->count;++i)
    {
        SDL_Event event={0};event.type=down?SDL_EVENT_KEY_DOWN:SDL_EVENT_KEY_UP;
        event.key.scancode=step->keys[down?i:step->count-1-i];event.key.mod=modifiers;
        event.key.key=SDL_GetKeyFromScancode(event.key.scancode,modifiers,false);event.key.down=down;
        SDL_PushEvent(&event);
    }
}
void MwSdlInitializeDebugControl(void)
{
    const char *directory=getenv("MECHWARRIOR_DEBUG_CONTROL_DIRECTORY");
    enabled=false;busy=false;held.count=0;activeId=completedId=nextPoll=nextStatus=0;
    keyReadCount=0;inputPolls=totalKeyReads=0;
    if(!directory || !*directory)return;
    if(snprintf(debugDirectory,sizeof(debugDirectory),"%s",directory)>=(int)sizeof(debugDirectory)
        || snprintf(commandPath,sizeof(commandPath),"%s/command.txt",directory)>=(int)sizeof(commandPath)
        || snprintf(statusPath,sizeof(statusPath),"%s/status.json",directory)>=(int)sizeof(statusPath)
        || snprintf(feedPath,sizeof(feedPath),"%s/feed.txt",directory)>=(int)sizeof(feedPath))return;
    feedActive=false;feedCount=0;feedError="";
    enabled=true;commandError="";
    LoadFeed(); /* A feed present at start covers every poll from the first. */
}
/* Golden traces need a repeatable start: the campaign mixes the BIOS tick
 * count into its random words once at startup (081B:0C0A). With debug control
 * on, MECHWARRIOR_DEBUG_BIOS_TICKS sets the clock that one read sees, as if
 * the DOS clock had been set before the run. Game code is untouched. */
bool MwSdlDebugPinnedBiosTicks(uint32_t *ticks)
{
    const char *control=getenv("MECHWARRIOR_DEBUG_CONTROL_DIRECTORY");
    const char *text=getenv("MECHWARRIOR_DEBUG_BIOS_TICKS");
    if(!control || !*control || !text || !*text)return false;
    char *end=NULL;unsigned long value=strtoul(text,&end,0);
    if(!end || *end)return false;
    *ticks=(uint32_t)value;return true;
}
void MwSdlPollDebugControl(void)
{
    if(!enabled)return;
    uint64_t now=SDL_GetTicks();if(now<nextPoll)return;nextPoll=now+DEBUG_POLL_MS;
    if(busy && now>=nextStep)
    {
        SendStep(&held,false);held.count=0;
        if(stepIndex<stepCount)
        {held=steps[stepIndex++];SendStep(&held,true);nextStep=now+held.milliseconds;}
        else
        {
            busy=false;completedId=activeId;
            char stem[64];snprintf(stem,sizeof(stem),"debug-%llu",(unsigned long long)completedId);
            if(!MwSdlCapture(stem))commandError="Screenshot not available";
        }
    }
    if(!busy && !feedActive) LoadFeed();
    if(!busy && !feedActive)
    {
        FILE *file=fopen(commandPath,"r");
        if(file)
        {
            unsigned long long id=0;unsigned count=0,total=0;char line[256];bool valid=true;
            valid=fgets(line,sizeof(line),file) && sscanf(line,"%llu %u",&id,&count)==2 && id>activeId && count<=DEBUG_MAX_STEPS-1;
            for(unsigned i=0;valid && i<count;++i)
            {
                if(!fgets(line,sizeof(line),file)){valid=false;break;}
                char *token=strtok(line," \t\r\n"),*end=NULL;
                unsigned long duration=token?strtoul(token,&end,10):0;
                if(!token || *end || duration>DEBUG_MAX_STEP_MS || duration<25){valid=false;break;}
                steps[i]=(DebugKeyStep){(uint32_t)duration,0,{0}};total+=(unsigned)duration;
                while((token=strtok(NULL," \t\r\n")))
                {
                    for(char *ch=token;*ch;++ch)if(*ch=='_')*ch=' ';
                    SDL_Scancode scan=SDL_GetScancodeFromName(token);
                    if(!scan || steps[i].count==DEBUG_MAX_KEYS){valid=false;break;}
                    steps[i].keys[steps[i].count++]=scan;
                }
            }
            fclose(file);
            /* Consume our mailbox only, never an arbitrary user path. */
            remove(commandPath);
            if(valid && total<=DEBUG_MAX_BATCH_MS)
            {activeId=id;keyReadCount=0;stepCount=count+1;steps[count]=(DebugKeyStep){DEBUG_SETTLE_MS,0,{0}};stepIndex=0;busy=true;nextStep=now;commandError="";}
            else commandError="Rejected malformed, stale, oversized or unknown-key command";
        }
    }
    if(now>=nextStatus){nextStatus=now+DEBUG_STATUS_MS;PublishStatus();}
}
void MwSdlShutdownDebugControl(void)
{if(enabled){SendStep(&held,false);held.count=0;busy=false;PublishStatus();enabled=false;}}
