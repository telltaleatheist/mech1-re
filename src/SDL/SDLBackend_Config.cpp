#include "SDLBackend.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

MwHostSoundSystem mwSdlSoundSystem=MW_SOUND_SPEAKER;
char mwSdlMt32ControlRom[4096],mwSdlMt32PcmRom[4096],mwSdlGameFolder[4096];
enum { HOST_MIN_COMBAT_TICK_MS=20,HOST_MAX_COMBAT_TICK_MS=500 };
static char userConfigurationPath[4096];

static bool ParseWhole(const char *value,unsigned long minimum,unsigned long maximum,unsigned long *result)
{
    if (!isdigit((unsigned char)*value)) return false;
    char *end;errno=0;unsigned long number=strtoul(value,&end,10);
    if (errno || *end || number<minimum || number>maximum) return false;
    *result=number;return true;
}
/* Parse into temporary configuration: an invalid trailing key must not change
 * the combat tick or partially select an audio device. ROM paths are
 * resolved relative to this INI, not the game's working/save directory. */
bool MwSdlLoadTimingConfiguration(const char *path,bool optional)
{
    FILE *file=fopen(path,"r");
    if (!file) return optional && errno==ENOENT;
    char line[8192],controlRom[4096],pcmRom[4096],gameFolder[4096];
    strcpy(controlRom,mwSdlMt32ControlRom);strcpy(pcmRom,mwSdlMt32PcmRom);strcpy(gameFolder,mwSdlGameFolder);
    uint32_t tickMs=mwSdlCombatTickMs;uint8_t speed=mwSdlCombatSpeed;
    MwHostSoundSystem soundSystem=mwSdlSoundSystem;
    bool valid=true;
    while (fgets(line,sizeof(line),file))
    {
        char *key=line;
        while (isspace((unsigned char)*key)) ++key;
        if (!*key || *key==';' || *key=='#') continue;
        char *value=strchr(key,'=');
        if (!value) {valid=false;break;}
        char *keyEnd=value;
        while (keyEnd>key && isspace((unsigned char)keyEnd[-1])) --keyEnd;
        *keyEnd=0;++value;
        while (isspace((unsigned char)*value)) ++value;
        char *end=value+strlen(value);
        while (end>value && isspace((unsigned char)end[-1])) --end;
        *end=0;
        unsigned long number;
        if (!strcmp(key,"combat_tick_ms"))
        {
            if (!ParseWhole(value,HOST_MIN_COMBAT_TICK_MS,HOST_MAX_COMBAT_TICK_MS,&number)) {valid=false;break;}
            tickMs=(uint32_t)number;
        }
        else if (!strcmp(key,"combat_speed"))
        {
            if (!ParseWhole(value,MW_COMBAT_SPEED_MIN,MW_COMBAT_SPEED_MAX,&number)) {valid=false;break;}
            speed=(uint8_t)number;
        }
        else if (!strcmp(key,"game_folder"))
        {
            if (strlen(value)>=sizeof(gameFolder)) {valid=false;break;}
            strcpy(gameFolder,value);
        }
        else if (!strcmp(key,"cpu_cycles_per_ms")) /* The old cycle model's key: ignored. */
            fprintf(stderr,"%s: cpu_cycles_per_ms is no longer used; combat_tick_ms sets the combat speed.\n",path);
        else if (!strcmp(key,"sound_system"))
        {
            if (!strcmp(value,"speaker")) soundSystem=MW_SOUND_SPEAKER;
            else if (!strcmp(value,"mt32")) soundSystem=MW_SOUND_MT32;
            else if (!strcmp(value,"adlib")) soundSystem=MW_SOUND_ADLIB;
            else {valid=false;break;}
        }
        else if (!strcmp(key,"mt32_control_rom") || !strcmp(key,"mt32_pcm_rom"))
        {
            char *destination=!strcmp(key,"mt32_control_rom")?controlRom:pcmRom;
            if (value[0]=='"' && end>value+1 && end[-1]=='"') {*--end=0;++value;}
            if (!*value || value[0]=='/' || value[0]=='\\' || (strlen(value)>1 && value[1]==':'))
            {
                if (strlen(value)>=4096) {valid=false;break;}
                strcpy(destination,value);
            }
            else
            {
                const char *slash=strrchr(path,'/'),*backslash=strrchr(path,'\\');
                if (backslash && (!slash || backslash>slash)) slash=backslash;
                size_t prefix=slash?(size_t)(slash-path)+1:0;
                if (prefix+strlen(value)>=4096) {valid=false;break;}
                memcpy(destination,path,prefix);strcpy(destination+prefix,value);
            }
        }
        else {valid=false;break;}
    }
    if (ferror(file)) valid=false;
    if (fclose(file)) valid=false;
    if (valid)
    {
        mwSdlCombatTickMs=tickMs;mwSdlCombatSpeed=speed;mwSdlSoundSystem=soundSystem;
        strcpy(mwSdlMt32ControlRom,controlRom);strcpy(mwSdlMt32PcmRom,pcmRom);strcpy(mwSdlGameFolder,gameFolder);
    }
    return valid;
}

/* The player's MechWarrior.ini, in the write directory beside the saves. It
 * is optional and holds only the keys the player set (or changed in play). */
bool MwSdlLoadUserConfiguration(void)
{
    char directory[4096];
    if (!MwSdlResolveWriteDirectory(directory,sizeof(directory))) return false;
    size_t length=strlen(directory);
    const char *separator=length && (directory[length-1]=='/' || directory[length-1]=='\\')?"":"/";
    if (snprintf(userConfigurationPath,sizeof(userConfigurationPath),"%s%sMechWarrior.ini",directory,separator)>=(int)sizeof(userConfigurationPath))
    {userConfigurationPath[0]=0;return false;}
    return MwSdlLoadTimingConfiguration(userConfigurationPath,true);
}
void MwSdlSaveUserSetting(const char *key,unsigned value)
{char text[16];snprintf(text,sizeof(text),"%u",value);MwSdlSaveUserText(key,text);}
/* Replace (or add) one key's line, keeping the player's other lines. */
void MwSdlSaveUserText(const char *key,const char *value)
{
    if (strchr(value,'\n') || strchr(value,'\r')) return;
    if (!userConfigurationPath[0]) return;
    static char text[65536];size_t used=0;
    size_t keyLength=strlen(key);
    bool replaced=false;
    FILE *file=fopen(userConfigurationPath,"r");
    if (file)
    {
        char line[8192];
        while (fgets(line,sizeof(line),file))
        {
            const char *start=line;
            while (isspace((unsigned char)*start)) ++start;
            const char *after=start+keyLength;
            bool match=!strncmp(start,key,keyLength);
            while (match && (*after==' ' || *after=='\t')) ++after;
            int written=match && *after=='='?snprintf(text+used,sizeof(text)-used,"%s=%s\n",key,value)
                :snprintf(text+used,sizeof(text)-used,"%s",line);
            if (match && *after=='=') replaced=true;
            if (written<0 || (size_t)written>=sizeof(text)-used) {fclose(file);fprintf(stderr,"%s: too long to update.\n",userConfigurationPath);return;}
            used+=(size_t)written;
        }
        fclose(file);
    }
    if (!replaced)
    {
        int written=snprintf(text+used,sizeof(text)-used,"%s=%s\n",key,value);
        if (written<0 || (size_t)written>=sizeof(text)-used) return;
        used+=(size_t)written;
    }
    file=fopen(userConfigurationPath,"w");
    if (!file || fwrite(text,1,used,file)!=used || fclose(file))
        fprintf(stderr,"Could not save %s to %s.\n",key,userConfigurationPath);
}
