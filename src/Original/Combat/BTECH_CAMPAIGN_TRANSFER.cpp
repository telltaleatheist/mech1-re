/* Actual resident IO decode only. Runtime game code uses typed fields. */
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_record.h"
#include <string.h>

/* The campaign's own fields that combat reads, inside the block's campaign
 * prefix: the block has a fixed size and every read is inside it. */
enum
{
    TRANSFER_COMMANDER_NAME=0x003B,TRANSFER_PILOT_IDS=0x006F,TRANSFER_GUNNERY_TIERS=0x00AA,
    TRANSFER_PILOTING_TIERS=0x00B2,TRANSFER_OWNED_MECH_SLOTS=0x00C2
};

static uint16_t TransferWord(MwTransferWord word) { return (uint16_t)(word.low|((uint16_t)word.high<<8)); }

void BtechDecodeCampaignImport(const uint8_t bytes[CAMPAIGN_EXCHANGE_BYTES],BtechCampaignImport *company)
{
    MwCampaignTransferData block;
    memcpy(&block,bytes,sizeof(block));
    company->memoryTier=block.memoryTier;company->landscapeIndex=block.landscape;
    company->enemyHeavyCount=block.enemyHeavyCount;company->enemyMediumCount=block.enemyMediumCount;company->enemyLightCount=block.enemyLightCount;
    company->planetSelector=block.planetSelector;company->scenarioCode=block.scenarioCode;company->terrainDetailLevel=block.terrainDetail;
    company->soundDisabled=block.soundDisabled;
    memcpy(company->commanderName,bytes+TRANSFER_COMMANDER_NAME,sizeof(company->commanderName));
    for (uint16_t slot=0;slot<4;++slot)
    {
        company->pilotIds[slot]=MwReadWord(bytes+TRANSFER_PILOT_IDS+slot*2);
        company->gunneryTiers[slot]=MwReadWord(bytes+TRANSFER_GUNNERY_TIERS+slot*2);
        company->pilotingTiers[slot]=MwReadWord(bytes+TRANSFER_PILOTING_TIERS+slot*2);
        company->ownedMechSlots[slot]=MwReadWord(bytes+TRANSFER_OWNED_MECH_SLOTS+slot*2);
    }
    for (uint16_t slot=0;slot<12;++slot)
    {
        company->ownedUnitKinds[slot]=TransferWord(block.exchangeOwnedUnitKinds[slot]);
        company->ownedDamage[slot]=block.ownedDamage[slot];
        for (uint16_t group=0;group<6;++group)
            company->ammoQuantities[group][slot]=TransferWord(block.ammo[group][slot]);
    }
}
