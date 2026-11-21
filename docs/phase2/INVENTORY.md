# Phase 2 inventory

Generated from `src/Original/Memory` by `tools/phase2/inventory.py` (every file-scope item). An item read through a door shows where it now comes from: a story piece, a rule piece, or a save codec field of `new_campaign.json`, with the exe offset those tables give. Any other item shows the first native offset its importer reads, or `-` for runtime state with no import. The groups follow `docs/phase2/PLAN.md`. Names and offsets only: no MW1 text.

| Group | Items |
|---|---|
| Extracted: the original's creative content (the story file, first run) | 61 |
| Ours: UI words and system messages (data/text/interface.json) | 108 |
| Ours: screen layout, panels and menus (data/ui/*.json) | 58 |
| Ours: rules and constants (data/rules/*.json) | 78 |
| Ours: the new campaign's starting state (data/rules/new_campaign.json) | 103 |
| Dropped | 6 |
| Code: runtime state and renderer tables (no file) | 1085 |

## Extracted: the original's creative content (the story file, first run)

| File | Item | Offset | From |
|---|---|---|---|
| BTECH_304E.cpp | `combatDefaultCommanderNameBytes` | BTECH 304E:0978 | story combat.commander.default_name |
| BTECH_304E_ADLIB.cpp | `combatAdlibDefaultInstruments` | - | story, MwStoryCopyAdlibInstruments |
| BTECH_304E_COCKPIT.cpp | `mechTemplateNames` | BTECH 304E:2045 | story combat.mech_template.1.name |
| BTECH_304E_COCKPIT.cpp | `missionTitleText` | 0x09F0 | - |
| BTECH_304E_COCKPIT.cpp | `pilotDefinitionNameDirectory` | BTECH 304E:20AD | story combat.pilot.1.name |
| BTECH_304E_COCKPIT.cpp | `pilotDefinitionNames` | BTECH 304E:20AD | story combat.pilot.1.name |
| BTECH_304E_SOUND.cpp | `musicSequences` | 0x7D8A | - |
| MW_MAIN_1090.cpp | `afterActionGeneralFailure` | MW_MAIN 1090:159B | story after_action.failure.general |
| MW_MAIN_1090.cpp | `afterActionGeneralSuccess` | MW_MAIN 1090:1742 | story after_action.success.general |
| MW_MAIN_1090.cpp | `afterActionKuritaFailure` | MW_MAIN 1090:149B | story after_action.failure.kurita |
| MW_MAIN_1090.cpp | `afterActionKuritaSuccess` | MW_MAIN 1090:1674 | story after_action.success.kurita |
| MW_MAIN_1090.cpp | `afterActionLiaoFailure` | MW_MAIN 1090:1528 | story after_action.failure.liao |
| MW_MAIN_1090.cpp | `afterActionLiaoSuccess` | MW_MAIN 1090:16D6 | story after_action.success.liao |
| MW_MAIN_1090.cpp | `alphabeticPlanetRecordOffsets` | - | story, MwStoryBuildPlanets |
| MW_MAIN_1090.cpp | `campaignDefaultCommanderNameBytes` | MW_MAIN 1090:5CE2 | story commander.default_name |
| MW_MAIN_1090.cpp | `contractNarrative` | MW_MAIN 1090:4890 | story contract.narrative |
| MW_MAIN_1090.cpp | `contractOfficePlanetGroupStorage` | 0x4B8A | - |
| MW_MAIN_1090.cpp | `darkWingAttackFailedText` | MW_MAIN 1090:1BBC | story dark_wing.attack_failed |
| MW_MAIN_1090.cpp | `darkWingVictoryText` | MW_MAIN 1090:1A37 | story dark_wing.victory |
| MW_MAIN_1090.cpp | `deploymentUneventfulText` | MW_MAIN 1090:140D | story deployment.uneventful |
| MW_MAIN_1090.cpp | `eventPlanetChoicePool0` | - | story, MwStoryCopyPlanetPools |
| MW_MAIN_1090.cpp | `eventPlanetChoicePool1` | - | story, MwStoryCopyPlanetPools |
| MW_MAIN_1090.cpp | `eventPlanetChoicePool2` | - | story, MwStoryCopyPlanetPools |
| MW_MAIN_1090.cpp | `eventPlanetChoicePool3` | - | story, MwStoryCopyPlanetPools |
| MW_MAIN_1090.cpp | `eventPlanetChoicePool4` | - | story, MwStoryCopyPlanetPools |
| MW_MAIN_1090.cpp | `eventPlanetChoicePool5` | - | story, MwStoryCopyPlanetPools |
| MW_MAIN_1090.cpp | `houseContractNames` | MW_MAIN 1090:4A73 | story house.kurita.name |
| MW_MAIN_1090.cpp | `houseContractRefusalText` | MW_MAIN 1090:4C87 | story contract.refusal.house |
| MW_MAIN_1090.cpp | `introFifthLeftText` | MW_MAIN 1090:1D92 | story intro.caption.5.left |
| MW_MAIN_1090.cpp | `introFifthRightText` | MW_MAIN 1090:1DDC | story intro.caption.5.right |
| MW_MAIN_1090.cpp | `introFirstLeftText` | MW_MAIN 1090:1C37 | story intro.caption.1.left |
| MW_MAIN_1090.cpp | `introFirstRightText` | MW_MAIN 1090:1C5B | story intro.caption.1.right |
| MW_MAIN_1090.cpp | `introFourthLeftText` | MW_MAIN 1090:1D57 | story intro.caption.4.left |
| MW_MAIN_1090.cpp | `introFourthRightText` | MW_MAIN 1090:1D73 | story intro.caption.4.right |
| MW_MAIN_1090.cpp | `introNarrationLines` | MW_MAIN 1090:129C | story intro.narration.1 |
| MW_MAIN_1090.cpp | `introSecondLeftText` | MW_MAIN 1090:1C84 | story intro.caption.2.left |
| MW_MAIN_1090.cpp | `introSecondRightText` | MW_MAIN 1090:1CAD | story intro.caption.2.right |
| MW_MAIN_1090.cpp | `introSeventhLeftText` | MW_MAIN 1090:1E44 | story intro.caption.7.left |
| MW_MAIN_1090.cpp | `introSeventhRightText` | MW_MAIN 1090:1EBC | story intro.caption.7.right |
| MW_MAIN_1090.cpp | `introSixthLeftText` | MW_MAIN 1090:1DFB | story intro.caption.6.left |
| MW_MAIN_1090.cpp | `introSixthRightText` | MW_MAIN 1090:1E1C | story intro.caption.6.right |
| MW_MAIN_1090.cpp | `introThirdLeftText` | MW_MAIN 1090:1CD3 | story intro.caption.3.left |
| MW_MAIN_1090.cpp | `introThirdRightText` | MW_MAIN 1090:1D23 | story intro.caption.3.right |
| MW_MAIN_1090.cpp | `missionDescriptionListEntries` | - | story, MwStoryBuildMissions |
| MW_MAIN_1090.cpp | `missionDescriptionListOffsetsByCategory` | - | story, MwStoryBuildMissions |
| MW_MAIN_1090.cpp | `missionDescriptionMaximumIndicesByCategory` | - | story, MwStoryBuildMissions |
| MW_MAIN_1090.cpp | `missionDescriptionStorage` | - | story, MwStoryBuildMissions |
| MW_MAIN_1090.cpp | `pilotDefinitionOffsets` | - | story, MwStoryBuildPilots |
| MW_MAIN_1090.cpp | `pilotDefinitionStorage` | - | story, MwStoryBuildPilots |
| MW_MAIN_1090.cpp | `pilotDefinitions` | - | story, MwStoryBuildPilots |
| MW_MAIN_1090.cpp | `planetRecordOffsets` | - | story, MwStoryBuildPlanets |
| MW_MAIN_1090.cpp | `planetRecordStorage` | - | story, MwStoryBuildPlanets |
| MW_MAIN_1090.cpp | `settlementNoPaymentText` | MW_MAIN 1090:19A2 | story settlement.no_payment |
| MW_MAIN_1090.cpp | `settlementPerformanceDeductionText` | MW_MAIN 1090:1834 | story settlement.deduction.performance |
| MW_MAIN_1090.cpp | `settlementSupplyDeductionText` | MW_MAIN 1090:178C | story settlement.deduction.supply |
| MW_MAIN_1090.cpp | `settlementTransportDeductionText` | MW_MAIN 1090:18E0 | story settlement.deduction.transport |
| MW_MAIN_1090.cpp | `shopUnitNames` | MW_MAIN 1090:2DCB | story unit.locust.shop_name |
| MW_MAIN_1090.cpp | `unitDisplayNames` | MW_MAIN 1090:2E86 | story unit.locust.name |
| MW_MAIN_1090_EVENTS.cpp | `the event script (records, conditions, actions)` | - | story, MwStoryBuildCampaignEvents |
| MW_MAIN_1AA5.cpp | `eventTextStorage` | - | story, MwStoryBuildEventText |
| MW_MAIN_1AA5.cpp | `eventTextStorage / eventTextRecords` | - | story, MwStoryBuildEventText |

## Ours: UI words and system messages (data/text/interface.json)

| File | Item | Offset | From |
|---|---|---|---|
| BTECH_304E_COCKPIT.cpp | `actuatorConditionLabelIndices` | 0x04E6 | - |
| BTECH_304E_COCKPIT.cpp | `actuatorLabels` | 0x046C | - |
| BTECH_304E_COCKPIT.cpp | `automaticWeaponsLabel` | 0x0059 | - |
| BTECH_304E_COCKPIT.cpp | `bodyLocationLabels` | 0x038E | - |
| BTECH_304E_COCKPIT.cpp | `engineConditionLabelIndices` | 0x04DE | - |
| BTECH_304E_COCKPIT.cpp | `heatSinksOfText` | 0x0566 | - |
| BTECH_304E_COCKPIT.cpp | `inspectionConditionLabels` | 0x03A6 | - |
| BTECH_304E_COCKPIT.cpp | `inspectionSubsystemLabels` | 0x0520 | - |
| BTECH_304E_COCKPIT.cpp | `inspectionWeaponHeading` | 0x057A | - |
| BTECH_304E_COCKPIT.cpp | `jumpJetsOfText` | 0x0577 | - |
| BTECH_304E_COCKPIT.cpp | `jumpJetsText` | 0x0569 | - |
| BTECH_304E_COCKPIT.cpp | `pausedText` | 0x0081 | - |
| BTECH_304E_COCKPIT.cpp | `radarRangeText` | 0x23EA | - |
| BTECH_304E_COCKPIT.cpp | `reactorShutdownText` | 0x005D | - |
| BTECH_304E_COCKPIT.cpp | `retreatChoiceText` | 0x083E | - |
| BTECH_304E_COCKPIT.cpp | `retreatQuestionText` | 0x082F | - |
| BTECH_304E_COCKPIT.cpp | `simulationText` | 0x0076 | - |
| BTECH_304E_COCKPIT.cpp | `subsystemLampLabels` | 0x23BE | - |
| BTECH_304E_COCKPIT.cpp | `twoHitSystemConditionLabelIndices` | 0x04D8 | - |
| BTECH_304E_COCKPIT.cpp | `weaponLabelText` | 0x2004 | - |
| BTECH_304E_COCKPIT.cpp | `weaponRangeAndAmmoText` | 0x2410 | - |
| BTECH_304E_COCKPIT.cpp | `zoomLabel` | 0x0052 | - |
| BTECH_304E_ORDERS.cpp | `orderDisplayIndices` | 0x02C4 | - |
| MW_MAIN_1090.cpp | `acceptContractCaption` | 0x4B35 | - |
| MW_MAIN_1090.cpp | `afterActionCrewHeading` | 0x11DF | - |
| MW_MAIN_1090.cpp | `afterActionMechHeading` | 0x11EC | - |
| MW_MAIN_1090.cpp | `afterActionPaymentHeading` | 0x1202 | - |
| MW_MAIN_1090.cpp | `afterActionPercentSuffix` | 0x120B | - |
| MW_MAIN_1090.cpp | `afterActionPilotStatusText` | 0x120D | - |
| MW_MAIN_1090.cpp | `afterActionSalvageHeading` | 0x11F9 | - |
| MW_MAIN_1090.cpp | `ammoBayFullText` | 0x315B | - |
| MW_MAIN_1090.cpp | `ammoFamilyDisplayNames` | 0x2F78 | - |
| MW_MAIN_1090.cpp | `assignedMechUnreadyText` | 0x0FF7 | - |
| MW_MAIN_1090.cpp | `barMenuLabels` | 0x9013 | - |
| MW_MAIN_1090.cpp | `calendarMonthText` | 0x4AAC | - |
| MW_MAIN_1090.cpp | `campaignDamageTierLabels` | 0x2B23 | - |
| MW_MAIN_1090.cpp | `cannotAffordRepairText` | 0x2BB0 | - |
| MW_MAIN_1090.cpp | `commanderNeedsMechText` | 0x0FC0 | - |
| MW_MAIN_1090.cpp | `commanderProfileLabels` | 0x2283 | - |
| MW_MAIN_1090.cpp | `commanderProfileName` | 0x21C2 | - |
| MW_MAIN_1090.cpp | `commanderReputationLabels` | 0x23D0 | - |
| MW_MAIN_1090.cpp | `companyMenuLabels` | 0x21EB | - |
| MW_MAIN_1090.cpp | `contractAcceptedText` | 0x4C6C | - |
| MW_MAIN_1090.cpp | `contractCancelledText` | 0x1031 | - |
| MW_MAIN_1090.cpp | `crewLabels` | 0x2348 | - |
| MW_MAIN_1090.cpp | `dealerPurchaseLimitText` | 0x258A | - |
| MW_MAIN_1090.cpp | `dropShipStatusLabels` | 0x2D1C | - |
| MW_MAIN_1090.cpp | `fullFourPilotRosterText` | 0x9C5A | - |
| MW_MAIN_1090.cpp | `fullThreePilotRosterText` | 0x9C8D | - |
| MW_MAIN_1090.cpp | `marketBodyCondition` | 0x4685 | - |
| MW_MAIN_1090.cpp | `marketComponentDone` | 0x47F5 | - |
| MW_MAIN_1090.cpp | `marketComponentTitle` | 0x4536 | - |
| MW_MAIN_1090.cpp | `marketDropShipStatusLabels` | 0x4459 | - |
| MW_MAIN_1090.cpp | `marketListDoneText` | 0x3049 | - |
| MW_MAIN_1090.cpp | `marketListHeading` | 0x2922 | - |
| MW_MAIN_1090.cpp | `marketMechStatusLabels` | 0x439B | - |
| MW_MAIN_1090.cpp | `marketWeaponHeading` | 0x46A2 | - |
| MW_MAIN_1090.cpp | `mechComplexLabels` | 0x25E4 | - |
| MW_MAIN_1090.cpp | `mechNonfunctionalTextBytes` | 0x2B55 | - |
| MW_MAIN_1090.cpp | `mechStatusLabels` | 0x2C5B | - |
| MW_MAIN_1090.cpp | `negotiationTerminatedText` | 0x4C25 | - |
| MW_MAIN_1090.cpp | `noAvailableRecruitsText` | 0x9C39 | - |
| MW_MAIN_1090.cpp | `noContractsText` | 0x4BFA | - |
| MW_MAIN_1090.cpp | `noMarketMechsText` | 0x31BB | - |
| MW_MAIN_1090.cpp | `noRepairWorkText` | 0x2BE7 | - |
| MW_MAIN_1090.cpp | `ownedListDoneText` | 0x302F | - |
| MW_MAIN_1090.cpp | `ownedMechCapacityText` | 0x4347 | - |
| MW_MAIN_1090.cpp | `ownedUnitListHeading` | 0x2696 | - |
| MW_MAIN_1090.cpp | `pilotSkillLabels` | 0x2452 | - |
| MW_MAIN_1090.cpp | `positionedText` | 0x55EC | - |
| MW_MAIN_1090.cpp | `purchaseInsufficientFundsText` | 0x4330 | - |
| MW_MAIN_1090.cpp | `rearmConfirmationLabels` | 0x312B | - |
| MW_MAIN_1090.cpp | `recruitInterviewLabels` | 0x9091 | - |
| MW_MAIN_1090.cpp | `recruitStatLabels` | 0x9106 | - |
| MW_MAIN_1090.cpp | `repairBodyCondition` | 0x33A8 | - |
| MW_MAIN_1090.cpp | `repairCompleteText` | 0x2C01 | - |
| MW_MAIN_1090.cpp | `repairFailedText` | 0x3CB5 | - |
| MW_MAIN_1090.cpp | `repairFooter` | 0x33C0 | - |
| MW_MAIN_1090.cpp | `repairIncompleteText` | 0x2C21 | - |
| MW_MAIN_1090.cpp | `repairJumpJetCountTemplateBytes` | 0x36A5 | - |
| MW_MAIN_1090.cpp | `repairNoJumpJetsTextBytes` | 0x36A0 | - |
| MW_MAIN_1090.cpp | `repairTitle` | 0x3254 | - |
| MW_MAIN_1090.cpp | `reserveAmmoBuyLabel` | 0x413A | - |
| MW_MAIN_1090.cpp | `reserveAmmoSellLabel` | 0x413F | - |
| MW_MAIN_1090.cpp | `saleConfirmationLabels` | 0x3090 | - |
| MW_MAIN_1090.cpp | `submitContractCaption` | 0x4B35 | - |
| MW_MAIN_1090.cpp | `travelBrowseLabel` | 0x5C75 | - |
| MW_MAIN_1090.cpp | `travelConfirmLabel` | 0x5C17 | - |
| MW_MAIN_1090.cpp | `travelCostLabel` | 0x5BF9 | - |
| MW_MAIN_1090.cpp | `travelDestinationHeaderLabels` | 0x5C35 | - |
| MW_MAIN_1090.cpp | `travelHouseBrowserLabels` | 0x5A0D | - |
| MW_MAIN_1090.cpp | `travelHousePlanetCancelLabel` | 0x57FD | - |
| MW_MAIN_1090.cpp | `travelHouseSearchCaption` | 0x5534 | - |
| MW_MAIN_1090.cpp | `travelInsufficientFundsText` | 0x5B5A | - |
| MW_MAIN_1090.cpp | `travelLandscapeLabels` | 0x5CA6 | - |
| MW_MAIN_1090.cpp | `travelPopulationSuffix` | 0x5B83 | - |
| MW_MAIN_1090.cpp | `unassignedMechWarriorText` | 0x3024 | - |
| MW_MAIN_1090_OPTIONS.cpp | `audioOptionLabels` | 0xA071 | - |
| MW_MAIN_1090_OPTIONS.cpp | `loadDiskErrorText` | 0xA0E3 | - |
| MW_MAIN_1090_OPTIONS.cpp | `newsAtEnd` | 0x2562 | - |
| MW_MAIN_1090_OPTIONS.cpp | `newsAtStart` | 0x2562 | - |
| MW_MAIN_1090_OPTIONS.cpp | `newsBoundaryText` | 0x2564 | - |
| MW_MAIN_1090_OPTIONS.cpp | `restoreCancel` | 0x9F6C | - |
| MW_MAIN_1090_OPTIONS.cpp | `restoreHeading` | 0x9E79 | - |
| MW_MAIN_1090_OPTIONS.cpp | `saveDiskErrorText` | 0xA0B3 | - |
| MW_MAIN_1090_OPTIONS.cpp | `saveDiskFullText` | 0xA088 | - |
| MW_MAIN_1090_OPTIONS.cpp | `saveNamePrompt` | 0xA031 | - |
| MW_MAIN_1090_OPTIONS.cpp | `terrainDetailLabels` | 0xA079 | - |

## Ours: screen layout, panels and menus (data/ui/*.json)

| File | Item | Offset | From |
|---|---|---|---|
| BTECH_304E_COCKPIT.cpp | `inspectionDiagramOrigins` | 0x1878 | - |
| BTECH_304E_COCKPIT.cpp | `mechCockpitLayoutIndices` | 0x1D38 | - |
| BTECH_304E_COCKPIT.cpp | `subsystemLampColourTables` | 0x23CE | - |
| BTECH_304E_COCKPIT.cpp | `targetDiagramAnchors` | 0x1AD8 | - |
| BTECH_304E_ORDERS.cpp | `orderEngineConditionColours` | 0x030E | - |
| BTECH_304E_ORDERS.cpp | `orderMapPositionMask` | 0x4EF0 | - |
| BTECH_304E_ORDERS.cpp | `orderNavigationPanelDeltas` | 0x009A | - |
| BTECH_304E_ORDERS.cpp | `orderNavigationRowDeltas` | 0x00B0 | - |
| BTECH_304E_ORDERS.cpp | `orderSelectionFirstRows` | 0x0092 | - |
| BTECH_304E_ORDERS.cpp | `orderSelectionLastRows` | 0x0096 | - |
| BTECH_304E_ORDERS.cpp | `orderSystemConditionColours` | 0x031E | - |
| MW_MAIN_1090.cpp | `afterActionRosterRowOffset` | 0x1229 | - |
| MW_MAIN_1090.cpp | `barMenuPanel` | 0x8FF6 | - |
| MW_MAIN_1090.cpp | `bodyDamageTierFillColours` | 0x3CD2 | - |
| MW_MAIN_1090.cpp | `calendarMonthNameWidths` | 0x11BA | - |
| MW_MAIN_1090.cpp | `commanderProfilePanel` | 0x2258 | - |
| MW_MAIN_1090.cpp | `companyMenuPanel` | 0x21CE | - |
| MW_MAIN_1090.cpp | `componentRepairPanel` | 0x3214 | - |
| MW_MAIN_1090.cpp | `contractDateYearXOffsets` | 0x4B1A | - |
| MW_MAIN_1090.cpp | `crewLabelsPanel` | 0x2325 | - |
| MW_MAIN_1090.cpp | `dropShipStatusPanel` | 0x2D06 | - |
| MW_MAIN_1090.cpp | `dropShipTravelFrames` | 0x5B2E | - |
| MW_MAIN_1090.cpp | `hiredTransportFrames` | 0x5AEA | - |
| MW_MAIN_1090.cpp | `marketComponentPanel` | 0x44F1 | - |
| MW_MAIN_1090.cpp | `marketDropShipStatusPanel` | 0x443E | - |
| MW_MAIN_1090.cpp | `marketMechStatusPanel` | 0x437E | - |
| MW_MAIN_1090.cpp | `marketSystemConditionRows` | 0x454F | - |
| MW_MAIN_1090.cpp | `marketUnitListPanel` | 0x28F1 | - |
| MW_MAIN_1090.cpp | `marketUnitListRows` | 0x2931 | - |
| MW_MAIN_1090.cpp | `marketWeaponConditionRows` | 0x46BA | - |
| MW_MAIN_1090.cpp | `mechComplexAnimationFrames` | 0x318D | - |
| MW_MAIN_1090.cpp | `mechComplexPanel` | 0x25C5 | - |
| MW_MAIN_1090.cpp | `mechStatusPanel` | 0x2C3F | - |
| MW_MAIN_1090.cpp | `ownedUnitListPanel` | 0x266A | - |
| MW_MAIN_1090.cpp | `ownedUnitListRows` | 0x26B5 | - |
| MW_MAIN_1090.cpp | `pilotCardColumnOrigins` | 0x244A | - |
| MW_MAIN_1090.cpp | `rearmConfirmationPanel` | 0x3110 | - |
| MW_MAIN_1090.cpp | `recruitInterviewPanel` | 0x9076 | - |
| MW_MAIN_1090.cpp | `repairSystemConditionRows` | 0x3272 | - |
| MW_MAIN_1090.cpp | `repairTitleLeadingSpacesByKind` | 0x31F2 | - |
| MW_MAIN_1090.cpp | `repairWeaponConditionRows` | 0x3403 | - |
| MW_MAIN_1090.cpp | `reserveAmmoMechColours` | 0x3F70 | - |
| MW_MAIN_1090.cpp | `saleConfirmationPanel` | 0x3075 | - |
| MW_MAIN_1090.cpp | `travelBrowsePanel` | 0x5C75 | - |
| MW_MAIN_1090.cpp | `travelConfirmPanel` | 0x5C17 | - |
| MW_MAIN_1090.cpp | `travelCostPanel` | 0x5BF9 | - |
| MW_MAIN_1090.cpp | `travelDestinationHeaderPanel` | 0x5C35 | - |
| MW_MAIN_1090.cpp | `travelHouseBrowserPanel` | 0x5A0D | - |
| MW_MAIN_1090.cpp | `unitDetailPictureXOffsets` | 0x2B83 | - |
| MW_MAIN_1090.cpp | `unitDetailPictureYOrigins` | 0x2B99 | - |
| MW_MAIN_1090_OPTIONS.cpp | `confirmationMenu` | 0x9D34 | - |
| MW_MAIN_1090_OPTIONS.cpp | `confirmationPanel` | 0x9CBE | - |
| MW_MAIN_1090_OPTIONS.cpp | `newsMenu` | 0x2538 | - |
| MW_MAIN_1090_OPTIONS.cpp | `newsMenuPanel` | 0x9CBE | - |
| MW_MAIN_1090_OPTIONS.cpp | `optionsMenu` | 0x9DEB | - |
| MW_MAIN_1090_OPTIONS.cpp | `optionsPanel` | 0x9D52 | - |
| MW_MAIN_1090_OPTIONS.cpp | `restoreMenu` | 0x9F7B | - |
| MW_MAIN_1090_OPTIONS.cpp | `restorePanel` | 0x9D52 | - |

## Ours: rules and constants (data/rules/*.json)

| File | Item | Offset | From |
|---|---|---|---|
| BTECH_304E.cpp | `actuatorIndicesByBodyLocation` | BTECH 304E:0E5E | rule combat.actuator_of_limb |
| BTECH_304E.cpp | `actuatorIntegrityTierByCondition` | BTECH 304E:09DC | rule combat.actuator_integrity_by_condition |
| BTECH_304E.cpp | `actuatorLossByDamageTier` | BTECH 304E:08D0 | rule combat.actuator_loss_by_tier |
| BTECH_304E.cpp | `armorToBodyLocations` | BTECH 304E:0E3A | rule combat.body_of_armour |
| BTECH_304E.cpp | `campaignBodyTierByArmorLocation` | BTECH 304E:08AE | rule combat.campaign_body_tier_of_armour |
| BTECH_304E.cpp | `campaignUnitKindByCombatTemplate` | BTECH 304E:09CC | rule combat.unit_kind_of_template |
| BTECH_304E.cpp | `combatContinuationProfiles` | BTECH 304E:08F6 | rule combat.continuation_profiles |
| BTECH_304E.cpp | `combatTemplateByCampaignUnitKind` | BTECH 304E:08E2 | rule combat.template_of_unit_kind |
| BTECH_304E.cpp | `criticalTransferBodyLocations` | BTECH 304E:0E28 | rule combat.critical_transfer |
| BTECH_304E.cpp | `damageTransferArmorLocations` | BTECH 304E:0E4C | rule combat.damage_transfer |
| BTECH_304E.cpp | `heatAimJitterThresholds` | BTECH 304E:271E | rule combat.heat_aim_jitter_thresholds |
| BTECH_304E.cpp | `mechFallbackStartTemplates` | BTECH 304E:0982 | rule combat.fallback_start_template |
| BTECH_304E.cpp | `mechFallbackTemplateSteps` | BTECH 304E:0992 | rule combat.fallback_template_steps |
| BTECH_304E.cpp | `mechTemplateBudgetCostsBytes` | BTECH 304E:09A2 | rule combat.template_budget_costs |
| BTECH_304E.cpp | `mechTemplates` | BTECH 304E:1178 | rule mechs.templates |
| BTECH_304E.cpp | `missionRules` | BTECH 304E:092C | rule combat.mission_side_zero_rules |
| BTECH_304E.cpp | `opposingObjectiveRules` | BTECH 304E:08D8 | rule combat.opposing_objective |
| BTECH_304E.cpp | `systemDamageTierByHits` | BTECH 304E:09E6 | rule combat.system_tier_by_hits |
| BTECH_304E.cpp | `systemHitsByDamageTier` | BTECH 304E:08C8 | rule combat.system_hits_by_tier |
| BTECH_304E.cpp | `weaponDefinitions` | BTECH 304E:0EEC | rule weapons.definitions |
| BTECH_304E.cpp | `weaponDisabledByDamageTier` | BTECH 304E:08C0 | rule combat.weapon_disabled_by_tier |
| BTECH_304E.cpp | `weaponIntegrityTierByOperational` | BTECH 304E:09EC | rule combat.weapon_integrity_by_operational |
| BTECH_304E_POSE.cpp | `animationPrefixSequenceCounts` | BTECH 304E:24B4 | rule assets.animation_prefix_counts |
| BTECH_304E_POSE.cpp | `playerPoseFrameCounts` | BTECH 304E:265E | rule assets.player_pose_frame_counts |
| BTECH_304E_WORLD.cpp | `avoidanceHeadingOffsets` | BTECH 304E:2438 | rule combat.avoidance_heading_offsets |
| BTECH_304E_WORLD.cpp | `bodyDamageEffectResourceDeltas` | BTECH 304E:2342 | rule assets.body_damage_effect_steps |
| BTECH_304E_WORLD.cpp | `bodyLocationReferenceIndices` | BTECH 304E:0EBC | rule combat.body_reference_point |
| BTECH_304E_WORLD.cpp | `defaultEffectResourceDeltas` | BTECH 304E:0E96 | rule assets.default_effect_steps |
| BTECH_304E_WORLD.cpp | `hitLocationRows` | BTECH 304E:26BC | rule combat.hit_locations |
| BTECH_304E_WORLD.cpp | `machineGunEffectResourceDeltas` | BTECH 304E:0EA4 | rule assets.machine_gun_effect_steps |
| BTECH_304E_WORLD.cpp | `missileClusterRowByWeapon` | BTECH 304E:234A | rule weapons.missile_cluster_rows |
| BTECH_304E_WORLD.cpp | `missileClusterRows` | BTECH 304E:235E | rule combat.missile_cluster_rows |
| BTECH_304E_WORLD.cpp | `opposingSideFirstUnitSlots` | BTECH 304E:2434 | rule combat.opposing_side_first_unit_slots |
| BTECH_304E_WORLD.cpp | `projectileProfileIndicesByWeapon` | BTECH 304E:232E | rule weapons.projectile_profiles |
| BTECH_304E_WORLD.cpp | `rangeHitModifiers` | BTECH 304E:2714 | rule combat.range_hit_modifiers |
| MW_MAIN_1090.cpp | `ammunitionBasePrices` | MW_MAIN 1090:2EF8 | rule market.ammunition_prices |
| MW_MAIN_1090.cpp | `ammunitionTransactionPrices` | MW_MAIN 1090:2F10 | rule market.start.ammunition_transaction_prices |
| MW_MAIN_1090.cpp | `bodyRepairBasePricesByKind` | MW_MAIN 1090:3CAB | rule units.body_repair_prices |
| MW_MAIN_1090.cpp | `campaignMechTonnages` | MW_MAIN 1090:122B | rule units.tonnages |
| MW_MAIN_1090.cpp | `defaultHouseMissionTypeLimits` | MW_MAIN 1090:5CBA | rule houses.mission_type_limits |
| MW_MAIN_1090.cpp | `generateStartingJenner` | MW_MAIN 1090:3A6A | rule market.start.generate_starting_jenner |
| MW_MAIN_1090.cpp | `generatedAmmoKinds` | MW_MAIN 1090:3A5E | rule market.start.generated_ammo_kinds |
| MW_MAIN_1090.cpp | `generatedAmmoQuantities` | MW_MAIN 1090:3A64 | rule market.start.generated_ammo_quantities |
| MW_MAIN_1090.cpp | `generatedKindRangeMaxima` | MW_MAIN 1090:3A37 | rule market.stock_kind_range_maxima |
| MW_MAIN_1090.cpp | `generatedKindRangeStarts` | MW_MAIN 1090:3A3A | rule market.stock_kind_range_starts |
| MW_MAIN_1090.cpp | `houseAdvanceBasePercent` | MW_MAIN 1090:4B69 | rule houses.advance_base_percent |
| MW_MAIN_1090.cpp | `houseDeductionRollThresholds` | MW_MAIN 1090:1241 | rule houses.deduction_roll_thresholds |
| MW_MAIN_1090.cpp | `houseExtraEnemyRollThresholds` | MW_MAIN 1090:4BF0 | rule houses.extra_enemy_roll_thresholds |
| MW_MAIN_1090.cpp | `houseMissionPlanetPolicies` | MW_MAIN 1090:8E9A | rule houses.mission_planet_policies |
| MW_MAIN_1090.cpp | `houseMissionPlanetTableOffsets` | MW_MAIN 1090:8E90 | rule houses.mission_planet_policy_offsets |
| MW_MAIN_1090.cpp | `houseNegotiationAttemptBase` | MW_MAIN 1090:11A3 | rule houses.negotiation_attempt_base |
| MW_MAIN_1090.cpp | `housePaymentAdjustmentPercent` | MW_MAIN 1090:4B4B | rule houses.payment_adjustment_percent |
| MW_MAIN_1090.cpp | `housePaymentRollBias` | MW_MAIN 1090:4B41 | rule houses.payment_roll_bias |
| MW_MAIN_1090.cpp | `housePlanetRangeCounts` | MW_MAIN 1090:6566 | rule houses.planet_range_counts |
| MW_MAIN_1090.cpp | `housePlanetRangeStarts` | MW_MAIN 1090:655C | rule houses.planet_range_starts |
| MW_MAIN_1090.cpp | `houseReducedPayoutPercentages` | MW_MAIN 1090:124B | rule houses.reduced_payout_percent |
| MW_MAIN_1090.cpp | `houseSalvageBasePercent` | MW_MAIN 1090:4B5F | rule houses.salvage_base_percent |
| MW_MAIN_1090.cpp | `missionDeploymentUnitsByScenario` | MW_MAIN 1090:8E46 | rule contracts.deployment_units_by_scenario |
| MW_MAIN_1090.cpp | `missionForceWeightByUnitKind` | MW_MAIN 1090:4B55 | rule units.force_weights |
| MW_MAIN_1090.cpp | `pilotWageAmounts` | MW_MAIN 1090:2476 | rule contracts.pilot_wages |
| MW_MAIN_1090.cpp | `repairSuccessRollLimitsByMarketTier` | MW_MAIN 1090:3A6B | rule repair.success_roll_limits |
| MW_MAIN_1090.cpp | `reserveAmmoFreightRates` | MW_MAIN 1090:5C94 | rule market.freight_rates |
| MW_MAIN_1090.cpp | `systemRepairBasePrices` | MW_MAIN 1090:3B53 | rule repair.system_prices |
| MW_MAIN_1090.cpp | `unitAmmoGroupCounts` | MW_MAIN 1090:2EEE | rule units.ammo_groups |
| MW_MAIN_1090.cpp | `unitAmmoProfileEntries` | MW_MAIN 1090:2F3C | rule units.ammo_profile_entries |
| MW_MAIN_1090.cpp | `unitAmmoProfiles` | MW_MAIN 1090:2F28 | rule units.ammo_profiles |
| MW_MAIN_1090.cpp | `unitBasePrices` | MW_MAIN 1090:305F | rule units.base_prices |
| MW_MAIN_1090.cpp | `unitConditionScoreDivisors` | MW_MAIN 1090:2B18 | rule units.condition_score_divisors |
| MW_MAIN_1090.cpp | `unitConditionSystemFieldCounts` | MW_MAIN 1090:2AE3 | rule units.condition_fields |
| MW_MAIN_1090.cpp | `unitDisplayJumpCapacitiesMetres` | MW_MAIN 1090:2B79 | rule units.display_jumps |
| MW_MAIN_1090.cpp | `unitDisplaySpeedsKph` | MW_MAIN 1090:2B6F | rule units.display_speeds |
| MW_MAIN_1090.cpp | `unitDisplayTonnages` | MW_MAIN 1090:2B65 | rule units.display_tonnages |
| MW_MAIN_1090.cpp | `unitHeatSinkCounts` | MW_MAIN 1090:368C | rule units.heat_sinks |
| MW_MAIN_1090.cpp | `unitJumpJetGroupCounts` | MW_MAIN 1090:3696 | rule units.jump_jet_groups |
| MW_MAIN_1090.cpp | `unitPriceTierMultipliers` | MW_MAIN 1090:3057 | rule market.price_tier_multipliers |
| MW_MAIN_1090.cpp | `unitWeaponCounts` | MW_MAIN 1090:2AED | rule units.weapons |
| MW_MAIN_1090.cpp | `weaponRepairPrices` | MW_MAIN 1090:3C2F | rule units.weapon_repair_prices |
| MW_MAIN_1090.cpp | `weaponRepairPricesByKind` | MW_MAIN 1090:3C1B | rule units.weapon_repair_price_lists |

## Ours: the new campaign's starting state (data/rules/new_campaign.json)

| File | Item | Offset | From |
|---|---|---|---|
| MW_MAIN_1090.cpp | `acceptedContractActive` | MW_MAIN 1090:0468 | new_campaign.json acceptedContractActive |
| MW_MAIN_1090.cpp | `acceptedContractAdvancePercent` | MW_MAIN 1090:09FE | new_campaign.json acceptedContractAdvancePercent |
| MW_MAIN_1090.cpp | `acceptedContractPaymentThousands` | MW_MAIN 1090:09FA | new_campaign.json acceptedContractPaymentThousands |
| MW_MAIN_1090.cpp | `acceptedContractSalvagePercent` | MW_MAIN 1090:09FC | new_campaign.json acceptedContractSalvagePercent |
| MW_MAIN_1090.cpp | `acceptedContractTargetHouseIndex` | MW_MAIN 1090:0B53 | new_campaign.json acceptedContractTargetHouseIndex |
| MW_MAIN_1090.cpp | `audioOptionLabelIndex` | MW_MAIN 1090:0A2A | new_campaign.json audioOptionLabelIndex |
| MW_MAIN_1090.cpp | `availableRecruitCount` | MW_MAIN 1090:051E | new_campaign.json availableRecruitCount |
| MW_MAIN_1090.cpp | `availableRecruitIds` | MW_MAIN 1090:0520 | new_campaign.json availableRecruitIds |
| MW_MAIN_1090.cpp | `calendarMonthIndex` | MW_MAIN 1090:047B | new_campaign.json calendarMonthIndex |
| MW_MAIN_1090.cpp | `calendarSubmonthUnits` | MW_MAIN 1090:0479 | new_campaign.json calendarSubmonthUnits |
| MW_MAIN_1090.cpp | `calendarYear` | MW_MAIN 1090:047D | new_campaign.json calendarYear |
| MW_MAIN_1090.cpp | `campaignEventPlanetSelectors` | MW_MAIN 1090:0B46 | new_campaign.json campaignEventPlanetSelectors |
| MW_MAIN_1090.cpp | `campaignFundsHigh` | MW_MAIN 1090:0493 | new_campaign.json campaignFundsHigh |
| MW_MAIN_1090.cpp | `campaignFundsLow` | MW_MAIN 1090:0491 | new_campaign.json campaignFundsLow |
| MW_MAIN_1090.cpp | `campaignTerrainDetailLevel` | MW_MAIN 1090:0B55 | new_campaign.json campaignTerrainDetailLevel |
| MW_MAIN_1090.cpp | `combatEnemyHeavyCount` | MW_MAIN 1090:044B | new_campaign.json combatEnemyHeavyCount |
| MW_MAIN_1090.cpp | `combatEnemyLightCount` | MW_MAIN 1090:044D | new_campaign.json combatEnemyLightCount |
| MW_MAIN_1090.cpp | `combatEnemyMediumCount` | MW_MAIN 1090:044C | new_campaign.json combatEnemyMediumCount |
| MW_MAIN_1090.cpp | `combatPlanetSelector` | MW_MAIN 1090:044E | new_campaign.json combatPlanetSelector |
| MW_MAIN_1090.cpp | `combatScenarioCode` | MW_MAIN 1090:044F | new_campaign.json combatScenarioCode |
| MW_MAIN_1090.cpp | `combatTransitionResult` | MW_MAIN 1090:0467 | new_campaign.json combatTransitionResult |
| MW_MAIN_1090.cpp | `commanderAgeYears` | MW_MAIN 1090:048F | new_campaign.json commanderAgeYears |
| MW_MAIN_1090.cpp | `commanderNameBytes` | MW_MAIN 1090:0483 | new_campaign.json commanderNameBytes |
| MW_MAIN_1090.cpp | `contractNegotiationAttemptLimit` | MW_MAIN 1090:09E6 | new_campaign.json contractNegotiationAttemptLimit |
| MW_MAIN_1090.cpp | `currentPlanetFacilityDisplayFlag` | MW_MAIN 1090:0471 | new_campaign.json currentPlanetFacilityDisplayFlag |
| MW_MAIN_1090.cpp | `currentPlanetHouseIndex` | MW_MAIN 1090:046D | new_campaign.json currentPlanetHouseIndex |
| MW_MAIN_1090.cpp | `currentPlanetIndex` | MW_MAIN 1090:0469 | new_campaign.json currentPlanetIndex |
| MW_MAIN_1090.cpp | `currentPlanetLandscapeIndex` | MW_MAIN 1090:046B | new_campaign.json currentPlanetLandscapeIndex |
| MW_MAIN_1090.cpp | `currentPlanetMapPositionX` | MW_MAIN 1090:0473 | new_campaign.json currentPlanetMapPositionX |
| MW_MAIN_1090.cpp | `currentPlanetMapPositionY` | MW_MAIN 1090:0475 | new_campaign.json currentPlanetMapPositionY |
| MW_MAIN_1090.cpp | `currentPlanetMarketTier` | MW_MAIN 1090:046F | new_campaign.json currentPlanetMarketTier |
| MW_MAIN_1090.cpp | `darkWingBaseVictoryRecorded` | MW_MAIN 1090:0B5C | new_campaign.json darkWingBaseVictoryRecorded |
| MW_MAIN_1090.cpp | `dealerUnitPurchaseLimit` | MW_MAIN 1090:0B56 | new_campaign.json dealerUnitPurchaseLimit |
| MW_MAIN_1090.cpp | `deploymentLandscapeIndex` | MW_MAIN 1090:044A | new_campaign.json deploymentLandscapeIndex |
| MW_MAIN_1090.cpp | `deploymentTerrainDetailLevel` | MW_MAIN 1090:0450 | new_campaign.json deploymentTerrainDetailLevel |
| MW_MAIN_1090.cpp | `dropShipMissedUpkeepPayments` | MW_MAIN 1090:051A | new_campaign.json dropShipMissedUpkeepPayments |
| MW_MAIN_1090.cpp | `elapsedBillingMonths` | MW_MAIN 1090:0481 | new_campaign.json elapsedBillingMonths |
| MW_MAIN_1090.cpp | `eventDirectoryIndex` | MW_MAIN 1090:0AB8 | new_campaign.json eventDirectoryIndex |
| MW_MAIN_1090.cpp | `eventSeenFlags` | MW_MAIN 1090:0ABA | new_campaign.json eventSeenFlags |
| MW_MAIN_1090.cpp | `houseFavorPoints` | MW_MAIN 1090:049F | new_campaign.json houseFavorPoints |
| MW_MAIN_1090.cpp | `houseGrievancePoints` | MW_MAIN 1090:04A9 | new_campaign.json houseGrievancePoints |
| MW_MAIN_1090.cpp | `houseMissionTypeLimits` | MW_MAIN 1090:0A02 | new_campaign.json houseMissionTypeLimits |
| MW_MAIN_1090.cpp | `houseNetStandings` | MW_MAIN 1090:0495 | new_campaign.json houseNetStandings |
| MW_MAIN_1090.cpp | `launcherMemoryTier` | MW_MAIN 1090:0B5D | new_campaign.json launcherMemoryTier |
| MW_MAIN_1090.cpp | `majorCampaignMissionRollThreshold` | MW_MAIN 1090:0B52 | new_campaign.json majorCampaignMissionRollThreshold |
| MW_MAIN_1090.cpp | `marketAmmoKindsByGroup` | MW_MAIN 1090:078A | new_campaign.json marketAmmoKindsByGroup |
| MW_MAIN_1090.cpp | `marketAmmoQuantitiesByGroup` | MW_MAIN 1090:06EE | new_campaign.json marketAmmoQuantitiesByGroup |
| MW_MAIN_1090.cpp | `marketRefreshUnits` | MW_MAIN 1090:047F | new_campaign.json marketRefreshUnits |
| MW_MAIN_1090.cpp | `marketUnitCount` | MW_MAIN 1090:07D2 | new_campaign.json marketUnitCount |
| MW_MAIN_1090.cpp | `marketUnitDamage` | MW_MAIN 1090:07EC | new_campaign.json marketUnitDamage |
| MW_MAIN_1090.cpp | `marketUnitKinds` | MW_MAIN 1090:07D4 | new_campaign.json marketUnitKinds |
| MW_MAIN_1090.cpp | `missionAdvancePercentageCeilings` | MW_MAIN 1090:09DA | new_campaign.json missionAdvancePercentageCeilings |
| MW_MAIN_1090.cpp | `missionAdvancePercentages` | MW_MAIN 1090:09B6 | new_campaign.json missionAdvancePercentages |
| MW_MAIN_1090.cpp | `missionCategoryOrMemoryTier` | MW_MAIN 1090:0449 | new_campaign.json missionCategoryOrMemoryTier |
| MW_MAIN_1090.cpp | `missionDescriptionOffsets` | MW_MAIN 1090:094A | new_campaign.json missionDescriptionOffsets |
| MW_MAIN_1090.cpp | `missionEnemyHeavyCounts` | MW_MAIN 1090:097A | new_campaign.json missionEnemyHeavyCounts |
| MW_MAIN_1090.cpp | `missionEnemyLightCounts` | MW_MAIN 1090:0992 | new_campaign.json missionEnemyLightCounts |
| MW_MAIN_1090.cpp | `missionEnemyMediumCounts` | MW_MAIN 1090:0986 | new_campaign.json missionEnemyMediumCounts |
| MW_MAIN_1090.cpp | `missionOfferCount` | MW_MAIN 1090:0948 | new_campaign.json missionOfferCount |
| MW_MAIN_1090.cpp | `missionPaymentCeilingsThousands` | MW_MAIN 1090:09C2 | new_campaign.json missionPaymentCeilingsThousands |
| MW_MAIN_1090.cpp | `missionPaymentThousands` | MW_MAIN 1090:099E | new_campaign.json missionPaymentThousands |
| MW_MAIN_1090.cpp | `missionPlanetRecordIndices` | MW_MAIN 1090:0956 | new_campaign.json missionPlanetRecordIndices |
| MW_MAIN_1090.cpp | `missionSalvagePercentageCeilings` | MW_MAIN 1090:09CE | new_campaign.json missionSalvagePercentageCeilings |
| MW_MAIN_1090.cpp | `missionSalvagePercentages` | MW_MAIN 1090:09AA | new_campaign.json missionSalvagePercentages |
| MW_MAIN_1090.cpp | `missionScenarioCodes` | MW_MAIN 1090:0A2B | new_campaign.json missionScenarioCodes |
| MW_MAIN_1090.cpp | `missionTargetHouseIndices` | MW_MAIN 1090:0962 | new_campaign.json missionTargetHouseIndices |
| MW_MAIN_1090.cpp | `missionTypeCodes` | MW_MAIN 1090:096E | new_campaign.json missionTypeCodes |
| MW_MAIN_1090.cpp | `ownedAmmoKindsByGroup` | MW_MAIN 1090:0742 | new_campaign.json ownedAmmoKindsByGroup |
| MW_MAIN_1090.cpp | `ownedAmmoQuantitiesByGroup` | MW_MAIN 1090:06A6 | new_campaign.json ownedAmmoQuantitiesByGroup |
| MW_MAIN_1090.cpp | `ownedUnitCount` | MW_MAIN 1090:0530 | new_campaign.json ownedUnitCount |
| MW_MAIN_1090.cpp | `ownedUnitDamage` | MW_MAIN 1090:054A | new_campaign.json ownedUnitDamage |
| MW_MAIN_1090.cpp | `ownedUnitKinds` | MW_MAIN 1090:0532 | new_campaign.json ownedUnitKinds |
| MW_MAIN_1090.cpp | `pilotDepartureFlagTail` | MW_MAIN 1090:04E9 | new_campaign.json pilotDepartureFlagTail |
| MW_MAIN_1090.cpp | `pilotExperienceCounts` | MW_MAIN 1090:0A8D | new_campaign.json pilotExperienceCounts |
| MW_MAIN_1090.cpp | `pilotGunneryTiers` | MW_MAIN 1090:0A37 | new_campaign.json pilotGunneryTiers |
| MW_MAIN_1090.cpp | `pilotIdentityStorage` | MW_MAIN 1090:04BF | new_campaign.json pilotIdentityStorage |
| MW_MAIN_1090.cpp | `pilotPilotingTiers` | MW_MAIN 1090:0A62 | new_campaign.json pilotPilotingTiers |
| MW_MAIN_1090.cpp | `planetMarketPremiumPercent` | MW_MAIN 1090:0477 | new_campaign.json planetMarketPremiumPercent |
| MW_MAIN_1090.cpp | `playerCombatCredit` | MW_MAIN 1090:04B3 | new_campaign.json playerCombatCredit |
| MW_MAIN_1090.cpp | `playerSkillProgress` | MW_MAIN 1090:0B5A | new_campaign.json playerSkillProgress |
| MW_MAIN_1090.cpp | `remainingContractPaymentThousands` | MW_MAIN 1090:0A00 | new_campaign.json remainingContractPaymentThousands |
| MW_MAIN_1090.cpp | `requestContractAdvanceEnabled` | MW_MAIN 1090:09F2 | new_campaign.json requestContractAdvanceEnabled |
| MW_MAIN_1090.cpp | `requestContractPaymentEnabled` | MW_MAIN 1090:09EE | new_campaign.json requestContractPaymentEnabled |
| MW_MAIN_1090.cpp | `requestContractSalvageEnabled` | MW_MAIN 1090:09F0 | new_campaign.json requestContractSalvageEnabled |
| MW_MAIN_1090.cpp | `requestedContractAdvancePercent` | MW_MAIN 1090:09EC | new_campaign.json requestedContractAdvancePercent |
| MW_MAIN_1090.cpp | `requestedContractPaymentThousands` | MW_MAIN 1090:09E8 | new_campaign.json requestedContractPaymentThousands |
| MW_MAIN_1090.cpp | `requestedContractSalvagePercent` | MW_MAIN 1090:09EA | new_campaign.json requestedContractSalvagePercent |
| MW_MAIN_1090.cpp | `reserveAmmoQuantitiesByFamily` | MW_MAIN 1090:0736 | new_campaign.json reserveAmmoQuantitiesByFamily |
| MW_MAIN_1090.cpp | `returnedDisabledEnemyCount` | MW_MAIN 1090:0465 | new_campaign.json returnedDisabledEnemyCount |
| MW_MAIN_1090.cpp | `rosterGunneryTiers` | MW_MAIN 1090:04F2 | new_campaign.json rosterGunneryTiers |
| MW_MAIN_1090.cpp | `rosterMechAssignments` | MW_MAIN 1090:050A | new_campaign.json rosterMechAssignments |
| MW_MAIN_1090.cpp | `rosterPayTiers` | MW_MAIN 1090:04EA | new_campaign.json rosterPayTiers |
| MW_MAIN_1090.cpp | `rosterPilotingTiers` | MW_MAIN 1090:04FA | new_campaign.json rosterPilotingTiers |
| MW_MAIN_1090.cpp | `rosterPortraitIds` | MW_MAIN 1090:0512 | new_campaign.json rosterPortraitIds |
| MW_MAIN_1090.cpp | `rosterStatus` | MW_MAIN 1090:0451 | new_campaign.json rosterStatus |
| MW_MAIN_1090.cpp | `rosterUnknownWords` | MW_MAIN 1090:0502 | new_campaign.json rosterUnknownWords |
| MW_MAIN_1090.cpp | `salvageEntries` | MW_MAIN 1090:0455 | new_campaign.json salvageEntries |
| MW_MAIN_1090.cpp | `savedVideoMode` | MW_MAIN 1090:0B5F | new_campaign.json savedVideoMode |
| MW_MAIN_1090.cpp | `unidentifiedField18` | MW_MAIN 1090:0448 | new_campaign.json unidentifiedField18 |
| MW_MAIN_1090.cpp | `unidentifiedField19` | MW_MAIN 1090:048D | new_campaign.json unidentifiedField19 |
| MW_MAIN_1090.cpp | `unidentifiedField20` | MW_MAIN 1090:09F4 | new_campaign.json unidentifiedField20 |
| MW_MAIN_1090.cpp | `unitsSoldSinceMarketRefresh` | MW_MAIN 1090:0B58 | new_campaign.json unitsSoldSinceMarketRefresh |
| MW_MAIN_1090.cpp | `unpaidPilotWageMarker` | MW_MAIN 1090:051C | new_campaign.json unpaidPilotWageMarker |

## Dropped

| File | Item | Offset | From |
|---|---|---|---|
| BTECH_304E_VIDEO.cpp | `herculesDisplayRowOffsets` | 0x34DE | - |
| MW_0806_STATE.cpp | `launcherCampaignFilename` | 0x07E3 | - |
| MW_0806_STATE.cpp | `launcherCombatFilename` | 0x07EF | - |
| MW_0806_STATE.cpp | `launcherIdentity` | 0x07D4 | - |
| MW_MAIN_1090.cpp | `combatExecutableFilename` | 0x0EF6 | - |
| MW_MAIN_1090.cpp | `launcherExitCode` | 0x0006 | - |

## Code: runtime state and renderer tables (no file)

Not listed one by one: BTECH_0810_STATE.cpp 1, BTECH_1ADF_STATE.cpp 2, BTECH_1BF5_STATE.cpp 5, BTECH_22B6_DATA.cpp 15, BTECH_2FEB.cpp 24, BTECH_304E.cpp 35, BTECH_304E_ADLIB.cpp 25, BTECH_304E_COCKPIT.cpp 45, BTECH_304E_MPU.cpp 10, BTECH_304E_ORDERS.cpp 35, BTECH_304E_POLYGONS.cpp 24, BTECH_304E_POSE.cpp 16, BTECH_304E_RENDER.cpp 62, BTECH_304E_RUNTIME.cpp 17, BTECH_304E_SCENE.cpp 8, BTECH_304E_SOUND.cpp 42, BTECH_304E_STARTUP.cpp 59, BTECH_304E_VIDEO.cpp 87, BTECH_304E_WORLD.cpp 111, MW_0806_STATE.cpp 1, MW_EGA_0800_STATE.cpp 66, MW_MAIN_0F41_STATE.cpp 5, MW_MAIN_1090.cpp 329, MW_MAIN_1090_OPTIONS.cpp 21, MW_MAIN_1AA5.cpp 1, MW_MAIN_28BE.cpp 39.

## Reclassified or removed in slice 3

- `houseMissionPlanetTableOffsets` (MW_MAIN 1090:8E90) was listed as story content; it is the pointer table of the Houses' mission-planet policies, a rule (`houses.mission_planet_policy_offsets`).
- `repairAttemptFailuresByKind` (MW_MAIN 1090:3A77, 220 bytes) was listed as a rule; it is zero in the exe and written in play, so it is code, zeroed where the exe reloaded it.
- `unusedRepairTimeByDamageTier` (MW_MAIN 1090:3A6F, 8 bytes) was listed as a rule; it was imported and never read, and is gone.
- `combatMissionSideZeroRules` (BTECH 304E:092C, 74 bytes; an alias into `missionRules`) and `mechFallbackTemplateSteps` (BTECH 304E:0992, 16 bytes) were missing; both are rules.
- `mountedWeaponCountsByKind`, `operationalWeaponCountsByKind` (both BTECH 304E:4F32), `selectionObjectListIndices` (6F5A) and `scenarioSelectors` (7D6A) lie past BTECH's file data (304E:4EE0), so the exe holds zeros and play writes them: code.
- `actuatorConditionLabelIndices`, `engineConditionLabelIndices`, `twoHitSystemConditionLabelIndices` and `orderDisplayIndices` pick labels and move with them in slice 5: UI words.
- Eight start values are always written before they're read, and are code, zeroed where the exe reloaded them (all eight are zero in the exe): `settlementPayoutPercent` (1090:123F), `pilotCardGunneryTier`, `pilotCardPilotingTier`, `pilotCardQuotedWageTier` (2440, 2442, 2446), `missionForceBudget` (4B79), `missionPaymentBaseThousands` (4B7B), `travelDestinationTimeWeight` (54DA, written and never read) and `sideObjectiveRules` (BTECH 304E:0ED0). The other four, `ammunitionTransactionPrices`, `generateStartingJenner`, `generatedAmmoKinds` and `generatedAmmoQuantities`, are read first on some path and stay `"start"` entries in market.json.
- Two of the save codec's 105 fields, `pilotRosterCount` and `rosterPilotIds`, are owned in `src/Original/Campaign`, so this list (of `src/Original/Memory`) doesn't show them; new_campaign.json holds them.
