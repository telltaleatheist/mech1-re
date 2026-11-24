// The game's files the program opens, as the two copies the project knows hold
// them (the 1989 release and the one of 1990-01-22; they differ in none of these
// but the known damaged one, and in BTECH.EXE and MW_MAIN.EXE, which have their
// own checks). Written by tools/setup/make_game_files.py, which says where each
// is opened; don't edit. Names, sizes and SHA-256 only.
#include "FileCheck.h"

namespace mw1o::setup {
namespace {

using When = GameFile::When;
constexpr GameFile kFiles[] = {
    // import_story (src/Import/Story.cpp): import
    {"MW_EGA.EXE", 5599, "fbbeef961ab0308127c390a3e2b345e1d82b6fcebe5c487c5f949532d0abba16", When::kImport, "the first start can't import the story"},
    // EgaLoadFonts (MW_EGA_0800.cpp): start
    {"6X6.FNT", 580, "c0df8daeec521175190e72eb1c868fcb9d2deee3359b480311b83dee3f5115cc", When::kStart, "the campaign can't start"},
    {"FOX88.FNT", 772, "0a1d188c6f4eba381aa098a3474a888164392d9492ece3cba06cd05159a505e9", When::kStart, "the campaign can't start"},
    // MwLoadGeneralPictureBank (MW_MAIN_081B_RESOURCES.cpp): start
    {"MW_GPICS.BIN", 18846, "db31ff5880315b78bdeaaa6f2af69195856c79873a23b96bd69566b13749cd6a", When::kStart, "the campaign can't start"},
    // MwLoadCockpitPictureBank (MW_MAIN_081B_RESOURCES.cpp): start
    {"MW_CPICS.BIN", 30097, "bd8b1421760d410f3104ae8de7febf6f0616f1fe56028f1057d0da9c9f9d5800", When::kStart, "the campaign can't start"},
    // MwLoadSecondaryPictureBank (MW_MAIN_081B_RESOURCES.cpp): start
    {"MW_2PICS.BIN", 21348, "55ca122298548b28d73804c3510f9e95cf188f4abd93eb7b6bb3d083850057ea", When::kStart, "the campaign can't start"},
    // MwLoadAlternatePictureBank (MW_MAIN_081B_RESOURCES.cpp): start
    {"MW_APICS.BIN", 5013, "de30de978c3caa0185ee3c27c278babe069b329718ede911e216ba5d95901aa2", When::kStart, "the title stops"},
    // MwLoadTPictureBank (MW_MAIN_081B_RESOURCES.cpp): start
    {"MW_TPICS.BIN", 32593, "33f8ea2e502b740a19d8471c47781b2985508654f138ba346f4a9d97d8ef7c99", When::kStart, "the title stops"},
    // MwLoadMechMusic (MW_MAIN_081B_STARTUP.cpp): start
    {"MECH.MUS", 12114, "d9ecb66edad3f50fc15b57e7b2f994f63434a265235dbc5729102e4862e9da06", When::kStart, "the title has no music"},
    // MwLoadIndexedPicture (MW_MAIN_081B_RESOURCES.cpp): play
    {"MW_PICS.BIN", 328171, "d11684f462a0af5f89a3c5f018c43f4e21b623ce012a74c9934571f093eccbce", When::kPlay, "the campaign stops"},
    // MwLoadPrimaryPictureBank (MW_MAIN_081B_RESOURCES.cpp): play
    {"MW_1PICS.BIN", 26662, "803b4bdedbf48f5168393075f3e64821020dd51f1814070cc640e30869893a03", When::kPlay, "the campaign stops"},
    // MwLoadBarDeathAndLaunchMusic (MW_MAIN_081B_STARTUP.cpp): play
    {"MECHBAR.MUS", 8977, "ce494c435df9a837216b3f245c3105eb8acfc432446ce3febcb1ec0aeb3913b6", When::kPlay, "the campaign lacks music"},
    {"DEATH.MUS", 1916, "db9b1bf377ef7f8623485660107baea338b626fe45f5fbf1bb0a2a3f35761459", When::kPlay, "the campaign lacks music"},
    {"LAUNCH.MUS", 1538, "71ad889323a9a25882e537f60756130ba93532b0acbf3aef0fc132999a9e5f78", When::kPlay, "the campaign lacks music"},
    // MwLoadFPictureBank (MW_MAIN_081B_RESOURCES.cpp), the ending: some
    {"MW_FPICS.BIN", 13644, "25f711906b33e00eb1051b7b57ce432160607f527f4df1417a46410eec37028b", When::kSome, "the ending stops"},
    // MwLoadVictoryMusic (MW_MAIN_081B_STARTUP.cpp), the ending: some
    {"VICTORY.MUS", 13076, "c8812fc9f7492f5bcf9a00d9cb7eaf11ed5bfd48e25b1918b5f623888ee17502", When::kSome, "the ending has no music"},
    // BtechInitializeCombatRuntime (BTECH_0810_STARTUP.cpp): play
    {"ARCTIC.PAL", 314, "739c04752a8d84ec3a9dfc60cd1323c97791bdd823bf2a068f1817bc39724f55", When::kPlay, "a battle stops"},
    {"DESERT.PAL", 314, "ea8416b5f36a374827f565b0ad1a651cb436c8349910d00a3eccb838ec2de3ea", When::kPlay, "a battle stops"},
    {"TROPIC.PAL", 314, "4a8733ba0138b570e9c2067521219fa842858d848c65ad8948efea5732d97c7e", When::kPlay, "a battle stops"},
    {"DMGPAL.PAL", 314, "ffb525e5876c8415b3ed0ef2461bf6d9ce210e697bf7ded6760134d4d7861dbd", When::kPlay, "a battle stops"},
    {"6X6B.FNT", 588, "f8e794e6c0484dd8d6c26563a753eb3931fe4cb04ba07f4e6f119c7ef632f2f3", When::kPlay, "a battle stops"},
    {"PROMPT.BMP", 294, "7c6805f817119d9e376693f6113224bba749a073883ca10f6c16b42db4943308", When::kPlay, "a battle stops"},
    {"BTECHSND.SND", 795, "ae8fca499741a6c7b9d05f5c90da1fd921681afe35533c3f4d98b3d78e95d5c9", When::kPlay, "a battle stops"},
    // BtechLoadCockpitHudResources (BTECH_0810_COCKPIT.cpp): play
    {"POINT.BMP", 278, "43e9683506afa04294ef72e464a074284c514a8cb24d9a0276b5eda756413ee1", When::kPlay, "a battle stops"},
    {"SM_MECHS.BMP", 3072, "688bf3a104e9df42d1a90f27daa822bf225342007ab42b74cf9047628c71f522", When::kPlay, "a battle stops"},
    {"STRUTS.BMP", 1135, "a2b9d2cd714d8d9573fae54703a13cdf32c77c3c7bb5115c0ae987bba98fd9db", When::kPlay, "a battle stops"},
    {"COCKPIT.BMP", 767, "7077105462f3fc8c89245f5261699a82bccc794a1b5b79b3c3227711d2198405", When::kPlay, "a battle stops"},
    {"HUD_NUMS.BMP", 252, "682acf4122c233c531d4000195155c0573b18132d3aefc2d86a652fa272c93cb", When::kPlay, "a battle stops"},
    // BtechShowCombatStageBriefing (BTECH_0810_BRIEFING.cpp): play
    {"STATUS.SCR", 1593, "32041ea193465e452733b62a558cf7e12f6eeb2f6836fb01cd7a89082097d6f9", When::kPlay, "a battle stops"},
    // BtechRefreshLanceOrderScreen (BTECH_17FB_SCREEN.cpp), the order screen: some
    {"MAP.SCR", 3443, "dee945718c3d03f666c93421640253372ae406eb7e500c595ebc68d2111f7874", When::kSome, "the order screen stops a battle"},
    // BtechLoadScenarioAndWorld (BTECH_0810_WORLD.cpp): play
    {"SNARIO.DAT", 3648, "ab17c786bd6531a6da5e363c2531946fd59a29821ddccbf9858dae6fab9641c8", When::kPlay, "a battle stops"},
    // BtechLoadCombatChassisResourceBanks (BTECH_0810_WORLD.cpp): play
    {"TERPCK.TBL", 8293, "6b7a399e0711b9944069e0206c1bcf4bc33097809b34518a1cd886087e4c0414", When::kPlay, "a battle stops"},
    {"TERPCK.GI", 15036, "02cd058e8b93c42517450b53751230e9f48e43b0f9cc024a28f894aaced8998d", When::kPlay, "a battle stops"},
    {"OTHPCK.TBL", 2182, "1450ece44914d1383294df6ae7d396230bdc92464701794d59532cd2db8a95e3", When::kPlay, "a battle stops"},
    // BtechRefreshCockpitViewportAndGauges (BTECH_0810_COCKPIT.cpp), by the 'Mech: some
    {"LIGHT.SCR", 4703, "8fb0dc101b92672967294e43cfca3a4016348a06a03bb57247cf881bbcc24d15", When::kSome, "a battle in some 'Mechs stops"},
    {"MEDIUM.SCR", 4159, "c697f934facd5fe99b216c186950ea86eed46e4f146f2e225a54f3cff3a761e1", When::kSome, "a battle in some 'Mechs stops"},
    {"HEAVY.SCR", 3475, "63e1d01f54befd1b455ab82d44d7b54041b92b8778aa38797f90c8e33e398985", When::kSome, "a battle in some 'Mechs stops"},
    // BtechLoadCombatChassisResourceBanks (BTECH_0810_WORLD.cpp), by the 'Mechs: some
    {"LOCPCK.TBL", 26627, "39d83df552edd9c87a9c7d17fb852034de449a86d1060f5a38a7f4a4ca8b0589", When::kSome, "a battle with a Locust stops"},
    // BtechLoadCombatChassisResourceBanks (BTECH_0810_WORLD.cpp), by the 'Mechs: some
    {"JENPCK.TBL", 22866, "82fc7d5482b3410a2ba95c7d7f42982c29cc1a9c0dc22b942bb371b68be19bca", When::kSome, "a battle with a Jenner stops"},
    // BtechLoadCombatChassisResourceBanks (BTECH_0810_WORLD.cpp), by the 'Mechs: some
    {"PHAPCK.TBL", 24673, "49d3051dfb09e3a643ad5cf4204cd26189070b591d0ea3a2b5fa1738e5a1272d", When::kSome, "a battle with a Phoenix Hawk stops"},
    // BtechLoadCombatChassisResourceBanks (BTECH_0810_WORLD.cpp), by the 'Mechs: some
    {"SHAPCK.TBL", 40544, "63abc05e37d544c0c9c74593a0e1cb33ee0c5cba7267c86bd8206f5b8818e7f9", When::kSome, "a battle with a Shadow Hawk stops"},
    // BtechLoadCombatChassisResourceBanks (BTECH_0810_WORLD.cpp), by the 'Mechs: some
    {"RIFPCK.TBL", 32775, "622866d77ba6f8b55a5b6916c95222298baa6e74dc2b6699bd10c2b85dab8473", When::kSome, "a battle with a Rifleman stops"},
    // BtechLoadCombatChassisResourceBanks (BTECH_0810_WORLD.cpp), by the 'Mechs: some
    {"HAMPCK.TBL", 29960, "1812f1d922bd82141fc4c9a5380e5c3a51528a72c4c805e9dcce4b8be0bcee60", When::kSome, "a battle with a Warhammer stops"},
    // BtechLoadCombatChassisResourceBanks (BTECH_0810_WORLD.cpp), by the 'Mechs: some
    {"MARPCK.TBL", 39022, "559e679236cf6e914e0d603437b48f0d7a6bc4b5bbf13348c41997fbe500ab6f", When::kSome, "a battle with a Marauder stops"},
    // BtechLoadCombatChassisResourceBanks (BTECH_0810_WORLD.cpp), by the 'Mechs: some
    {"BMAPCK.TBL", 34973, "2b576e9a668301b7e790df7e6d8979463c72863283231158cd4338a9b522dd29", When::kSome, "a battle with a BattleMaster stops"},
    // BtechUpdateMechInspectionScreen (BTECH_0810_INSPECTION.cpp), by the 'Mech: some
    {"LOC.BMP", 3033, "5cd240f5748222cdf016ded95761dc8454fad8502d651dcf2adf73ca31c17007", When::kSome, "a look at some 'Mechs in a battle stops"},
    {"JEN.BMP", 2787, "723a86ef231350a0939aa204507636e38b8ad83fe669b0764d05df1827b2b136", When::kSome, "a look at some 'Mechs in a battle stops"},
    {"PHO.BMP", 3577, "01a776d96443b78737ac4ce233161912567b19d0e6804c1afdfa3e8ea23f107c", When::kSome, "a look at some 'Mechs in a battle stops"},
    {"SHA.BMP", 3864, "4e31d965ab1f4e957272befe0b6edb62fb1ff96361be22080c531ecf119f6757", When::kSome, "a look at some 'Mechs in a battle stops"},
    {"RIF.BMP", 4293, "feed4012ecfea7b765e0bf81961a6e40d273c6d5b5e21360f94fbecd3ae5e4c1", When::kSome, "a look at some 'Mechs in a battle stops"},
    {"WAR.BMP", 3684, "4a125cdc9fdd89258d8400ab9fbaf686c5f76ab747f5b5bc1a7ca906fbc0b9be", When::kSome, "a look at some 'Mechs in a battle stops"},
    {"MAR.BMP", 3324, "0408df3c15c27faa79741fb9f07fffcae4e591842a402045815ac12c6634b12c", When::kSome, "a look at some 'Mechs in a battle stops"},
    {"BAT.BMP", 4152, "8d74f68b08d336bdc61bae80fedcc3a76601394ee16cfaef9e1b4595e9449bf8", When::kSome, "a look at some 'Mechs in a battle stops"},
    // BtechLoadTerrainWorldResources (BTECH_0810_WORLD.cpp), at high terrain detail: some
    {"TILE0.WLD", 42, "17e027fb67112053a11fd2cd531be77aa72d710c2097eb42808cc7f5fcabf2e9", When::kSome, "some battles stop at high terrain detail"},
    {"TILE0.GRD", 4089, "37e7f89b50901ec608a96a61740f153ed9258653a35c25ab60a47006db8a89ed", When::kSome, "some battles stop at high terrain detail"},
    {"TILE1.WLD", 56, "5ba1c1e0730e911febef372217bbed130d4466266006fa8c6424938be90855d1", When::kSome, "some battles stop at high terrain detail"},
    {"TILE1.GRD", 4089, "03386b1bc6c994b3c2359bc6e871b06f3ee150b33dd4d4c3922b8ec2e8d30fcd", When::kSome, "some battles stop at high terrain detail"},
    {"TILE2.WLD", 42, "56f19eba274a2a74486da671fdb5a46ec04e5774c2239bdbb7437a135c4c4a98", When::kSome, "some battles stop at high terrain detail"},
    {"TILE2.GRD", 4089, "f74da6cfd9d062ed1ad785647858febfc83f9f98fa9bcca928b7c4c162a13ec9", When::kSome, "some battles stop at high terrain detail"},
    {"TILE3.WLD", 42, "c9fdfe25fd77f647a5eb92e00a39099153c73c5b409811b8594c24a4ac6814d5", When::kSome, "some battles stop at high terrain detail"},
    {"TILE3.GRD", 4089, "0ba0132acf086b97082a0a53e22a6074fea75345e4d883a8b24f297e39da266e", When::kSome, "some battles stop at high terrain detail"},
    {"TILE4.WLD", 56, "c3617f64307346c38d51cf986bce90874999c36d270027ec8d11c50c92b25ef4", When::kSome, "some battles stop at high terrain detail"},
    {"TILE4.GRD", 4089, "ff623d8c30cc77bc09db154b0af184a1e5c5f3ada0a2c548b9c57376cc61f5a5", When::kSome, "some battles stop at high terrain detail"},
    {"TILE5.WLD", 42, "389117dbfb03b0381c275add9912bbd14eb5a0d90c421fdd0170b22f1cd49076", When::kSome, "some battles stop at high terrain detail"},
    {"TILE5.GRD", 4089, "451f92effd235c23636027c64dc6f7856ffae6ed9bca055aa880a1d3f9a0fced", When::kSome, "some battles stop at high terrain detail"},
    {"TILE6.WLD", 56, "b638dd2e93e6e4ff79f2977d2d7f506f47adf01c0ca01c7fb2b2dd7ac22b6373", When::kSome, "some battles stop at high terrain detail"},
    {"TILE6.GRD", 4089, "620158be90b798674a2b66501c896e281c31a115cadb3f77261e4c4e8af7f91e", When::kSome, "some battles stop at high terrain detail"},
    {"TILE7.WLD", 56, "d29d85448a5bae860bb09edb0cb2d8fdcd128b388ddde03c05f9e4e2b87bdb61", When::kSome, "some battles stop at high terrain detail"},
    {"TILE7.GRD", 4089, "7fac3f460f57b0570598a32d1637488654ac2561850a08eb724a7a9f2d9e09cc", When::kSome, "some battles stop at high terrain detail"},
    {"TILE8.WLD", 56, "1d0a1af9a015a8bd712fde3fc314d5fea996d79f2d697769629ff0e411da6d1b", When::kSome, "some battles stop at high terrain detail"},
    {"TILE8.GRD", 4089, "2869bc06bfb4f78a46d00fe15e60fea074cf432532ef11e0f1a3928a85c03281", When::kSome, "some battles stop at high terrain detail"},
    {"TILE9.WLD", 56, "4ca3557ab7665ccaf5104391835fce7d5cebc5bf567e74f8181c2ac82237be18", When::kSome, "some battles stop at high terrain detail"},
    {"TILE9.GRD", 4089, "29d0843749a72625fc4edb0ac8594e5aab7dab25ae79fbf864d4225a7a6cec8e", When::kSome, "some battles stop at high terrain detail"},
    {"TILE10.WLD", 56, "d209fc4879e613ed89b54be3b3d0c217c8514b4e733b519fe2bc589358de9cf4", When::kSome, "some battles stop at high terrain detail"},
    {"TILE10.GRD", 4089, "03e9809dfe6f9a73e94b9629677fbdbad22b99ad267c7220301e50ed44ae05c5", When::kSome, "some battles stop at high terrain detail"},
    {"TILE11.WLD", 70, "2684093c40c15ea1b9301612477b9c5c852fcb66a13ea61200b9fe0801dd362a", When::kSome, "some battles stop at high terrain detail"},
    {"TILE11.GRD", 4089, "6c6aa1e7e8972caa828ae63b73a5a91c8d4f9e7cb167e5e489ef32f047a709c8", When::kSome, "some battles stop at high terrain detail"},
    {"TILE12.WLD", 42, "ac732d95f813aa373fbaa7ce994b16184f83f7fb1e9c4620b01a32f6e2a0c2e4", When::kSome, "some battles stop at high terrain detail"},
    {"TILE12.GRD", 4089, "6ec7e52d34bd79bc4ef07f17c538258c1fb703cca0ba38f6603233e6b72aa761", When::kSome, "some battles stop at high terrain detail"},
    {"TILE13.WLD", 70, "bb692644908a80d0610dc4d2b5707d8694107b7c6be1fb31fa12b57461474cec", When::kSome, "some battles stop at high terrain detail"},
    {"TILE13.GRD", 4089, "e8d00596f67953e892391bd57fd6a4a3b9b2059f57775141c99d5a511dc0bc94", When::kSome, "some battles stop at high terrain detail"},
    {"TILE14.WLD", 42, "cbbd3c108e23cace09c3d4048272f0b86b9e9955a0dbd5b34ec7ed22e306284c", When::kSome, "some battles stop at high terrain detail"},
    {"TILE14.GRD", 4089, "c5697fc67adba4b7a84aaa37ef8ff99bed6c18d2d9e971124e5ec834b53897af", When::kSome, "some battles stop at high terrain detail"},
    {"TILE15.WLD", 42, "61a76d551ef10cf4fa6e17658f51e625aa5380779b9a9cc51fbe215faca94552", When::kSome, "some battles stop at high terrain detail"},
    {"TILE15.GRD", 4089, "585ab440a19cd508e8183c41704c905582b0646f881adaa6dedae84c0bea0444", When::kSome, "some battles stop at high terrain detail"},
    // BtechLoadTerrainWorldResources (BTECH_0810_WORLD.cpp), at medium terrain detail: some
    {"TILE0B.WLD", 14, "100edd9726815137e97fe4bc0ad9fd4cd5ba8b51c6393773994bad3c46eb58ed", When::kSome, "some battles stop at medium terrain detail"},
    {"TILE0B.GRD", 4089, "480385a6935f0dc180ea1e008f463516dc75bf1a957dbe2e5df4bc520eaaff90", When::kSome, "some battles stop at medium terrain detail"},
    {"TILE1B.WLD", 28, "fe49ee571c9f01ceba0de0c525a6c451a8562863dd7ff8eb7b14d8901e806e08", When::kSome, "some battles stop at medium terrain detail"},
    {"TILE1B.GRD", 4089, "03386b1bc6c994b3c2359bc6e871b06f3ee150b33dd4d4c3922b8ec2e8d30fcd", When::kSome, "some battles stop at medium terrain detail"},
    {"TILE2B.WLD", 14, "74e01b2cfd1c07809a1086b3e9fe49dee1bda451502e07e02c0eca0bee1fa2ab", When::kSome, "some battles stop at medium terrain detail"},
    {"TILE2B.GRD", 4089, "f74da6cfd9d062ed1ad785647858febfc83f9f98fa9bcca928b7c4c162a13ec9", When::kSome, "some battles stop at medium terrain detail"},
    {"TILE3B.WLD", 14, "50982cdf9dae4252d52adba52a5bef22f91031d5880ab6fd77e1fcae2153c239", When::kSome, "some battles stop at medium terrain detail"},
    {"TILE3B.GRD", 4089, "1865a679087690a1cf3de06e50c642a333b27b73ebe2a8636ef445dcb49abf14", When::kSome, "some battles stop at medium terrain detail"},
    {"TILE4B.WLD", 14, "708f1de9c277e4d7e9bc74fb8f14c568274f4ebb72fe7ecfbf6f95a0517048d2", When::kSome, "some battles stop at medium terrain detail"},
    {"TILE4B.GRD", 4089, "645ec69ddd6b1d9b075a97c1bbd20be36c5a9b2370c0e7f6b5f9055b9c743c25", When::kSome, "some battles stop at medium terrain detail"},
    {"TILE5B.WLD", 14, "5900c0d32f7ceeeb936409b245eb78d01831ab0f12888e0889962c310aaf160f", When::kSome, "some battles stop at medium terrain detail"},
    {"TILE5B.GRD", 4089, "624380b496938ec4ddb21a79c64289d3b5832369ee3df0f5d528f09a165011cf", When::kSome, "some battles stop at medium terrain detail"},
    {"TILE6B.WLD", 14, "28a26a34bdf0ca9b2421ab397864c0c97fdeaff07a274f8e014b1306c5032670", When::kSome, "some battles stop at medium terrain detail"},
    {"TILE6B.GRD", 4089, "75a5e3ac665987121c8cf9e1d3b1760a92851b4497850da7d8b26f836de13ea3", When::kSome, "some battles stop at medium terrain detail"},
    {"TILE7B.WLD", 28, "a40f380aa2ff19fdb0f9f443b2834cc33cb2d3eadc3a29af41ae554a9c4ee96d", When::kSome, "some battles stop at medium terrain detail"},
    {"TILE7B.GRD", 4089, "372ebe554dd019898f23aa07707610442991aa08774365a252f662c88dcd14ad", When::kSome, "some battles stop at medium terrain detail"},
    {"TILE8B.WLD", 28, "cb4dadce7de81a66748335fa2691f57a447cd75bb250685d15430e7ce40e4f7f", When::kSome, "some battles stop at medium terrain detail"},
    {"TILE8B.GRD", 4089, "784f161072cdedec84b301c5973b0f4958d9e3fb20a5b8942b88819d513f8e1f", When::kSome, "some battles stop at medium terrain detail"},
    {"TILE9B.WLD", 28, "c7442afb19a43e47b3823b0bdbad14888e3dbfc54d3260bd719079449f4c4cac", When::kSome, "some battles stop at medium terrain detail"},
    {"TILE9B.GRD", 4089, "8bcaaa4f8e625b474cb6e2a2ab926975e532f3ec45e091fd9a72820220304666", When::kSome, "some battles stop at medium terrain detail"},
    {"TILE10B.WLD", 28, "b3ecfdec0118c7eb93b6eaef5cb0115cf36fbc27f7373d3755264e28e326d108", When::kSome, "some battles stop at medium terrain detail"},
    {"TILE10B.GRD", 4089, "a410c8f3d2c0cf1098774d557b27528689302ebef9f9806bd0964fdb00e86f39", When::kSome, "some battles stop at medium terrain detail"},
    {"TILE11B.WLD", 42, "e5707675b7a60fcde79474bce9af12aa331249ce85b27e0542fa3691955b2975", When::kSome, "some battles stop at medium terrain detail"},
    {"TILE11B.GRD", 4089, "6c6aa1e7e8972caa828ae63b73a5a91c8d4f9e7cb167e5e489ef32f047a709c8", When::kSome, "some battles stop at medium terrain detail"},
    {"TILE12B.WLD", 28, "1aabda3ddd16d97900192c9e2650666f935b7fc98d24b2949db9028c6b480621", When::kSome, "some battles stop at medium terrain detail"},
    {"TILE12B.GRD", 4089, "9d10725d03dc94cd0cea6a88fd6b003a536139b7115ce9dd950e47abeee5a2fe", When::kSome, "some battles stop at medium terrain detail"},
    {"TILE13B.WLD", 42, "13c20ef2942712746d35ea06b15d8b1fb3b261b0207171d5748ebb5c984aa0f5", When::kSome, "some battles stop at medium terrain detail"},
    {"TILE13B.GRD", 4089, "6c6aa1e7e8972caa828ae63b73a5a91c8d4f9e7cb167e5e489ef32f047a709c8", When::kSome, "some battles stop at medium terrain detail"},
    {"TILE14B.WLD", 28, "d5d7f87476cc93d3126e260d4fa9b16c14364f37fc66aa8f885e0ec766f67409", When::kSome, "some battles stop at medium terrain detail"},
    {"TILE14B.GRD", 4089, "1c24382ef765da65875593057613a03e1ed5a564e4291679eed761e7fe46e06d", When::kSome, "some battles stop at medium terrain detail"},
    {"TILE15B.WLD", 14, "74926113db5ab55f015a597eb59b83f339650f45943b9324c1f896023c6221f0", When::kSome, "some battles stop at medium terrain detail"},
    {"TILE15B.GRD", 4089, "585ab440a19cd508e8183c41704c905582b0646f881adaa6dedae84c0bea0444", When::kSome, "some battles stop at medium terrain detail"},
};
constexpr KnownDamage kDamaged[] = {
    {"PHAPCK.TBL", 24673, "d31595562474ed3ba6a8a4360116789f47b9a7392e8d02a527939ae57b7a4f76"},
};

}  // namespace

std::span<const GameFile> game_files() { return kFiles; }
std::span<const KnownDamage> known_damage() { return kDamaged; }

}  // namespace mw1o::setup
