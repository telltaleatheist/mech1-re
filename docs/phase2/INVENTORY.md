# Phase 2 inventory

Generated from `src/Original/Memory` by `tools/phase2/inventory.py` (every file-scope item). An item read through a door shows where it now comes from: a story piece, a rule piece, a layout piece, a UI word, or a save codec field of `new_campaign.json`, with the exe offset those tables give. Any other item shows the first native offset its importer reads, or `-` for runtime state with no import; its group names the slice that moves it, and the story item not yet moved names its slice. An item an importer decodes from a native span of the story (`story/native.json`) shows the span and the exe offset it holds. The groups follow `docs/phase2/PLAN.md`. Names and offsets only: no MW1 text.

| Group | Items |
|---|---|
| Extracted: the original's creative content (the story file, first run) | 88 |
| Ours: UI words and system messages (story/words.json, imported) | 142 |
| Ours: screen layout, panels and menus (data/ui/*.json) | 126 |
| Ours: rules and constants (data/rules/*.json) | 83 |
| Ours: the new campaign's starting state (data/rules/new_campaign.json) | 103 |
| Dropped | 6 |
| Code: runtime state and renderer tables (no file) | 960 |

## Extracted: the original's creative content (the story file, first run)

| File | Item | Offset | From |
|---|---|---|---|
| BTECH_304E.cpp | `combatDefaultCommanderNameBytes` | BTECH 304E:0978 | story combat.commander.default_name |
| BTECH_304E_ADLIB.cpp | `combatAdlibDefaultInstruments` | - | story, MwStoryCopyAdlibInstruments |
| BTECH_304E_COCKPIT.cpp | `mechTemplateNames` | BTECH 304E:2045 | story combat.mech_template.1.name |
| BTECH_304E_COCKPIT.cpp | `missionTitleText` | 0x09F0 | story 2g, not yet moved |
| BTECH_304E_COCKPIT.cpp | `pilotDefinitionNameDirectory` | BTECH 304E:20AD | story combat.pilot.1.name |
| BTECH_304E_COCKPIT.cpp | `pilotDefinitionNames` | BTECH 304E:20AD | story combat.pilot.1.name |
| MW_MAIN_1090.cpp | `afterActionGeneralFailure` | MW_MAIN 1090:159B | story after_action.failure.general |
| MW_MAIN_1090.cpp | `afterActionGeneralSuccess` | MW_MAIN 1090:1742 | story after_action.success.general |
| MW_MAIN_1090.cpp | `afterActionKuritaFailure` | MW_MAIN 1090:149B | story after_action.failure.kurita |
| MW_MAIN_1090.cpp | `afterActionKuritaSuccess` | MW_MAIN 1090:1674 | story after_action.success.kurita |
| MW_MAIN_1090.cpp | `afterActionLiaoFailure` | MW_MAIN 1090:1528 | story after_action.failure.liao |
| MW_MAIN_1090.cpp | `afterActionLiaoSuccess` | MW_MAIN 1090:16D6 | story after_action.success.liao |
| MW_MAIN_1090.cpp | `alphabeticPlanetRecordOffsets` | - | story, MwStoryBuildPlanets |
| MW_MAIN_1090.cpp | `campaignDefaultCommanderNameBytes` | MW_MAIN 1090:5CE2 | story commander.default_name |
| MW_MAIN_1090.cpp | `contractActionsNavigationProbe` | MW_MAIN 1090:5807 | story probe.contract_actions |
| MW_MAIN_1090.cpp | `contractNarrative` | MW_MAIN 1090:4890 | story contract.narrative |
| MW_MAIN_1090.cpp | `contractOfficeNavigationProbe` | MW_MAIN 1090:5661 | story probe.contract_office |
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
| MW_MAIN_1090.cpp | `hubFirstNavigationProbe` | MW_MAIN 1090:1E75 | story probe.hub_planet |
| MW_MAIN_1090.cpp | `hubSecondNavigationProbe` | MW_MAIN 1090:1EC9 | story probe.hub_contract |
| MW_MAIN_1090.cpp | `hubThirdNavigationProbe` | MW_MAIN 1090:1EF3 | story probe.hub_remote_planet |
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
| MW_MAIN_1090_OPTIONS.cpp | `confirmationNavigationProbe` | MW_MAIN 1090:AB26 | story probe.confirmation |
| MW_MAIN_1090_OPTIONS.cpp | `newsNavigationProbe` | MW_MAIN 1090:332A | story probe.news |
| MW_MAIN_1090_OPTIONS.cpp | `optionsNavigationProbe` | MW_MAIN 1090:ABDD | story probe.options |
| MW_MAIN_1090_OPTIONS.cpp | `restoreNavigationProbe` | MW_MAIN 1090:AD6D | story probe.restore |
| MW_MAIN_1AA5.cpp | `eventTextStorage` | - | story, MwStoryBuildEventText |
| MW_MAIN_1AA5.cpp | `eventTextStorage / eventTextRecords` | - | story, MwStoryBuildEventText |
| MW_MAIN_28BE.cpp | `adlibAddressDelayReads` | MW_MAIN 28BE:00E0 | story, music.driver (native span) |
| MW_MAIN_28BE.cpp | `adlibDataDelayReads` | MW_MAIN 28BE:00E2 | story, music.driver (native span) |
| MW_MAIN_28BE.cpp | `adlibPackedNoteFrequencies` | MW_MAIN 28BE:00E6 | story, music.driver (native span) |
| MW_MAIN_28BE.cpp | `adlibRhythmRegister` | MW_MAIN 28BE:00E4 | story, music.driver (native span) |
| MW_MAIN_28BE.cpp | `adlibVoiceRoutes` | MW_MAIN 28BE:01CC | story, music.driver (native span) |
| MW_MAIN_28BE.cpp | `audioChannels` | MW_MAIN 28BE:04C6 | story, music.driver (native span) |
| MW_MAIN_28BE.cpp | `audioDummyChannel` | MW_MAIN 28BE:0646 | story, music.driver (native span) |
| MW_MAIN_28BE.cpp | `midiBarDivision` | MW_MAIN 28BE:004A | story, music.driver (native span) |
| MW_MAIN_28BE.cpp | `midiBeatDivision` | MW_MAIN 28BE:0048 | story, music.driver (native span) |
| MW_MAIN_28BE.cpp | `midiByteHandlerDisplacement` | MW_MAIN 28BE:00A8 | story, music.driver (native span) |
| MW_MAIN_28BE.cpp | `midiByteHandlers` | MW_MAIN 28BE:008C | story, music.driver (native span) |
| MW_MAIN_28BE.cpp | `midiStateUpdateInProgress` | MW_MAIN 28BE:005A | story, music.driver (native span) |
| MW_MAIN_28BE.cpp | `midiSysex` | MW_MAIN 28BE:07CE | story, music.driver (native span) |
| MW_MAIN_28BE.cpp | `midiTickDenominator` | MW_MAIN 28BE:0042 | story, music.driver (native span) |
| MW_MAIN_28BE.cpp | `midiTickInProgress` | MW_MAIN 28BE:005B | story, music.driver (native span) |
| MW_MAIN_28BE.cpp | `midiTickNumerator` | MW_MAIN 28BE:0044 | story, music.driver (native span) |
| MW_MAIN_28BE.cpp | `midiTrackChannelHandlers` | MW_MAIN 28BE:005D | story, music.driver (native span) |
| MW_MAIN_28BE.cpp | `midiTrackRunningStatus` | MW_MAIN 28BE:005C | story, music.driver (native span) |
| MW_MAIN_28BE.cpp | `midiTrackSystemHandlers` | MW_MAIN 28BE:006B | story, music.driver (native span) |
| MW_MAIN_28BE.cpp | `mpuTransmitQueue` | MW_MAIN 28BE:0224 | story, music.driver (native span) |

## Ours: UI words and system messages (story/words.json, imported)

| File | Item | Offset | From |
|---|---|---|---|
| BTECH_304E_COCKPIT.cpp | `actuatorLabels` | BTECH 304E:046C | words, cockpit.word.actuator.1 |
| BTECH_304E_COCKPIT.cpp | `automaticWeaponsLabel` | BTECH 304E:0059 | words, cockpit.word.automatic_weapons |
| BTECH_304E_COCKPIT.cpp | `bodyLocationLabels` | BTECH 304E:038E | words, cockpit.word.body_location.1 |
| BTECH_304E_COCKPIT.cpp | `briefingCrewLabel` | BTECH 304E:0DA9 | words, briefing.word.crew |
| BTECH_304E_COCKPIT.cpp | `briefingMechLabel` | BTECH 304E:0DB6 | words, briefing.word.mechs |
| BTECH_304E_COCKPIT.cpp | `briefingMissionLabel` | BTECH 304E:0D96 | words, briefing.word.mission |
| BTECH_304E_COCKPIT.cpp | `energyAmmoPlaceholder` | BTECH 304E:2410 | words, cockpit.word.range_class.1 |
| BTECH_304E_COCKPIT.cpp | `heatSinksOfText` | BTECH 304E:0566 | words, cockpit.word.heat_sinks_of |
| BTECH_304E_COCKPIT.cpp | `inspectionBlankWeaponRow` | BTECH 304E:0590 | words, cockpit.word.blank_weapon_row |
| BTECH_304E_COCKPIT.cpp | `inspectionConditionLabels` | BTECH 304E:03A6 | words, cockpit.word.condition.1 |
| BTECH_304E_COCKPIT.cpp | `inspectionSubsystemLabels` | BTECH 304E:0520 | words, cockpit.word.subsystem.1 |
| BTECH_304E_COCKPIT.cpp | `inspectionWeaponHeading` | BTECH 304E:057A | words, cockpit.word.weapon_heading |
| BTECH_304E_COCKPIT.cpp | `jumpJetsOfText` | BTECH 304E:0577 | words, cockpit.word.jump_jets_of |
| BTECH_304E_COCKPIT.cpp | `jumpJetsText` | BTECH 304E:0569 | words, cockpit.word.jump_jets |
| BTECH_304E_COCKPIT.cpp | `pausedText` | BTECH 304E:0081 | words, cockpit.word.paused |
| BTECH_304E_COCKPIT.cpp | `radarRangeLabels` | BTECH 304E:23EA | words, cockpit.word.radar_range.1 |
| BTECH_304E_COCKPIT.cpp | `radarRangeText` | BTECH 304E:23EA | words, cockpit.word.radar_range.1 |
| BTECH_304E_COCKPIT.cpp | `reactorShutdownText` | BTECH 304E:005D | words, cockpit.word.reactor_shutdown |
| BTECH_304E_COCKPIT.cpp | `retreatChoiceText` | BTECH 304E:083E | words, cockpit.word.retreat_choice |
| BTECH_304E_COCKPIT.cpp | `retreatQuestionText` | BTECH 304E:082F | words, cockpit.word.retreat_question |
| BTECH_304E_COCKPIT.cpp | `simulationText` | BTECH 304E:0076 | words, cockpit.word.simulation |
| BTECH_304E_COCKPIT.cpp | `subsystemLampLabels` | BTECH 304E:23BE | words, cockpit.word.lamp.1 |
| BTECH_304E_COCKPIT.cpp | `weaponLabelText` | BTECH 304E:2004 | words, cockpit.word.weapon.1 |
| BTECH_304E_COCKPIT.cpp | `weaponLabels` | BTECH 304E:2004 | words, cockpit.word.weapon.1 |
| BTECH_304E_COCKPIT.cpp | `weaponRangeAndAmmoText` | BTECH 304E:2410 | words, cockpit.word.range_class.1 |
| BTECH_304E_COCKPIT.cpp | `weaponRangeClassLabels` | BTECH 304E:2410 | words, cockpit.word.range_class.1 |
| BTECH_304E_COCKPIT.cpp | `zoomLabel` | BTECH 304E:0052 | words, cockpit.word.zoom |
| BTECH_304E_ORDERS.cpp | `orderBaseText` | BTECH 304E:010C | words, orders.word.order.1 |
| BTECH_304E_ORDERS.cpp | `orderColon` | BTECH 304E:010C | words, orders.word.order.1 |
| BTECH_304E_ORDERS.cpp | `orderDamageText` | BTECH 304E:010C | words, orders.word.order.1 |
| BTECH_304E_ORDERS.cpp | `orderLocationText` | BTECH 304E:010C | words, orders.word.order.1 |
| BTECH_304E_ORDERS.cpp | `orderOfText` | BTECH 304E:010C | words, orders.word.order.1 |
| BTECH_304E_ORDERS.cpp | `orderPanelText` | BTECH 304E:010C | words, orders.word.order.1 |
| BTECH_304E_ORDERS.cpp | `orderSelectionHeading` | BTECH 304E:010C | words, orders.word.order.1 |
| BTECH_304E_ORDERS.cpp | `orderWeaponHeading` | BTECH 304E:010C | words, orders.word.order.1 |
| BTECH_304E_ORDERS.cpp | `orderWeaponsText` | BTECH 304E:010C | words, orders.word.order.1 |
| MW_MAIN_1090.cpp | `acceptContractCaption` | MW_MAIN 1090:4B35 | words, contract_office.word.submit |
| MW_MAIN_1090.cpp | `afterActionCrewHeading` | MW_MAIN 1090:11DF | words, hub.word.after_action.crew_heading |
| MW_MAIN_1090.cpp | `afterActionMechHeading` | MW_MAIN 1090:11EC | words, hub.word.after_action.mech_heading |
| MW_MAIN_1090.cpp | `afterActionPaymentHeading` | MW_MAIN 1090:1202 | words, hub.word.after_action.payment_heading |
| MW_MAIN_1090.cpp | `afterActionPercentSuffix` | MW_MAIN 1090:120B | words, hub.word.after_action.percent |
| MW_MAIN_1090.cpp | `afterActionPilotStatusText` | MW_MAIN 1090:120D | words, hub.word.after_action.pilot_status |
| MW_MAIN_1090.cpp | `afterActionSalvageHeading` | MW_MAIN 1090:11F9 | words, hub.word.after_action.salvage_heading |
| MW_MAIN_1090.cpp | `ammoBayFullText` | MW_MAIN 1090:315B | words, complex.word.ammo_bay_full |
| MW_MAIN_1090.cpp | `ammoFamilyDisplayNames` | MW_MAIN 1090:2F91 | words, complex.word.ammo_family.1 |
| MW_MAIN_1090.cpp | `assignedMechUnreadyText` | MW_MAIN 1090:0FF7 | words, contract_office.word.mech_unready |
| MW_MAIN_1090.cpp | `barMenuLabels` | MW_MAIN 1090:9013 | words, crew.caption.bar_title.text |
| MW_MAIN_1090.cpp | `calendarMonthNames` | MW_MAIN 1090:4AAC | words, hub.word.month.1 |
| MW_MAIN_1090.cpp | `calendarMonthText` | MW_MAIN 1090:4AAC | words, hub.word.month.1 |
| MW_MAIN_1090.cpp | `campaignDamageTierLabels` | MW_MAIN 1090:2B2B | words, complex.word.damage_tier.1 |
| MW_MAIN_1090.cpp | `cannotAffordRepairText` | MW_MAIN 1090:2BB0 | words, complex.word.cannot_afford_repair |
| MW_MAIN_1090.cpp | `commanderNeedsMechText` | MW_MAIN 1090:0FC0 | words, contract_office.word.commander_needs_mech |
| MW_MAIN_1090.cpp | `commanderProfileLabels` | MW_MAIN 1090:2283 | words, crew.caption.profile_1.text |
| MW_MAIN_1090.cpp | `commanderProfileName` | MW_MAIN 1090:21C2 | words, crew.word.commander_name |
| MW_MAIN_1090.cpp | `commanderReputationLabels` | MW_MAIN 1090:23D8 | words, crew.word.reputation.1 |
| MW_MAIN_1090.cpp | `companyMenuLabels` | MW_MAIN 1090:21EB | words, crew.caption.company_title.text |
| MW_MAIN_1090.cpp | `contractAcceptedText` | MW_MAIN 1090:4C6C | words, contract_office.word.accepted |
| MW_MAIN_1090.cpp | `contractActionLabels` | MW_MAIN 1090:49F9 | words, contract_office.caption.negotiate.text |
| MW_MAIN_1090.cpp | `contractCancelledText` | MW_MAIN 1090:1031 | words, contract_office.word.cancelled |
| MW_MAIN_1090.cpp | `contractOfficeLabels` | MW_MAIN 1090:484F | words, contract_office.caption.request_offer.text |
| MW_MAIN_1090.cpp | `crewLabels` | MW_MAIN 1090:2348 | words, crew.caption.crew_1.text |
| MW_MAIN_1090.cpp | `dealerPurchaseLimitText` | MW_MAIN 1090:258A | words, dealer.word.purchase_limit |
| MW_MAIN_1090.cpp | `dropShipStatusLabels` | MW_MAIN 1090:2D21 | words, complex.caption.dropship_status.text |
| MW_MAIN_1090.cpp | `fullFourPilotRosterText` | MW_MAIN 1090:9C5A | words, crew.word.roster_full_four |
| MW_MAIN_1090.cpp | `fullThreePilotRosterText` | MW_MAIN 1090:9C8D | words, crew.word.roster_full_three |
| MW_MAIN_1090.cpp | `marketBodyCondition` | MW_MAIN 1090:468A | words, dealer.caption.component_body.text |
| MW_MAIN_1090.cpp | `marketComponentDone` | MW_MAIN 1090:47F5 | words, dealer.caption.component_done.text |
| MW_MAIN_1090.cpp | `marketComponentTitle` | MW_MAIN 1090:4536 | words, dealer.caption.component_title.text |
| MW_MAIN_1090.cpp | `marketDropShipStatusLabels` | MW_MAIN 1090:4459 | words, dealer.caption.dropship_title.text |
| MW_MAIN_1090.cpp | `marketListDoneText` | MW_MAIN 1090:3049 | words, dealer.word.market_list_done |
| MW_MAIN_1090.cpp | `marketListHeading` | MW_MAIN 1090:2922 | words, dealer.caption.market_heading.text |
| MW_MAIN_1090.cpp | `marketMechStatusLabels` | MW_MAIN 1090:439B | words, dealer.caption.mech_title.text |
| MW_MAIN_1090.cpp | `marketSystemConditionRows` | MW_MAIN 1090:4554 | words, dealer.caption.component_system_1.text |
| MW_MAIN_1090.cpp | `marketUnitListRows` | MW_MAIN 1090:2936 | words, dealer.caption.market_1.text |
| MW_MAIN_1090.cpp | `marketWeaponConditionRows` | MW_MAIN 1090:46BF | words, dealer.caption.component_weapon_1.text |
| MW_MAIN_1090.cpp | `marketWeaponHeading` | MW_MAIN 1090:46A2 | words, dealer.caption.component_weapon_heading.text |
| MW_MAIN_1090.cpp | `mechComplexLabels` | MW_MAIN 1090:25E4 | words, complex.caption.complex_title.text |
| MW_MAIN_1090.cpp | `mechNonfunctionalTextBytes` | MW_MAIN 1090:2B55 | words, complex.word.mech_nonfunctional |
| MW_MAIN_1090.cpp | `mechStatusLabels` | MW_MAIN 1090:2C60 | words, complex.caption.mech_status.text |
| MW_MAIN_1090.cpp | `negotiationTerminatedText` | MW_MAIN 1090:4C25 | words, contract_office.word.negotiation_ended |
| MW_MAIN_1090.cpp | `noAvailableRecruitsText` | MW_MAIN 1090:9C39 | words, crew.word.no_recruits |
| MW_MAIN_1090.cpp | `noContractsText` | MW_MAIN 1090:4BFA | words, contract_office.word.no_contracts |
| MW_MAIN_1090.cpp | `noMarketMechsText` | MW_MAIN 1090:31BB | words, dealer.word.no_market_mechs |
| MW_MAIN_1090.cpp | `noRepairWorkText` | MW_MAIN 1090:2BE7 | words, complex.word.no_repair_work |
| MW_MAIN_1090.cpp | `notificationTextStorage` | MW_MAIN 1090:5EDB | words, hub.word.notification.1 |
| MW_MAIN_1090.cpp | `ownedListDoneText` | MW_MAIN 1090:302F | words, complex.word.owned_list_done |
| MW_MAIN_1090.cpp | `ownedMechCapacityText` | MW_MAIN 1090:4347 | words, dealer.word.owned_capacity |
| MW_MAIN_1090.cpp | `ownedUnitListHeading` | MW_MAIN 1090:269B | words, complex.caption.owned_heading.text |
| MW_MAIN_1090.cpp | `ownedUnitListRows` | MW_MAIN 1090:26BA | words, complex.caption.owned_1.text |
| MW_MAIN_1090.cpp | `pilotSkillLabels` | MW_MAIN 1090:245A | words, crew.word.skill.1 |
| MW_MAIN_1090.cpp | `positionedText` | MW_MAIN 1090:55F1 | words, travel.caption.planet_8.text |
| MW_MAIN_1090.cpp | `purchaseInsufficientFundsText` | MW_MAIN 1090:4330 | words, dealer.word.insufficient_funds |
| MW_MAIN_1090.cpp | `rearmConfirmationLabels` | MW_MAIN 1090:312B | words, complex.caption.rearm_prompt.text |
| MW_MAIN_1090.cpp | `recruitInterviewLabels` | MW_MAIN 1090:9091 | words, crew.caption.recruit_prompt.text |
| MW_MAIN_1090.cpp | `recruitStatLabels` | MW_MAIN 1090:9106 | words, crew.word.recruit_stats |
| MW_MAIN_1090.cpp | `repairBodyCondition` | MW_MAIN 1090:33AD | words, complex.caption.repair_body.text |
| MW_MAIN_1090.cpp | `repairCompleteText` | MW_MAIN 1090:2C01 | words, complex.word.repair_complete |
| MW_MAIN_1090.cpp | `repairFailedText` | MW_MAIN 1090:3CB5 | words, complex.word.repair_failed |
| MW_MAIN_1090.cpp | `repairFooter` | MW_MAIN 1090:33C5 | words, complex.caption.repair_footer.text |
| MW_MAIN_1090.cpp | `repairIncompleteText` | MW_MAIN 1090:2C21 | words, complex.word.repair_incomplete |
| MW_MAIN_1090.cpp | `repairJumpJetCountTemplateBytes` | MW_MAIN 1090:36A5 | words, complex.word.jump_jet_count |
| MW_MAIN_1090.cpp | `repairNoJumpJetsTextBytes` | MW_MAIN 1090:36A0 | words, complex.word.no_jump_jets |
| MW_MAIN_1090.cpp | `repairSystemConditionRows` | MW_MAIN 1090:3277 | words, complex.caption.repair_system_1.text |
| MW_MAIN_1090.cpp | `repairTitle` | MW_MAIN 1090:3259 | words, complex.caption.repair_title.text |
| MW_MAIN_1090.cpp | `repairWeaponConditionRows` | MW_MAIN 1090:3408 | words, complex.caption.repair_weapon_1.text |
| MW_MAIN_1090.cpp | `repairWeaponLabelStorage` | MW_MAIN 1090:36C2 | words, complex.word.repair_weapons.1 |
| MW_MAIN_1090.cpp | `reserveAmmoBuyLabel` | MW_MAIN 1090:413A | words, dealer.word.ammo_buy |
| MW_MAIN_1090.cpp | `reserveAmmoCompatibilityHeadings` | MW_MAIN 1090:3FDA | words, dealer.caption.compatibility_1.text |
| MW_MAIN_1090.cpp | `reserveAmmoDoneLabel` | MW_MAIN 1090:4263 | words, dealer.caption.ammo_done.text |
| MW_MAIN_1090.cpp | `reserveAmmoFamilyLabels` | MW_MAIN 1090:41C9 | words, dealer.caption.ammo_family_1.text |
| MW_MAIN_1090.cpp | `reserveAmmoQuantityRows` | MW_MAIN 1090:4221 | words, dealer.caption.ammo_quantity_1.text |
| MW_MAIN_1090.cpp | `reserveAmmoSellLabel` | MW_MAIN 1090:413F | words, dealer.word.ammo_sell |
| MW_MAIN_1090.cpp | `reserveAmmoShopHeading` | MW_MAIN 1090:4175 | words, dealer.caption.ammo_heading.text |
| MW_MAIN_1090.cpp | `saleConfirmationLabels` | MW_MAIN 1090:3090 | words, complex.caption.sale_prompt.text |
| MW_MAIN_1090.cpp | `submitContractCaption` | MW_MAIN 1090:4B35 | words, contract_office.word.submit |
| MW_MAIN_1090.cpp | `travelBrowseLabel` | MW_MAIN 1090:5C8C | words, travel.caption.browse_houses.text |
| MW_MAIN_1090.cpp | `travelConfirmLabel` | MW_MAIN 1090:5C2E | words, travel.caption.leave.text |
| MW_MAIN_1090.cpp | `travelCostLabel` | MW_MAIN 1090:5C10 | words, travel.caption.confirm_travel.text |
| MW_MAIN_1090.cpp | `travelDestinationHeaderLabels` | MW_MAIN 1090:5C4E | words, travel.caption.destination_name.text |
| MW_MAIN_1090.cpp | `travelDestinationName` | MW_MAIN 1090:5C4E | words, travel.caption.destination_name.text |
| MW_MAIN_1090.cpp | `travelHouseBrowserLabels` | MW_MAIN 1090:5A30 | words, travel.caption.browser_title.text |
| MW_MAIN_1090.cpp | `travelHousePlanetCancelLabel` | MW_MAIN 1090:5802 | words, travel.caption.planets_back.text |
| MW_MAIN_1090.cpp | `travelHousePlanetRows` | MW_MAIN 1090:5550 | words, travel.caption.planet_1.text |
| MW_MAIN_1090.cpp | `travelHouseSearchCaption` | MW_MAIN 1090:5539 | words, travel.caption.search.text |
| MW_MAIN_1090.cpp | `travelHouseSearchNames` | MW_MAIN 1090:59C5 | words, travel.word.house_search.1 |
| MW_MAIN_1090.cpp | `travelInsufficientFundsText` | MW_MAIN 1090:5B5A | words, travel.word.insufficient_funds |
| MW_MAIN_1090.cpp | `travelLandscapeLabels` | MW_MAIN 1090:5CA6 | words, travel.word.landscape.1 |
| MW_MAIN_1090.cpp | `travelPopulationSuffix` | MW_MAIN 1090:5B83 | words, travel.word.population_suffix |
| MW_MAIN_1090.cpp | `unassignedMechWarriorText` | MW_MAIN 1090:3024 | words, complex.word.unassigned |
| MW_MAIN_1090_OPTIONS.cpp | `audioOptionLabels` | MW_MAIN 1090:A071 | words, options.word.audio_labels |
| MW_MAIN_1090_OPTIONS.cpp | `confirmationLabels` | MW_MAIN 1090:9CD9 | words, options.caption.confirm_quit.text |
| MW_MAIN_1090_OPTIONS.cpp | `loadDiskErrorText` | MW_MAIN 1090:A0E3 | words, options.word.load_disk_error |
| MW_MAIN_1090_OPTIONS.cpp | `newsBoundaryText` | MW_MAIN 1090:2564 | words, options.word.news_boundary |
| MW_MAIN_1090_OPTIONS.cpp | `newsMenuLabels` | MW_MAIN 1090:251B | words, options.caption.news_previous.text |
| MW_MAIN_1090_OPTIONS.cpp | `optionsLabels` | MW_MAIN 1090:9D75 | words, options.caption.save.text |
| MW_MAIN_1090_OPTIONS.cpp | `restoreCancel` | MW_MAIN 1090:9F71 | words, options.caption.restore_cancel.text |
| MW_MAIN_1090_OPTIONS.cpp | `restoreHeading` | MW_MAIN 1090:9E7E | words, options.caption.restore_heading.text |
| MW_MAIN_1090_OPTIONS.cpp | `saveDiskErrorText` | MW_MAIN 1090:A0B3 | words, options.word.save_disk_error |
| MW_MAIN_1090_OPTIONS.cpp | `saveDiskFullText` | MW_MAIN 1090:A088 | words, options.word.disk_full |
| MW_MAIN_1090_OPTIONS.cpp | `saveListRows` | MW_MAIN 1090:9E99 | words, options.caption.save_1.text |
| MW_MAIN_1090_OPTIONS.cpp | `saveNamePrompt` | MW_MAIN 1090:A031 | words, options.word.save_prompt |
| MW_MAIN_1090_OPTIONS.cpp | `terrainDetailLabels` | MW_MAIN 1090:A079 | words, options.word.terrain_labels |

## Ours: screen layout, panels and menus (data/ui/*.json)

| File | Item | Offset | From |
|---|---|---|---|
| BTECH_304E_COCKPIT.cpp | `actuatorConditionLabelIndices` | BTECH 304E:04E6 | ui cockpit.condition_labels.actuator |
| BTECH_304E_COCKPIT.cpp | `cockpitLayouts` | BTECH 304E:1D48 | ui cockpit.layouts |
| BTECH_304E_COCKPIT.cpp | `engineConditionLabelIndices` | BTECH 304E:04DE | ui cockpit.condition_labels.engine |
| BTECH_304E_COCKPIT.cpp | `hudColourSchemes` | BTECH 304E:1F3C | ui cockpit.hud_colour_schemes |
| BTECH_304E_COCKPIT.cpp | `inspectionDiagramOrigins` | BTECH 304E:1878 | ui cockpit.inspection_diagram_origins |
| BTECH_304E_COCKPIT.cpp | `inspectionDiagramRegions` | BTECH 304E:1898 | ui cockpit.inspection_diagram_regions |
| BTECH_304E_COCKPIT.cpp | `mechCockpitLayoutIndices` | BTECH 304E:1D38 | ui cockpit.layout_by_template |
| BTECH_304E_COCKPIT.cpp | `subsystemLampColourTables` | BTECH 304E:23CE | ui cockpit.subsystem_lamp_colours |
| BTECH_304E_COCKPIT.cpp | `targetDiagramAnchors` | BTECH 304E:1AD8 | ui cockpit.target_diagram_anchors |
| BTECH_304E_COCKPIT.cpp | `targetDiagramRegions` | BTECH 304E:1AF8 | ui cockpit.target_diagram_regions |
| BTECH_304E_COCKPIT.cpp | `twoHitSystemConditionLabelIndices` | BTECH 304E:04D8 | ui cockpit.condition_labels.two_hit |
| BTECH_304E_ORDERS.cpp | `lanceOrderPanelTextX` | BTECH 304E:008C | ui orders.start.lance_panel_text_x |
| BTECH_304E_ORDERS.cpp | `orderDisplayIndices` | BTECH 304E:02C4 | ui orders.display_indices |
| BTECH_304E_ORDERS.cpp | `orderEngineConditionColours` | BTECH 304E:030E | ui orders.engine_condition_colours |
| BTECH_304E_ORDERS.cpp | `orderNavigationPanelDeltas` | BTECH 304E:009A | ui orders.navigation_panel_deltas |
| BTECH_304E_ORDERS.cpp | `orderNavigationRowDeltas` | BTECH 304E:00B0 | ui orders.navigation_row_deltas |
| BTECH_304E_ORDERS.cpp | `orderSelectionFirstRows` | BTECH 304E:0092 | ui orders.selection_first_rows |
| BTECH_304E_ORDERS.cpp | `orderSelectionLastRows` | BTECH 304E:0096 | ui orders.selection_last_rows |
| BTECH_304E_ORDERS.cpp | `orderSystemConditionColours` | BTECH 304E:031E | ui orders.system_condition_colours |
| BTECH_304E_VIDEO.cpp | `ellipseLeftInsets` | 0x401C | - |
| BTECH_304E_VIDEO.cpp | `landscapeCameraColourRows` | BTECH 304E:1E88 | ui cockpit.landscape_camera_rows |
| MW_MAIN_1090.cpp | `barMenu` | MW_MAIN 1090:904C | ui crew.menu.bar |
| MW_MAIN_1090.cpp | `barMenuPanel` | MW_MAIN 1090:8FF6 | ui crew.panel.bar |
| MW_MAIN_1090.cpp | `barRecords` | MW_MAIN 1090:904C | ui crew.menu.bar |
| MW_MAIN_1090.cpp | `bodyDamageTierFillColours` | MW_MAIN 1090:3CD2 | ui complex.table.body_damage_fill_colours |
| MW_MAIN_1090.cpp | `calendarMonthNameWidths` | MW_MAIN 1090:11BA | ui hub.table.calendar_month_name_widths |
| MW_MAIN_1090.cpp | `commanderProfilePanel` | MW_MAIN 1090:2258 | ui crew.panel.commander_profile |
| MW_MAIN_1090.cpp | `companyMenu` | MW_MAIN 1090:222E | ui crew.menu.company |
| MW_MAIN_1090.cpp | `companyMenuPanel` | MW_MAIN 1090:21CE | ui crew.panel.company |
| MW_MAIN_1090.cpp | `companyRecords` | MW_MAIN 1090:222E | ui crew.menu.company |
| MW_MAIN_1090.cpp | `componentRepairMenu` | MW_MAIN 1090:3558 | ui complex.menu.repair |
| MW_MAIN_1090.cpp | `componentRepairPanel` | MW_MAIN 1090:3214 | ui complex.panel.repair |
| MW_MAIN_1090.cpp | `componentRepairUiRecords` | MW_MAIN 1090:3558 | ui complex.menu.repair |
| MW_MAIN_1090.cpp | `contractActionRecords` | MW_MAIN 1090:4A15 | ui contract_office.menu.actions |
| MW_MAIN_1090.cpp | `contractActionsMenu` | MW_MAIN 1090:4A15 | ui contract_office.menu.actions |
| MW_MAIN_1090.cpp | `contractActionsPanel` | MW_MAIN 1090:49DE | ui contract_office.panel.actions |
| MW_MAIN_1090.cpp | `contractDateYearXOffsets` | MW_MAIN 1090:4B1A | ui contract_office.table.date_year_x_offsets |
| MW_MAIN_1090.cpp | `contractHubMenu` | MW_MAIN 1090:10D7 | ui hub.menu.contract |
| MW_MAIN_1090.cpp | `contractHubRecords` | MW_MAIN 1090:10D7 | ui hub.menu.contract |
| MW_MAIN_1090.cpp | `contractOfficeMenu` | MW_MAIN 1090:486F | ui contract_office.menu.office |
| MW_MAIN_1090.cpp | `contractOfficePanel` | MW_MAIN 1090:4822 | ui contract_office.panel.office |
| MW_MAIN_1090.cpp | `contractOfficeRecords` | MW_MAIN 1090:486F | ui contract_office.menu.office |
| MW_MAIN_1090.cpp | `crewLabelsPanel` | MW_MAIN 1090:2325 | ui crew.panel.crew |
| MW_MAIN_1090.cpp | `crewMenu` | MW_MAIN 1090:238A | ui crew.menu.crew |
| MW_MAIN_1090.cpp | `crewRecords` | MW_MAIN 1090:238A | ui crew.menu.crew |
| MW_MAIN_1090.cpp | `dropShipServiceMenu` | MW_MAIN 1090:2D99 | ui complex.menu.dropship_service |
| MW_MAIN_1090.cpp | `dropShipServiceUiRecords` | MW_MAIN 1090:2D99 | ui complex.menu.dropship_service |
| MW_MAIN_1090.cpp | `dropShipStatusPanel` | MW_MAIN 1090:2D06 | ui complex.panel.dropship_status |
| MW_MAIN_1090.cpp | `dropShipTravelFrames` | MW_MAIN 1090:5B2E | ui travel.table.dropship_frames |
| MW_MAIN_1090.cpp | `eventThreeChoiceMenu` | MW_MAIN 1090:5E38 | ui events.menu.three_choices |
| MW_MAIN_1090.cpp | `eventTwoChoiceMenu` | MW_MAIN 1090:5E1C | ui events.menu.two_choices |
| MW_MAIN_1090.cpp | `hiredTransportFrames` | MW_MAIN 1090:5AEA | ui travel.table.hired_transport_frames |
| MW_MAIN_1090.cpp | `marketComponentMenu` | MW_MAIN 1090:480F | ui dealer.menu.component |
| MW_MAIN_1090.cpp | `marketComponentPanel` | MW_MAIN 1090:44F1 | ui dealer.panel.component |
| MW_MAIN_1090.cpp | `marketComponentRecords` | MW_MAIN 1090:480F | ui dealer.menu.component |
| MW_MAIN_1090.cpp | `marketDropShipMenu` | MW_MAIN 1090:44D3 | ui dealer.menu.dropship |
| MW_MAIN_1090.cpp | `marketDropShipRecords` | MW_MAIN 1090:44D3 | ui dealer.menu.dropship |
| MW_MAIN_1090.cpp | `marketDropShipStatusPanel` | MW_MAIN 1090:443E | ui dealer.panel.dropship |
| MW_MAIN_1090.cpp | `marketMechMenu` | MW_MAIN 1090:44A9 | ui dealer.menu.mech |
| MW_MAIN_1090.cpp | `marketMechRecords` | MW_MAIN 1090:44A9 | ui dealer.menu.mech |
| MW_MAIN_1090.cpp | `marketMechStatusPanel` | MW_MAIN 1090:437E | ui dealer.panel.mech |
| MW_MAIN_1090.cpp | `marketUnitListMenu` | MW_MAIN 1090:2A1B | ui dealer.menu.market |
| MW_MAIN_1090.cpp | `marketUnitListPanel` | MW_MAIN 1090:28F1 | ui dealer.panel.market |
| MW_MAIN_1090.cpp | `marketUnitListRecords` | MW_MAIN 1090:2A1B | ui dealer.menu.market |
| MW_MAIN_1090.cpp | `mechBodyDiagramLayouts` | MW_MAIN 1090:3CEE | ui complex.table.body_diagrams |
| MW_MAIN_1090.cpp | `mechComplexAnimationFrames` | MW_MAIN 1090:318D | ui complex.table.animation_frames |
| MW_MAIN_1090.cpp | `mechComplexMenu` | MW_MAIN 1090:2632 | ui complex.menu.complex |
| MW_MAIN_1090.cpp | `mechComplexPanel` | MW_MAIN 1090:25C5 | ui complex.panel.complex |
| MW_MAIN_1090.cpp | `mechComplexRecords` | MW_MAIN 1090:2632 | ui complex.menu.complex |
| MW_MAIN_1090.cpp | `mechServiceMenu` | MW_MAIN 1090:2D53 | ui complex.menu.mech_service |
| MW_MAIN_1090.cpp | `mechServiceUiRecords` | MW_MAIN 1090:2D53 | ui complex.menu.mech_service |
| MW_MAIN_1090.cpp | `mechStatusPanel` | MW_MAIN 1090:2C3F | ui complex.panel.mech_status |
| MW_MAIN_1090.cpp | `notificationColumns` | MW_MAIN 1090:5EB3 | ui hub.table.notification_boxes |
| MW_MAIN_1090.cpp | `notificationRows` | MW_MAIN 1090:5EB3 | ui hub.table.notification_boxes |
| MW_MAIN_1090.cpp | `notificationTextInsetX` | MW_MAIN 1090:5EB3 | ui hub.table.notification_boxes |
| MW_MAIN_1090.cpp | `notificationTextInsetY` | MW_MAIN 1090:5EB3 | ui hub.table.notification_boxes |
| MW_MAIN_1090.cpp | `notificationTextOffsets` | MW_MAIN 1090:5ED3 | ui hub.table.notification_words |
| MW_MAIN_1090.cpp | `ownedUnitListMenu` | MW_MAIN 1090:283B | ui complex.menu.owned |
| MW_MAIN_1090.cpp | `ownedUnitListPanel` | MW_MAIN 1090:266A | ui complex.panel.owned |
| MW_MAIN_1090.cpp | `ownedUnitListUiRecords` | MW_MAIN 1090:283B | ui complex.menu.owned |
| MW_MAIN_1090.cpp | `pilotAssignmentCaptionColours` | MW_MAIN 1090:247E | ui crew.table.pilot_assignment_captions |
| MW_MAIN_1090.cpp | `pilotCardColumnOrigins` | MW_MAIN 1090:244A | ui crew.table.pilot_card_column_origins |
| MW_MAIN_1090.cpp | `planetHubMenu` | MW_MAIN 1090:1083 | ui hub.menu.planet |
| MW_MAIN_1090.cpp | `planetHubRecords` | MW_MAIN 1090:1083 | ui hub.menu.planet |
| MW_MAIN_1090.cpp | `rearmConfirmationMenu` | MW_MAIN 1090:316D | ui complex.menu.rearm |
| MW_MAIN_1090.cpp | `rearmConfirmationPanel` | MW_MAIN 1090:3110 | ui complex.panel.rearm |
| MW_MAIN_1090.cpp | `rearmConfirmationRecords` | MW_MAIN 1090:316D | ui complex.menu.rearm |
| MW_MAIN_1090.cpp | `recruitInterviewMenu` | MW_MAIN 1090:90EA | ui crew.menu.recruit |
| MW_MAIN_1090.cpp | `recruitInterviewPanel` | MW_MAIN 1090:9076 | ui crew.panel.recruit |
| MW_MAIN_1090.cpp | `recruitInterviewRecords` | MW_MAIN 1090:90EA | ui crew.menu.recruit |
| MW_MAIN_1090.cpp | `remotePlanetHubMenu` | MW_MAIN 1090:1101 | ui hub.menu.remote_planet |
| MW_MAIN_1090.cpp | `remotePlanetHubRecords` | MW_MAIN 1090:1101 | ui hub.menu.remote_planet |
| MW_MAIN_1090.cpp | `repairTitleLeadingSpacesByKind` | MW_MAIN 1090:31F2 | ui complex.table.repair_title_leading_spaces |
| MW_MAIN_1090.cpp | `requestedTermDigits` | MW_MAIN 1090:49CC | ui contract_office.table.requested_terms |
| MW_MAIN_1090.cpp | `requestedTermX` | MW_MAIN 1090:49CC | ui contract_office.table.requested_terms |
| MW_MAIN_1090.cpp | `requestedTermY` | MW_MAIN 1090:49CC | ui contract_office.table.requested_terms |
| MW_MAIN_1090.cpp | `reserveAmmoCompatibilityPanels` | MW_MAIN 1090:3F7B | ui dealer.table.compatibility_panels |
| MW_MAIN_1090.cpp | `reserveAmmoShopMenu` | MW_MAIN 1090:4268 | ui dealer.menu.ammo |
| MW_MAIN_1090.cpp | `reserveAmmoShopPanel` | MW_MAIN 1090:4144 | ui dealer.panel.ammo |
| MW_MAIN_1090.cpp | `reserveAmmoShopRecords` | MW_MAIN 1090:4268 | ui dealer.menu.ammo |
| MW_MAIN_1090.cpp | `saleConfirmationMenu` | MW_MAIN 1090:30F0 | ui complex.menu.sale |
| MW_MAIN_1090.cpp | `saleConfirmationPanel` | MW_MAIN 1090:3075 | ui complex.panel.sale |
| MW_MAIN_1090.cpp | `saleConfirmationRecords` | MW_MAIN 1090:30F0 | ui complex.menu.sale |
| MW_MAIN_1090.cpp | `travelBrowsePanel` | MW_MAIN 1090:5C75 | ui travel.panel.browse |
| MW_MAIN_1090.cpp | `travelConfirmPanel` | MW_MAIN 1090:5C17 | ui travel.panel.confirm |
| MW_MAIN_1090.cpp | `travelCostPanel` | MW_MAIN 1090:5BF9 | ui travel.panel.cost |
| MW_MAIN_1090.cpp | `travelDestinationHeaderPanel` | MW_MAIN 1090:5C35 | ui travel.panel.destination |
| MW_MAIN_1090.cpp | `travelHouseBrowserPanel` | MW_MAIN 1090:5A0D | ui travel.panel.house_browser |
| MW_MAIN_1090.cpp | `travelHouseBrowserRecords` | MW_MAIN 1090:5A7B | ui travel.menu.house_browser |
| MW_MAIN_1090.cpp | `travelHousePlanetPanel` | MW_MAIN 1090:54E4 | ui travel.panel.house_planets |
| MW_MAIN_1090.cpp | `travelHousePlanetRecords` | MW_MAIN 1090:5809 | ui travel.menu.house_planets |
| MW_MAIN_1090.cpp | `travelMapRecords` | MW_MAIN 1090:5B8C | ui travel.menu.map |
| MW_MAIN_1090.cpp | `unitDetailPictureXOffsets` | MW_MAIN 1090:2B83 | ui complex.table.detail_picture_x_offsets |
| MW_MAIN_1090.cpp | `unitDetailPictureYOrigins` | MW_MAIN 1090:2B99 | ui complex.table.detail_picture_y_origins |
| MW_MAIN_1090_OPTIONS.cpp | `confirmationMenu` | MW_MAIN 1090:9D34 | ui options.menu.confirmation |
| MW_MAIN_1090_OPTIONS.cpp | `confirmationPanel` | MW_MAIN 1090:9CBE | ui options.panel.confirmation |
| MW_MAIN_1090_OPTIONS.cpp | `confirmationRecords` | MW_MAIN 1090:9D34 | ui options.menu.confirmation |
| MW_MAIN_1090_OPTIONS.cpp | `newsMenu` | MW_MAIN 1090:2538 | ui options.menu.news |
| MW_MAIN_1090_OPTIONS.cpp | `newsMenuPanel` | MW_MAIN 1090:2500 | ui options.panel.news |
| MW_MAIN_1090_OPTIONS.cpp | `newsRecords` | MW_MAIN 1090:2538 | ui options.menu.news |
| MW_MAIN_1090_OPTIONS.cpp | `optionsMenu` | MW_MAIN 1090:9DEB | ui options.menu.options |
| MW_MAIN_1090_OPTIONS.cpp | `optionsPanel` | MW_MAIN 1090:9D52 | ui options.panel.options |
| MW_MAIN_1090_OPTIONS.cpp | `optionsRecords` | MW_MAIN 1090:9DEB | ui options.menu.options |
| MW_MAIN_1090_OPTIONS.cpp | `restoreMenu` | MW_MAIN 1090:9F7B | ui options.menu.restore |
| MW_MAIN_1090_OPTIONS.cpp | `restorePanel` | MW_MAIN 1090:9E4D | ui options.panel.restore |
| MW_MAIN_1090_OPTIONS.cpp | `restoreRecords` | MW_MAIN 1090:9F7B | ui options.menu.restore |

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
| BTECH_304E_POSE.cpp | `animationLogicalFrameCounts` | 0x24A0 | - |
| BTECH_304E_POSE.cpp | `animationPartFrameCounts` | 0x24AA | - |
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
| MW_MAIN_1090.cpp | `contractOfficePlanetGroupStorage` | MW_MAIN 1090:4B8A | rule houses.contract_planet_groups |
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
| MW_MAIN_1090.cpp | `randomStateWord0` | MW_MAIN 1090:03F5 | rule new_campaign.start.random_seed |
| MW_MAIN_1090.cpp | `randomStateWord1` | MW_MAIN 1090:03F5 | rule new_campaign.start.random_seed |
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
| BTECH_304E_VIDEO.cpp | `herculesDisplayRowOffsets` | - | - |
| MW_0806_STATE.cpp | `launcherCampaignFilename` | - | - |
| MW_0806_STATE.cpp | `launcherCombatFilename` | - | - |
| MW_0806_STATE.cpp | `launcherIdentity` | - | - |
| MW_MAIN_1090.cpp | `combatExecutableFilename` | - | - |
| MW_MAIN_1090.cpp | `launcherExitCode` | - | - |

## Code: runtime state and renderer tables (no file)

Not listed one by one: BTECH_0810_STATE.cpp 1, BTECH_1ADF_STATE.cpp 2, BTECH_1BF5_STATE.cpp 5, BTECH_22B6_DATA.cpp 15, BTECH_2FEB.cpp 24, BTECH_304E.cpp 35, BTECH_304E_ADLIB.cpp 25, BTECH_304E_COCKPIT.cpp 36, BTECH_304E_MPU.cpp 10, BTECH_304E_ORDERS.cpp 26, BTECH_304E_POLYGONS.cpp 24, BTECH_304E_POSE.cpp 14, BTECH_304E_RENDER.cpp 62, BTECH_304E_RUNTIME.cpp 17, BTECH_304E_SCENE.cpp 8, BTECH_304E_SOUND.cpp 43, BTECH_304E_STARTUP.cpp 59, BTECH_304E_VIDEO.cpp 85, BTECH_304E_WORLD.cpp 111, MW_0806_STATE.cpp 1, MW_EGA_0800_STATE.cpp 66, MW_MAIN_0F41_STATE.cpp 5, MW_MAIN_1090.cpp 250, MW_MAIN_1090_OPTIONS.cpp 11, MW_MAIN_1AA5.cpp 1, MW_MAIN_28BE.cpp 19, NativeStart.cpp 5.

Still read from the exe (slice 6 moves them): BTECH_304E.cpp 4, BTECH_304E_ADLIB.cpp 8, BTECH_304E_COCKPIT.cpp 4, BTECH_304E_MPU.cpp 6, BTECH_304E_ORDERS.cpp 3, BTECH_304E_POLYGONS.cpp 2, BTECH_304E_POSE.cpp 2, BTECH_304E_RENDER.cpp 9, BTECH_304E_RUNTIME.cpp 5, BTECH_304E_SOUND.cpp 19, BTECH_304E_STARTUP.cpp 23, BTECH_304E_VIDEO.cpp 27, BTECH_304E_WORLD.cpp 14.

## Reclassified or removed in slice 3

- `houseMissionPlanetTableOffsets` (MW_MAIN 1090:8E90) was listed as story content; it is the pointer table of the Houses' mission-planet policies, a rule (`houses.mission_planet_policy_offsets`).
- `repairAttemptFailuresByKind` (MW_MAIN 1090:3A77, 220 bytes) was listed as a rule; it is zero in the exe and written in play, so it is code, zeroed where the exe reloaded it.
- `unusedRepairTimeByDamageTier` (MW_MAIN 1090:3A6F, 8 bytes) was listed as a rule; it was imported and never read, and is gone.
- `combatMissionSideZeroRules` (BTECH 304E:092C, 74 bytes; an alias into `missionRules`) and `mechFallbackTemplateSteps` (BTECH 304E:0992, 16 bytes) were missing; both are rules.
- `mountedWeaponCountsByKind`, `operationalWeaponCountsByKind` (both BTECH 304E:4F32), `selectionObjectListIndices` (6F5A) and `scenarioSelectors` (7D6A) lie past BTECH's file data (304E:4EE0), so the exe holds zeros and play writes them: code.
- `musicSequences` (BTECH 304E:7D8A) was listed as story content; it lies past the file data too, and MUS:INF fills it at run time: code.
- `actuatorConditionLabelIndices`, `engineConditionLabelIndices`, `twoHitSystemConditionLabelIndices` and `orderDisplayIndices` pick labels and move with them in slice 5: UI words.
- Eight start values are always written before they're read, and are code, zeroed where the exe reloaded them (all eight are zero in the exe): `settlementPayoutPercent` (1090:123F), `pilotCardGunneryTier`, `pilotCardPilotingTier`, `pilotCardQuotedWageTier` (2440, 2442, 2446), `missionForceBudget` (4B79), `missionPaymentBaseThousands` (4B7B), `travelDestinationTimeWeight` (54DA, written and never read) and `sideObjectiveRules` (BTECH 304E:0ED0). The other four, `ammunitionTransactionPrices`, `generateStartingJenner`, `generatedAmmoKinds` and `generatedAmmoQuantities`, are read first on some path and stay `"start"` entries in market.json.
- Two of the save codec's 105 fields, `pilotRosterCount` and `rosterPilotIds`, are owned in `src/Original/Campaign`, so this list (of `src/Original/Memory`) doesn't show them; new_campaign.json holds them.

## Reclassified in slice 4

- Every item the layout door fills (`src/Ui/ui_pieces.txt`) is layout, with the piece it comes from. A caption whose head is a layout piece stays with the UI words, since its text is still read from the exe until slice 5 moves it; its row names the head's piece.
- Nine start values are always written before they're read, and are code, zeroed where the exe reloaded them (all nine are zero in the exe): `calendarTextX`, `calendarTextY`, `calendarBackground`, `calendarForeground` (1090:117E..1183, set as the calendar is drawn), `pilotCardTextX` (243C) and `crewScreenRightEdge` (2438, set as the roster card is drawn), `contractRequestTextColour` (4B34, set by the term editor before its only reader), `afterActionRosterRowOffset` (1229, set at each after-action) and `reserveAmmoMechColours` (3F70, set on entering the shop). `lanceOrderPanelTextX` (BTECH 304E:008C) is read first after an EXEC, and is orders.json's `"start"`.
- The font metrics (`fontAdvanceWidths`, `combatFontHeights`, `fontFirstCharacters`, `fontCharacterCounts`, BTECH 304E:2E84..2ED3) are written when a font loads into its slot, and are zero in the exe: code, zeroed where the exe reloaded them.
- `orderMapPositionMask` (BTECH 304E:4EF0) lies past BTECH's file data and is written before it's read: code, zeroed where the exe reloaded it.
- `positionedText` (MW_MAIN 1090:55EC) is the dealer component menu's past-end probe: code, like the other probes.

## Reclassified in slice 5

- Every item the words fill (`src/Import/word_pieces.txt`, through `MwWordsCopyText`, `MwWordsCopyBlock` or an importer's view of the words) is a UI word, with the word it comes from; a caption's row names its text's word (`<caption>.text`). None is read from the exe.
- `positionedText` (1090:55EC) and `travelDestinationName` (5C4E) are now filled from the eighth planet row's and the destination header's caption texts: UI words.
- The pointers into word blocks (`calendarMonthNames`, `weaponLabels`, `weaponRangeClassLabels`, `radarRangeLabels`, `energyAmmoPlaceholder`) show the block they point into.
- The four index tables (`actuatorConditionLabelIndices`, `engineConditionLabelIndices`, `twoHitSystemConditionLabelIndices`, `orderDisplayIndices`) and `notificationTextOffsets` are layout pieces: layout.
- `newsAtStart` and `newsAtEnd` (1090:257F, 2580) are the news screen's flags, zero in the exe and reset as the screen opens: code (their names had sorted them with the words).
