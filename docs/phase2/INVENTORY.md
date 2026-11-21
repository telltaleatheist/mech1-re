# Phase 2 inventory

Generated from `src/Original/Memory` (every file-scope item, with the first native offset its importer reads; `-` means the item is runtime state with no import). The groups follow `docs/phase2/PLAN.md`. Names and offsets only: no MW1 text.

| Group | Items |
|---|---|
| Extracted: the original's creative content (the story file, first run) | 63 |
| Ours: UI words and system messages (data/text/interface.json) | 104 |
| Ours: screen layout, panels and menus (data/ui/*.json) | 58 |
| Ours: rules and constants (data/rules/*.json) | 90 |
| Ours: the new campaign's starting state (data/rules/new_campaign.json) | 99 |
| Dropped | 26 |
| Code: runtime state and renderer tables (no file) | 1086 |

## Extracted: the original's creative content (the story file, first run)

| File | Item | Native offset |
|---|---|---|
| BTECH_304E.cpp | `combatDefaultCommanderNameBytes` | 0x0978 |
| BTECH_304E_ADLIB.cpp | `combatAdlibDefaultInstruments` | 0x2BA0 |
| BTECH_304E_COCKPIT.cpp | `mechTemplateNames` | 0x2045 |
| BTECH_304E_COCKPIT.cpp | `missionTitleText` | 0x09F0 |
| BTECH_304E_COCKPIT.cpp | `pilotDefinitionNameDirectory` | - |
| BTECH_304E_COCKPIT.cpp | `pilotDefinitionNames` | 0x20AD |
| BTECH_304E_SOUND.cpp | `musicSequences` | 0x7D8A |
| MW_MAIN_1090.cpp | `afterActionGeneralFailure` | 0x159B |
| MW_MAIN_1090.cpp | `afterActionGeneralSuccess` | 0x1742 |
| MW_MAIN_1090.cpp | `afterActionKuritaFailure` | 0x149B |
| MW_MAIN_1090.cpp | `afterActionKuritaSuccess` | 0x1674 |
| MW_MAIN_1090.cpp | `afterActionLiaoFailure` | 0x1528 |
| MW_MAIN_1090.cpp | `afterActionLiaoSuccess` | 0x16D6 |
| MW_MAIN_1090.cpp | `alphabeticPlanetRecordOffsets` | 0x6694 |
| MW_MAIN_1090.cpp | `campaignDefaultCommanderNameBytes` | 0x5CE2 |
| MW_MAIN_1090.cpp | `campaignEventPlanetSelectors` | 0x0B46 |
| MW_MAIN_1090.cpp | `contractNarrative` | 0x4890 |
| MW_MAIN_1090.cpp | `contractOfficePlanetGroupStorage` | 0x4B8A |
| MW_MAIN_1090.cpp | `darkWingAttackFailedText` | 0x1BBC |
| MW_MAIN_1090.cpp | `darkWingVictoryText` | 0x1A37 |
| MW_MAIN_1090.cpp | `deploymentUneventfulText` | 0x140D |
| MW_MAIN_1090.cpp | `eventPlanetChoicePool0` | 0x8FDA |
| MW_MAIN_1090.cpp | `eventPlanetChoicePool1` | 0x8FE2 |
| MW_MAIN_1090.cpp | `eventPlanetChoicePool2` | 0x8FE5 |
| MW_MAIN_1090.cpp | `eventPlanetChoicePool3` | 0x8FE9 |
| MW_MAIN_1090.cpp | `eventPlanetChoicePool4` | 0x8FEC |
| MW_MAIN_1090.cpp | `eventPlanetChoicePool5` | 0x8FF0 |
| MW_MAIN_1090.cpp | `houseContractNames` | 0x4A69 |
| MW_MAIN_1090.cpp | `houseContractRefusalText` | 0x4C87 |
| MW_MAIN_1090.cpp | `houseMissionPlanetTableOffsets` | 0x8E90 |
| MW_MAIN_1090.cpp | `introFifthLeftText` | 0x1D8E |
| MW_MAIN_1090.cpp | `introFifthRightText` | 0x1DD8 |
| MW_MAIN_1090.cpp | `introFirstLeftText` | 0x1C33 |
| MW_MAIN_1090.cpp | `introFirstRightText` | 0x1C57 |
| MW_MAIN_1090.cpp | `introFourthLeftText` | 0x1D53 |
| MW_MAIN_1090.cpp | `introFourthRightText` | 0x1D6F |
| MW_MAIN_1090.cpp | `introSecondLeftText` | 0x1C80 |
| MW_MAIN_1090.cpp | `introSecondRightText` | 0x1CA9 |
| MW_MAIN_1090.cpp | `introSeventhLeftText` | 0x1E40 |
| MW_MAIN_1090.cpp | `introSeventhRightText` | 0x1EB8 |
| MW_MAIN_1090.cpp | `introSixthLeftText` | 0x1DF7 |
| MW_MAIN_1090.cpp | `introSixthRightText` | 0x1E18 |
| MW_MAIN_1090.cpp | `introThirdLeftText` | 0x1CCF |
| MW_MAIN_1090.cpp | `introThirdRightText` | 0x1D1F |
| MW_MAIN_1090.cpp | `missionDescriptionListEntries` | 0x8A25 |
| MW_MAIN_1090.cpp | `missionDescriptionListOffsetsByCategory` | 0x8A1B |
| MW_MAIN_1090.cpp | `missionDescriptionMaximumIndicesByCategory` | 0x8A16 |
| MW_MAIN_1090.cpp | `missionDescriptionOffsets` | 0x094A |
| MW_MAIN_1090.cpp | `missionDescriptionStorage` | 0x8A69 |
| MW_MAIN_1090.cpp | `pilotDefinitionOffsets` | 0x9121 |
| MW_MAIN_1090.cpp | `pilotDefinitionStorage` | 0x9177 |
| MW_MAIN_1090.cpp | `pilotDefinitions` | - |
| MW_MAIN_1090.cpp | `pilotIdentityStorage` | 0x04BF |
| MW_MAIN_1090.cpp | `planetRecordOffsets` | 0x6570 |
| MW_MAIN_1090.cpp | `planetRecordStorage` | 0x67B6 |
| MW_MAIN_1090.cpp | `settlementNoPaymentText` | 0x19A2 |
| MW_MAIN_1090.cpp | `settlementPerformanceDeductionText` | 0x1834 |
| MW_MAIN_1090.cpp | `settlementSupplyDeductionText` | 0x178C |
| MW_MAIN_1090.cpp | `settlementTransportDeductionText` | 0x18E0 |
| MW_MAIN_1090.cpp | `shopUnitNames` | 0x2DB5 |
| MW_MAIN_1090.cpp | `unitDisplayNames` | 0x2E70 |
| MW_MAIN_1090_EVENTS.cpp | `the event script (records, conditions, actions)` | MwImportCampaignEventData |
| MW_MAIN_1AA5.cpp | `eventTextStorage / eventTextRecords` | 1AA5:0000 |

## Ours: UI words and system messages (data/text/interface.json)

| File | Item | Native offset |
|---|---|---|
| BTECH_304E_COCKPIT.cpp | `actuatorLabels` | 0x046C |
| BTECH_304E_COCKPIT.cpp | `automaticWeaponsLabel` | 0x0059 |
| BTECH_304E_COCKPIT.cpp | `bodyLocationLabels` | 0x038E |
| BTECH_304E_COCKPIT.cpp | `heatSinksOfText` | 0x0566 |
| BTECH_304E_COCKPIT.cpp | `inspectionConditionLabels` | 0x03A6 |
| BTECH_304E_COCKPIT.cpp | `inspectionSubsystemLabels` | 0x0520 |
| BTECH_304E_COCKPIT.cpp | `inspectionWeaponHeading` | 0x057A |
| BTECH_304E_COCKPIT.cpp | `jumpJetsOfText` | 0x0577 |
| BTECH_304E_COCKPIT.cpp | `jumpJetsText` | 0x0569 |
| BTECH_304E_COCKPIT.cpp | `pausedText` | 0x0081 |
| BTECH_304E_COCKPIT.cpp | `radarRangeText` | 0x23EA |
| BTECH_304E_COCKPIT.cpp | `reactorShutdownText` | 0x005D |
| BTECH_304E_COCKPIT.cpp | `retreatChoiceText` | 0x083E |
| BTECH_304E_COCKPIT.cpp | `retreatQuestionText` | 0x082F |
| BTECH_304E_COCKPIT.cpp | `simulationText` | 0x0076 |
| BTECH_304E_COCKPIT.cpp | `subsystemLampLabels` | 0x23BE |
| BTECH_304E_COCKPIT.cpp | `weaponLabelText` | 0x2004 |
| BTECH_304E_COCKPIT.cpp | `weaponRangeAndAmmoText` | 0x2410 |
| BTECH_304E_COCKPIT.cpp | `zoomLabel` | 0x0052 |
| MW_MAIN_1090.cpp | `acceptContractCaption` | 0x4B35 |
| MW_MAIN_1090.cpp | `afterActionCrewHeading` | 0x11DF |
| MW_MAIN_1090.cpp | `afterActionMechHeading` | 0x11EC |
| MW_MAIN_1090.cpp | `afterActionPaymentHeading` | 0x1202 |
| MW_MAIN_1090.cpp | `afterActionPercentSuffix` | 0x120B |
| MW_MAIN_1090.cpp | `afterActionPilotStatusText` | 0x120D |
| MW_MAIN_1090.cpp | `afterActionSalvageHeading` | 0x11F9 |
| MW_MAIN_1090.cpp | `ammoBayFullText` | 0x315B |
| MW_MAIN_1090.cpp | `ammoFamilyDisplayNames` | 0x2F78 |
| MW_MAIN_1090.cpp | `assignedMechUnreadyText` | 0x0FF7 |
| MW_MAIN_1090.cpp | `barMenuLabels` | 0x9013 |
| MW_MAIN_1090.cpp | `calendarMonthText` | 0x4AAC |
| MW_MAIN_1090.cpp | `campaignDamageTierLabels` | 0x2B23 |
| MW_MAIN_1090.cpp | `cannotAffordRepairText` | 0x2BB0 |
| MW_MAIN_1090.cpp | `commanderNeedsMechText` | 0x0FC0 |
| MW_MAIN_1090.cpp | `commanderProfileLabels` | 0x2283 |
| MW_MAIN_1090.cpp | `commanderProfileName` | 0x21C2 |
| MW_MAIN_1090.cpp | `commanderReputationLabels` | 0x23D0 |
| MW_MAIN_1090.cpp | `companyMenuLabels` | 0x21EB |
| MW_MAIN_1090.cpp | `contractAcceptedText` | 0x4C6C |
| MW_MAIN_1090.cpp | `contractCancelledText` | 0x1031 |
| MW_MAIN_1090.cpp | `crewLabels` | 0x2348 |
| MW_MAIN_1090.cpp | `dealerPurchaseLimitText` | 0x258A |
| MW_MAIN_1090.cpp | `dropShipStatusLabels` | 0x2D1C |
| MW_MAIN_1090.cpp | `fullFourPilotRosterText` | 0x9C5A |
| MW_MAIN_1090.cpp | `fullThreePilotRosterText` | 0x9C8D |
| MW_MAIN_1090.cpp | `marketBodyCondition` | 0x4685 |
| MW_MAIN_1090.cpp | `marketComponentDone` | 0x47F5 |
| MW_MAIN_1090.cpp | `marketComponentTitle` | 0x4536 |
| MW_MAIN_1090.cpp | `marketDropShipStatusLabels` | 0x4459 |
| MW_MAIN_1090.cpp | `marketListDoneText` | 0x3049 |
| MW_MAIN_1090.cpp | `marketListHeading` | 0x2922 |
| MW_MAIN_1090.cpp | `marketMechStatusLabels` | 0x439B |
| MW_MAIN_1090.cpp | `marketWeaponHeading` | 0x46A2 |
| MW_MAIN_1090.cpp | `mechComplexLabels` | 0x25E4 |
| MW_MAIN_1090.cpp | `mechNonfunctionalTextBytes` | 0x2B55 |
| MW_MAIN_1090.cpp | `mechStatusLabels` | 0x2C5B |
| MW_MAIN_1090.cpp | `negotiationTerminatedText` | 0x4C25 |
| MW_MAIN_1090.cpp | `noAvailableRecruitsText` | 0x9C39 |
| MW_MAIN_1090.cpp | `noContractsText` | 0x4BFA |
| MW_MAIN_1090.cpp | `noMarketMechsText` | 0x31BB |
| MW_MAIN_1090.cpp | `noRepairWorkText` | 0x2BE7 |
| MW_MAIN_1090.cpp | `ownedListDoneText` | 0x302F |
| MW_MAIN_1090.cpp | `ownedMechCapacityText` | 0x4347 |
| MW_MAIN_1090.cpp | `ownedUnitListHeading` | 0x2696 |
| MW_MAIN_1090.cpp | `pilotSkillLabels` | 0x2452 |
| MW_MAIN_1090.cpp | `positionedText` | 0x55EC |
| MW_MAIN_1090.cpp | `purchaseInsufficientFundsText` | 0x4330 |
| MW_MAIN_1090.cpp | `rearmConfirmationLabels` | 0x312B |
| MW_MAIN_1090.cpp | `recruitInterviewLabels` | 0x9091 |
| MW_MAIN_1090.cpp | `recruitStatLabels` | 0x9106 |
| MW_MAIN_1090.cpp | `repairBodyCondition` | 0x33A8 |
| MW_MAIN_1090.cpp | `repairCompleteText` | 0x2C01 |
| MW_MAIN_1090.cpp | `repairFailedText` | 0x3CB5 |
| MW_MAIN_1090.cpp | `repairFooter` | 0x33C0 |
| MW_MAIN_1090.cpp | `repairIncompleteText` | 0x2C21 |
| MW_MAIN_1090.cpp | `repairJumpJetCountTemplateBytes` | 0x36A5 |
| MW_MAIN_1090.cpp | `repairNoJumpJetsTextBytes` | 0x36A0 |
| MW_MAIN_1090.cpp | `repairTitle` | 0x3254 |
| MW_MAIN_1090.cpp | `reserveAmmoBuyLabel` | 0x413A |
| MW_MAIN_1090.cpp | `reserveAmmoSellLabel` | 0x413F |
| MW_MAIN_1090.cpp | `saleConfirmationLabels` | 0x3090 |
| MW_MAIN_1090.cpp | `submitContractCaption` | 0x4B35 |
| MW_MAIN_1090.cpp | `travelBrowseLabel` | 0x5C75 |
| MW_MAIN_1090.cpp | `travelConfirmLabel` | 0x5C17 |
| MW_MAIN_1090.cpp | `travelCostLabel` | 0x5BF9 |
| MW_MAIN_1090.cpp | `travelDestinationHeaderLabels` | 0x5C35 |
| MW_MAIN_1090.cpp | `travelHouseBrowserLabels` | 0x5A0D |
| MW_MAIN_1090.cpp | `travelHousePlanetCancelLabel` | 0x57FD |
| MW_MAIN_1090.cpp | `travelHouseSearchCaption` | 0x5534 |
| MW_MAIN_1090.cpp | `travelInsufficientFundsText` | 0x5B5A |
| MW_MAIN_1090.cpp | `travelLandscapeLabels` | 0x5CA6 |
| MW_MAIN_1090.cpp | `travelPopulationSuffix` | 0x5B83 |
| MW_MAIN_1090.cpp | `unassignedMechWarriorText` | 0x3024 |
| MW_MAIN_1090_OPTIONS.cpp | `audioOptionLabels` | 0xA071 |
| MW_MAIN_1090_OPTIONS.cpp | `loadDiskErrorText` | 0xA0E3 |
| MW_MAIN_1090_OPTIONS.cpp | `newsAtEnd` | 0x2562 |
| MW_MAIN_1090_OPTIONS.cpp | `newsAtStart` | 0x2562 |
| MW_MAIN_1090_OPTIONS.cpp | `newsBoundaryText` | 0x2564 |
| MW_MAIN_1090_OPTIONS.cpp | `restoreCancel` | 0x9F6C |
| MW_MAIN_1090_OPTIONS.cpp | `restoreHeading` | 0x9E79 |
| MW_MAIN_1090_OPTIONS.cpp | `saveDiskErrorText` | 0xA0B3 |
| MW_MAIN_1090_OPTIONS.cpp | `saveDiskFullText` | 0xA088 |
| MW_MAIN_1090_OPTIONS.cpp | `saveNamePrompt` | 0xA031 |
| MW_MAIN_1090_OPTIONS.cpp | `terrainDetailLabels` | 0xA079 |

## Ours: screen layout, panels and menus (data/ui/*.json)

| File | Item | Native offset |
|---|---|---|
| BTECH_304E_COCKPIT.cpp | `inspectionDiagramOrigins` | 0x1878 |
| BTECH_304E_COCKPIT.cpp | `mechCockpitLayoutIndices` | 0x1D38 |
| BTECH_304E_COCKPIT.cpp | `subsystemLampColourTables` | 0x23CE |
| BTECH_304E_COCKPIT.cpp | `targetDiagramAnchors` | 0x1AD8 |
| BTECH_304E_ORDERS.cpp | `orderEngineConditionColours` | 0x030E |
| BTECH_304E_ORDERS.cpp | `orderMapPositionMask` | 0x4EF0 |
| BTECH_304E_ORDERS.cpp | `orderNavigationPanelDeltas` | 0x009A |
| BTECH_304E_ORDERS.cpp | `orderNavigationRowDeltas` | 0x00B0 |
| BTECH_304E_ORDERS.cpp | `orderSelectionFirstRows` | 0x0092 |
| BTECH_304E_ORDERS.cpp | `orderSelectionLastRows` | 0x0096 |
| BTECH_304E_ORDERS.cpp | `orderSystemConditionColours` | 0x031E |
| MW_MAIN_1090.cpp | `afterActionRosterRowOffset` | 0x1229 |
| MW_MAIN_1090.cpp | `barMenuPanel` | 0x8FF6 |
| MW_MAIN_1090.cpp | `bodyDamageTierFillColours` | 0x3CD2 |
| MW_MAIN_1090.cpp | `calendarMonthNameWidths` | 0x11BA |
| MW_MAIN_1090.cpp | `commanderProfilePanel` | 0x2258 |
| MW_MAIN_1090.cpp | `companyMenuPanel` | 0x21CE |
| MW_MAIN_1090.cpp | `componentRepairPanel` | 0x3214 |
| MW_MAIN_1090.cpp | `contractDateYearXOffsets` | 0x4B1A |
| MW_MAIN_1090.cpp | `crewLabelsPanel` | 0x2325 |
| MW_MAIN_1090.cpp | `dropShipStatusPanel` | 0x2D06 |
| MW_MAIN_1090.cpp | `dropShipTravelFrames` | 0x5B2E |
| MW_MAIN_1090.cpp | `hiredTransportFrames` | 0x5AEA |
| MW_MAIN_1090.cpp | `marketComponentPanel` | 0x44F1 |
| MW_MAIN_1090.cpp | `marketDropShipStatusPanel` | 0x443E |
| MW_MAIN_1090.cpp | `marketMechStatusPanel` | 0x437E |
| MW_MAIN_1090.cpp | `marketSystemConditionRows` | 0x454F |
| MW_MAIN_1090.cpp | `marketUnitListPanel` | 0x28F1 |
| MW_MAIN_1090.cpp | `marketUnitListRows` | 0x2931 |
| MW_MAIN_1090.cpp | `marketWeaponConditionRows` | 0x46BA |
| MW_MAIN_1090.cpp | `mechComplexAnimationFrames` | 0x318D |
| MW_MAIN_1090.cpp | `mechComplexPanel` | 0x25C5 |
| MW_MAIN_1090.cpp | `mechStatusPanel` | 0x2C3F |
| MW_MAIN_1090.cpp | `ownedUnitListPanel` | 0x266A |
| MW_MAIN_1090.cpp | `ownedUnitListRows` | 0x26B5 |
| MW_MAIN_1090.cpp | `pilotCardColumnOrigins` | 0x244A |
| MW_MAIN_1090.cpp | `rearmConfirmationPanel` | 0x3110 |
| MW_MAIN_1090.cpp | `recruitInterviewPanel` | 0x9076 |
| MW_MAIN_1090.cpp | `repairSystemConditionRows` | 0x3272 |
| MW_MAIN_1090.cpp | `repairTitleLeadingSpacesByKind` | 0x31F2 |
| MW_MAIN_1090.cpp | `repairWeaponConditionRows` | 0x3403 |
| MW_MAIN_1090.cpp | `reserveAmmoMechColours` | 0x3F70 |
| MW_MAIN_1090.cpp | `saleConfirmationPanel` | 0x3075 |
| MW_MAIN_1090.cpp | `travelBrowsePanel` | 0x5C75 |
| MW_MAIN_1090.cpp | `travelConfirmPanel` | 0x5C17 |
| MW_MAIN_1090.cpp | `travelCostPanel` | 0x5BF9 |
| MW_MAIN_1090.cpp | `travelDestinationHeaderPanel` | 0x5C35 |
| MW_MAIN_1090.cpp | `travelHouseBrowserPanel` | 0x5A0D |
| MW_MAIN_1090.cpp | `unitDetailPictureXOffsets` | 0x2B83 |
| MW_MAIN_1090.cpp | `unitDetailPictureYOrigins` | 0x2B99 |
| MW_MAIN_1090_OPTIONS.cpp | `confirmationMenu` | 0x9D34 |
| MW_MAIN_1090_OPTIONS.cpp | `confirmationPanel` | 0x9CBE |
| MW_MAIN_1090_OPTIONS.cpp | `newsMenu` | 0x2538 |
| MW_MAIN_1090_OPTIONS.cpp | `newsMenuPanel` | 0x9CBE |
| MW_MAIN_1090_OPTIONS.cpp | `optionsMenu` | 0x9DEB |
| MW_MAIN_1090_OPTIONS.cpp | `optionsPanel` | 0x9D52 |
| MW_MAIN_1090_OPTIONS.cpp | `restoreMenu` | 0x9F7B |
| MW_MAIN_1090_OPTIONS.cpp | `restorePanel` | 0x9D52 |

## Ours: rules and constants (data/rules/*.json)

| File | Item | Native offset |
|---|---|---|
| BTECH_304E.cpp | `actuatorIndicesByBodyLocation` | 0x0E5E |
| BTECH_304E.cpp | `actuatorIntegrityTierByCondition` | 0x09DC |
| BTECH_304E.cpp | `actuatorLossByDamageTier` | 0x08D0 |
| BTECH_304E.cpp | `armorToBodyLocations` | 0x0E3A |
| BTECH_304E.cpp | `campaignBodyTierByArmorLocation` | 0x08AE |
| BTECH_304E.cpp | `campaignUnitKindByCombatTemplate` | 0x09CC |
| BTECH_304E.cpp | `combatContinuationProfiles` | 0x08F6 |
| BTECH_304E.cpp | `combatTemplateByCampaignUnitKind` | 0x08E2 |
| BTECH_304E.cpp | `criticalTransferBodyLocations` | 0x0E28 |
| BTECH_304E.cpp | `damageTransferArmorLocations` | 0x0E4C |
| BTECH_304E.cpp | `heatAimJitterThresholds` | 0x271E |
| BTECH_304E.cpp | `mechFallbackStartTemplates` | 0x0982 |
| BTECH_304E.cpp | `mechTemplateBudgetCostsBytes` | 0x09A2 |
| BTECH_304E.cpp | `mechTemplates / weaponDefinitions (BtechImportMechInitialData)` | BtechImportMechInitialData |
| BTECH_304E.cpp | `opposingObjectiveRules` | 0x08D8 |
| BTECH_304E.cpp | `sideObjectiveRules` | 0x0ED0 |
| BTECH_304E.cpp | `systemDamageTierByHits` | 0x09E6 |
| BTECH_304E.cpp | `systemHitsByDamageTier` | 0x08C8 |
| BTECH_304E.cpp | `weaponDisabledByDamageTier` | 0x08C0 |
| BTECH_304E.cpp | `weaponIntegrityTierByOperational` | 0x09EC |
| BTECH_304E_COCKPIT.cpp | `actuatorConditionLabelIndices` | 0x04E6 |
| BTECH_304E_COCKPIT.cpp | `engineConditionLabelIndices` | 0x04DE |
| BTECH_304E_COCKPIT.cpp | `twoHitSystemConditionLabelIndices` | 0x04D8 |
| BTECH_304E_ORDERS.cpp | `mountedWeaponCountsByKind` | 0x4F32 |
| BTECH_304E_ORDERS.cpp | `operationalWeaponCountsByKind` | 0x4F32 |
| BTECH_304E_ORDERS.cpp | `orderDisplayIndices` | 0x02C4 |
| BTECH_304E_POSE.cpp | `animationPrefixSequenceCounts` | 0x24B4 |
| BTECH_304E_POSE.cpp | `playerPoseFrameCounts` | 0x265E |
| BTECH_304E_RENDER.cpp | `selectionObjectListIndices` | 0x6F5A |
| BTECH_304E_WORLD.cpp | `avoidanceHeadingOffsets` | 0x2438 |
| BTECH_304E_WORLD.cpp | `bodyDamageEffectResourceDeltas` | 0x2342 |
| BTECH_304E_WORLD.cpp | `bodyLocationReferenceIndices` | 0x0EBC |
| BTECH_304E_WORLD.cpp | `defaultEffectResourceDeltas` | 0x0E96 |
| BTECH_304E_WORLD.cpp | `hitLocationRows` | 0x26BC |
| BTECH_304E_WORLD.cpp | `machineGunEffectResourceDeltas` | 0x0EA4 |
| BTECH_304E_WORLD.cpp | `missileClusterRowByWeapon` | 0x234A |
| BTECH_304E_WORLD.cpp | `missileClusterRows` | 0x235E |
| BTECH_304E_WORLD.cpp | `opposingSideFirstUnitSlots` | 0x2434 |
| BTECH_304E_WORLD.cpp | `projectileProfileIndicesByWeapon` | 0x232E |
| BTECH_304E_WORLD.cpp | `rangeHitModifiers` | 0x2714 |
| BTECH_304E_WORLD.cpp | `scenarioSelectors` | 0x7D6A |
| MW_MAIN_1090.cpp | `ammunitionBasePrices` | 0x2EF8 |
| MW_MAIN_1090.cpp | `ammunitionTransactionPrices` | 0x2F10 |
| MW_MAIN_1090.cpp | `bodyRepairBasePricesByKind` | 0x3CAB |
| MW_MAIN_1090.cpp | `campaignMechTonnages` | 0x122B |
| MW_MAIN_1090.cpp | `defaultHouseMissionTypeLimits` | 0x5CBA |
| MW_MAIN_1090.cpp | `generateStartingJenner` | 0x3A6A |
| MW_MAIN_1090.cpp | `generatedAmmoKinds` | 0x3A5E |
| MW_MAIN_1090.cpp | `generatedAmmoQuantities` | 0x3A64 |
| MW_MAIN_1090.cpp | `generatedKindRangeMaxima` | 0x3A37 |
| MW_MAIN_1090.cpp | `generatedKindRangeStarts` | 0x3A3A |
| MW_MAIN_1090.cpp | `houseAdvanceBasePercent` | 0x4B69 |
| MW_MAIN_1090.cpp | `houseDeductionRollThresholds` | 0x1241 |
| MW_MAIN_1090.cpp | `houseExtraEnemyRollThresholds` | 0x4BF0 |
| MW_MAIN_1090.cpp | `houseMissionPlanetPolicies` | 0x8E9A |
| MW_MAIN_1090.cpp | `houseNegotiationAttemptBase` | 0x11A3 |
| MW_MAIN_1090.cpp | `housePaymentAdjustmentPercent` | 0x4B4B |
| MW_MAIN_1090.cpp | `housePaymentRollBias` | 0x4B41 |
| MW_MAIN_1090.cpp | `housePlanetRangeCounts` | 0x6566 |
| MW_MAIN_1090.cpp | `housePlanetRangeStarts` | 0x655C |
| MW_MAIN_1090.cpp | `houseReducedPayoutPercentages` | 0x124B |
| MW_MAIN_1090.cpp | `houseSalvageBasePercent` | 0x4B5F |
| MW_MAIN_1090.cpp | `missionDeploymentUnitsByScenario` | 0x8E46 |
| MW_MAIN_1090.cpp | `missionForceBudget` | 0x4B79 |
| MW_MAIN_1090.cpp | `missionForceWeightByUnitKind` | 0x4B55 |
| MW_MAIN_1090.cpp | `missionPaymentBaseThousands` | 0x4B7B |
| MW_MAIN_1090.cpp | `pilotCardGunneryTier` | 0x2440 |
| MW_MAIN_1090.cpp | `pilotCardPilotingTier` | 0x2442 |
| MW_MAIN_1090.cpp | `pilotCardQuotedWageTier` | 0x2446 |
| MW_MAIN_1090.cpp | `pilotWageAmounts` | 0x2476 |
| MW_MAIN_1090.cpp | `repairAttemptFailuresByKind` | 0x3A77 |
| MW_MAIN_1090.cpp | `repairSuccessRollLimitsByMarketTier` | 0x3A6B |
| MW_MAIN_1090.cpp | `reserveAmmoFreightRates` | 0x5C94 |
| MW_MAIN_1090.cpp | `settlementPayoutPercent` | 0x123F |
| MW_MAIN_1090.cpp | `systemRepairBasePrices` | 0x3B53 |
| MW_MAIN_1090.cpp | `travelDestinationTimeWeight` | 0x54DA |
| MW_MAIN_1090.cpp | `unitAmmoGroupCounts` | 0x2EEE |
| MW_MAIN_1090.cpp | `unitAmmoProfileEntries` | 0x2F3C |
| MW_MAIN_1090.cpp | `unitBasePrices` | 0x305F |
| MW_MAIN_1090.cpp | `unitConditionScoreDivisors` | 0x2B18 |
| MW_MAIN_1090.cpp | `unitConditionSystemFieldCounts` | 0x2AE3 |
| MW_MAIN_1090.cpp | `unitDisplayJumpCapacitiesMetres` | 0x2B79 |
| MW_MAIN_1090.cpp | `unitDisplaySpeedsKph` | 0x2B6F |
| MW_MAIN_1090.cpp | `unitDisplayTonnages` | 0x2B65 |
| MW_MAIN_1090.cpp | `unitHeatSinkCounts` | 0x368C |
| MW_MAIN_1090.cpp | `unitJumpJetGroupCounts` | 0x3696 |
| MW_MAIN_1090.cpp | `unitPriceTierMultipliers` | 0x3057 |
| MW_MAIN_1090.cpp | `unitWeaponCounts` | 0x2AED |
| MW_MAIN_1090.cpp | `unusedRepairTimeByDamageTier` | 0x3A6F |
| MW_MAIN_1090.cpp | `weaponRepairPrices` | 0x3C2F |

## Ours: the new campaign's starting state (data/rules/new_campaign.json)

| File | Item | Native offset |
|---|---|---|
| MW_MAIN_1090.cpp | `acceptedContractActive` | 0x0468 |
| MW_MAIN_1090.cpp | `acceptedContractAdvancePercent` | 0x09FE |
| MW_MAIN_1090.cpp | `acceptedContractPaymentThousands` | 0x09FA |
| MW_MAIN_1090.cpp | `acceptedContractSalvagePercent` | 0x09FC |
| MW_MAIN_1090.cpp | `acceptedContractTargetHouseIndex` | 0x0B53 |
| MW_MAIN_1090.cpp | `audioOptionLabelIndex` | 0x0A2A |
| MW_MAIN_1090.cpp | `availableRecruitCount` | 0x051E |
| MW_MAIN_1090.cpp | `availableRecruitIds` | 0x0520 |
| MW_MAIN_1090.cpp | `calendarMonthIndex` | 0x047B |
| MW_MAIN_1090.cpp | `calendarSubmonthUnits` | 0x0479 |
| MW_MAIN_1090.cpp | `calendarYear` | 0x047D |
| MW_MAIN_1090.cpp | `campaignFundsHigh` | 0x0493 |
| MW_MAIN_1090.cpp | `campaignFundsLow` | 0x0491 |
| MW_MAIN_1090.cpp | `campaignTerrainDetailLevel` | 0x0B55 |
| MW_MAIN_1090.cpp | `combatEnemyHeavyCount` | 0x044B |
| MW_MAIN_1090.cpp | `combatEnemyLightCount` | 0x044D |
| MW_MAIN_1090.cpp | `combatEnemyMediumCount` | 0x044C |
| MW_MAIN_1090.cpp | `combatPlanetSelector` | 0x044E |
| MW_MAIN_1090.cpp | `combatScenarioCode` | 0x044F |
| MW_MAIN_1090.cpp | `combatTransitionResult` | 0x0467 |
| MW_MAIN_1090.cpp | `commanderAgeYears` | 0x048F |
| MW_MAIN_1090.cpp | `commanderNameBytes` | 0x0483 |
| MW_MAIN_1090.cpp | `contractNegotiationAttemptLimit` | 0x09E6 |
| MW_MAIN_1090.cpp | `currentPlanetFacilityDisplayFlag` | 0x0471 |
| MW_MAIN_1090.cpp | `currentPlanetHouseIndex` | 0x046D |
| MW_MAIN_1090.cpp | `currentPlanetIndex` | 0x0469 |
| MW_MAIN_1090.cpp | `currentPlanetLandscapeIndex` | 0x046B |
| MW_MAIN_1090.cpp | `currentPlanetMapPositionX` | 0x0473 |
| MW_MAIN_1090.cpp | `currentPlanetMapPositionY` | 0x0475 |
| MW_MAIN_1090.cpp | `currentPlanetMarketTier` | 0x046F |
| MW_MAIN_1090.cpp | `darkWingBaseVictoryRecorded` | 0x0B5C |
| MW_MAIN_1090.cpp | `dealerUnitPurchaseLimit` | 0x0B56 |
| MW_MAIN_1090.cpp | `deploymentLandscapeIndex` | 0x044A |
| MW_MAIN_1090.cpp | `deploymentTerrainDetailLevel` | 0x0450 |
| MW_MAIN_1090.cpp | `dropShipMissedUpkeepPayments` | 0x051A |
| MW_MAIN_1090.cpp | `elapsedBillingMonths` | 0x0481 |
| MW_MAIN_1090.cpp | `eventDirectoryIndex` | 0x0AB8 |
| MW_MAIN_1090.cpp | `eventSeenFlags` | 0x0ABA |
| MW_MAIN_1090.cpp | `houseFavorPoints` | 0x049F |
| MW_MAIN_1090.cpp | `houseGrievancePoints` | 0x04A9 |
| MW_MAIN_1090.cpp | `houseMissionTypeLimits` | 0x0A02 |
| MW_MAIN_1090.cpp | `houseNetStandings` | 0x0495 |
| MW_MAIN_1090.cpp | `majorCampaignMissionRollThreshold` | 0x0B52 |
| MW_MAIN_1090.cpp | `marketAmmoKindsByGroup` | 0x078A |
| MW_MAIN_1090.cpp | `marketAmmoQuantitiesByGroup` | 0x06EE |
| MW_MAIN_1090.cpp | `marketRefreshUnits` | 0x047F |
| MW_MAIN_1090.cpp | `marketUnitCount` | 0x07D2 |
| MW_MAIN_1090.cpp | `marketUnitDamage` | 0x07EC |
| MW_MAIN_1090.cpp | `marketUnitKinds` | 0x07D4 |
| MW_MAIN_1090.cpp | `missionAdvancePercentageCeilings` | 0x09DA |
| MW_MAIN_1090.cpp | `missionAdvancePercentages` | 0x09B6 |
| MW_MAIN_1090.cpp | `missionCategoryOrMemoryTier` | 0x0449 |
| MW_MAIN_1090.cpp | `missionEnemyHeavyCounts` | 0x097A |
| MW_MAIN_1090.cpp | `missionEnemyLightCounts` | 0x0992 |
| MW_MAIN_1090.cpp | `missionEnemyMediumCounts` | 0x0986 |
| MW_MAIN_1090.cpp | `missionOfferCount` | 0x0948 |
| MW_MAIN_1090.cpp | `missionPaymentCeilingsThousands` | 0x09C2 |
| MW_MAIN_1090.cpp | `missionPaymentThousands` | 0x099E |
| MW_MAIN_1090.cpp | `missionPlanetRecordIndices` | 0x0956 |
| MW_MAIN_1090.cpp | `missionSalvagePercentageCeilings` | 0x09CE |
| MW_MAIN_1090.cpp | `missionSalvagePercentages` | 0x09AA |
| MW_MAIN_1090.cpp | `missionScenarioCodes` | 0x0A2B |
| MW_MAIN_1090.cpp | `missionTargetHouseIndices` | 0x0962 |
| MW_MAIN_1090.cpp | `missionTypeCodes` | 0x096E |
| MW_MAIN_1090.cpp | `ownedAmmoKindsByGroup` | 0x0742 |
| MW_MAIN_1090.cpp | `ownedAmmoQuantitiesByGroup` | 0x06A6 |
| MW_MAIN_1090.cpp | `ownedUnitCount` | 0x0530 |
| MW_MAIN_1090.cpp | `ownedUnitDamage` | 0x054A |
| MW_MAIN_1090.cpp | `ownedUnitKinds` | 0x0532 |
| MW_MAIN_1090.cpp | `pilotDepartureFlagTail` | 0x04E9 |
| MW_MAIN_1090.cpp | `pilotExperienceCounts` | 0x0A8D |
| MW_MAIN_1090.cpp | `pilotGunneryTiers` | 0x0A37 |
| MW_MAIN_1090.cpp | `pilotPilotingTiers` | 0x0A62 |
| MW_MAIN_1090.cpp | `planetMarketPremiumPercent` | 0x0477 |
| MW_MAIN_1090.cpp | `playerCombatCredit` | 0x04B3 |
| MW_MAIN_1090.cpp | `playerSkillProgress` | 0x0B5A |
| MW_MAIN_1090.cpp | `remainingContractPaymentThousands` | 0x0A00 |
| MW_MAIN_1090.cpp | `requestContractAdvanceEnabled` | 0x09F2 |
| MW_MAIN_1090.cpp | `requestContractPaymentEnabled` | 0x09EE |
| MW_MAIN_1090.cpp | `requestContractSalvageEnabled` | 0x09F0 |
| MW_MAIN_1090.cpp | `requestedContractAdvancePercent` | 0x09EC |
| MW_MAIN_1090.cpp | `requestedContractPaymentThousands` | 0x09E8 |
| MW_MAIN_1090.cpp | `requestedContractSalvagePercent` | 0x09EA |
| MW_MAIN_1090.cpp | `reserveAmmoQuantitiesByFamily` | 0x0736 |
| MW_MAIN_1090.cpp | `returnedDisabledEnemyCount` | 0x0465 |
| MW_MAIN_1090.cpp | `rosterGunneryTiers` | 0x04F2 |
| MW_MAIN_1090.cpp | `rosterMechAssignments` | 0x050A |
| MW_MAIN_1090.cpp | `rosterPayTiers` | 0x04EA |
| MW_MAIN_1090.cpp | `rosterPilotingTiers` | 0x04FA |
| MW_MAIN_1090.cpp | `rosterPortraitIds` | 0x0512 |
| MW_MAIN_1090.cpp | `rosterStatus` | 0x0451 |
| MW_MAIN_1090.cpp | `rosterUnknownWords` | 0x0502 |
| MW_MAIN_1090.cpp | `salvageEntries` | 0x0455 |
| MW_MAIN_1090.cpp | `savedVideoMode` | 0x0B5F |
| MW_MAIN_1090.cpp | `unidentifiedField18` | 0x0448 |
| MW_MAIN_1090.cpp | `unidentifiedField19` | 0x048D |
| MW_MAIN_1090.cpp | `unidentifiedField20` | 0x09F4 |
| MW_MAIN_1090.cpp | `unitsSoldSinceMarketRefresh` | 0x0B58 |
| MW_MAIN_1090.cpp | `unpaidPilotWageMarker` | 0x051C |

## Dropped

| File | Item | Native offset |
|---|---|---|
| BTECH_304E.cpp | `resourceDiskPromptNumber` | 0xCFD6 |
| BTECH_304E_VIDEO.cpp | `herculesDisplayRowOffsets` | 0x34DE |
| MW_0806_STATE.cpp | `launcherCampaignDiskPrompt` | 0x0871 |
| MW_0806_STATE.cpp | `launcherCampaignFilename` | 0x07E3 |
| MW_0806_STATE.cpp | `launcherCombatDiskPrompt` | 0x088D |
| MW_0806_STATE.cpp | `launcherCombatFilename` | 0x07EF |
| MW_0806_STATE.cpp | `launcherDiskSuffix` | 0x09B1 |
| MW_0806_STATE.cpp | `launcherGraphicsPrompt` | 0x08CA |
| MW_0806_STATE.cpp | `launcherIdentity` | 0x07D4 |
| MW_0806_STATE.cpp | `launcherMemoryError` | 0x08A9 |
| MW_0806_STATE.cpp | `launcherReleaseError` | 0x085A |
| MW_MAIN_1090.cpp | `changeDiskPrompt` | 0x0E71 |
| MW_MAIN_1090.cpp | `combatDiskPrompt` | 0x0F00 |
| MW_MAIN_1090.cpp | `combatExecutableFilename` | 0x0EF6 |
| MW_MAIN_1090.cpp | `copyProtectionAcceptedText` | 0x2153 |
| MW_MAIN_1090.cpp | `copyProtectionAnswers` | 0x163A |
| MW_MAIN_1090.cpp | `copyProtectionCallsigns` | 0x0F8F |
| MW_MAIN_1090.cpp | `copyProtectionPictureIndices` | 0x1FA1 |
| MW_MAIN_1090.cpp | `copyProtectionPrompt` | 0x1F1C |
| MW_MAIN_1090.cpp | `copyProtectionSelection` | 0x1F1A |
| MW_MAIN_1090.cpp | `insertDiskOnePrompt` | 0x0ED6 |
| MW_MAIN_1090.cpp | `launcherExitCode` | 0x0006 |
| MW_MAIN_1090.cpp | `launcherMemoryTier` | 0x0B5D |
| MW_MAIN_1090.cpp | `startupErrorText` | 0x018B |
| MW_MAIN_1090.cpp | `tandyGraphicsSelected` | 0x03A4 |
| MW_MAIN_1090_OPTIONS.cpp | `saveDiskPrompt` | 0xA114 |

## Code: runtime state and renderer tables (no file)

Not listed one by one: BTECH_0810_STATE.cpp 1, BTECH_1ADF_STATE.cpp 2, BTECH_1BF5_STATE.cpp 5, BTECH_22B6_DATA.cpp 15, BTECH_2FEB.cpp 24, BTECH_304E.cpp 38, BTECH_304E_ADLIB.cpp 25, BTECH_304E_COCKPIT.cpp 45, BTECH_304E_MPU.cpp 10, BTECH_304E_ORDERS.cpp 33, BTECH_304E_POLYGONS.cpp 24, BTECH_304E_POSE.cpp 16, BTECH_304E_RENDER.cpp 61, BTECH_304E_RUNTIME.cpp 17, BTECH_304E_SCENE.cpp 8, BTECH_304E_SOUND.cpp 42, BTECH_304E_STARTUP.cpp 59, BTECH_304E_VIDEO.cpp 87, BTECH_304E_WORLD.cpp 110, MW_0806_STATE.cpp 7, MW_EGA_0800_STATE.cpp 66, MW_MAIN_0F41_STATE.cpp 5, MW_MAIN_1090.cpp 324, MW_MAIN_1090_OPTIONS.cpp 21, MW_MAIN_1AA5.cpp 2, MW_MAIN_28BE.cpp 39.
