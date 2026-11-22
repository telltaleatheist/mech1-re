cmake_minimum_required(VERSION 3.20)
# mw1-original: the game is C++20 (mw3-re's standard); C stays enabled for Nuked-OPL3's opl3.c until our own
# synth replaces it.
project(MechWarrior C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(mechwarrior_original STATIC
    src/Original/campaign_graphics.cpp
    src/Original/Campaign/MW_MAIN_081B.cpp
    src/Original/Campaign/MW_MAIN_081B_RESOURCES.cpp
    src/Original/Campaign/MW_MAIN_081B_TEXT.cpp
    src/Original/Campaign/MW_MAIN_081B_PLANET.cpp
    src/Original/Campaign/MW_MAIN_081B_REPAIR.cpp
    src/Original/Campaign/MW_MAIN_081B_PILOTS.cpp
    src/Original/Campaign/MW_MAIN_081B_TRAVEL.cpp
    src/Original/Campaign/MW_MAIN_081B_ECONOMY.cpp
    src/Original/Campaign/MW_MAIN_081B_CONTRACTS.cpp
    src/Original/Campaign/MW_MAIN_081B_DEPLOYMENT.cpp
    src/Original/Campaign/MW_MAIN_081B_OPTIONS.cpp
    src/Original/Campaign/MW_MAIN_SAVE_FORMAT.cpp
    src/Original/Campaign/MW_MAIN_081B_SAVE.cpp
    src/Original/Campaign/MW_MAIN_081B_NEWS.cpp
    src/Original/Campaign/MW_MAIN_081B_UI.cpp
    src/Original/Campaign/MW_MAIN_081B_SERVICE.cpp
    src/Original/Campaign/MW_MAIN_081B_MARKET.cpp
    src/Original/Campaign/MW_MAIN_081B_INPUT.cpp
    src/Original/Campaign/MW_MAIN_081B_STARTUP.cpp
    src/Original/Campaign/MW_MAIN_081B_EVENTS.cpp
    src/Original/Campaign/MW_MAIN_081B_EVENT_UI.cpp
    src/Original/Campaign/MW_MAIN_081B_DISK.cpp
    src/Original/Memory/MW_MAIN_1090.cpp
    src/Original/Memory/MW_MAIN_1090_OPTIONS.cpp
    src/Original/Memory/MW_MAIN_1090_LAYOUT.cpp
    src/Original/Memory/MW_0806_STATE.cpp
    src/Original/Launcher/MW_0806.cpp
    src/Original/Combat/BTECH_0810.cpp
    src/Original/Combat/BTECH_0810_IMPORT.cpp
    src/Original/Combat/BTECH_0810_PLATFORM.cpp
    src/Original/Combat/BTECH_0810_RESOURCES.cpp
    src/Original/Combat/BTECH_0810_WORLD.cpp
    src/Original/Combat/BTECH_0810_STARTUP.cpp
    src/Original/Combat/BTECH_0810_INPUT.cpp
    src/Original/Runtime/BTECH_1BF5_INPUT.cpp
    src/Original/Runtime/BTECH_1BF5_KEYBOARD.cpp
    src/Original/Combat/BTECH_0810_POSE.cpp
    src/Original/Combat/BTECH_0810_ANIMATION.cpp
    src/Original/Combat/BTECH_0810_MECH_TICK.cpp
    src/Original/Combat/BTECH_0810_SURFACE.cpp
    src/Original/Combat/BTECH_0810_MOVEMENT.cpp
    src/Original/Combat/BTECH_0810_DAMAGE.cpp
    src/Original/Combat/BTECH_17FB_DAMAGE.cpp
    src/Original/Combat/BTECH_0810_WEAPON_GEOMETRY.cpp
    src/Original/Combat/BTECH_0810_WEAPONS.cpp
    src/Original/Combat/BTECH_0810_PROJECTILES.cpp
    src/Original/Combat/BTECH_0810_PLAYER_CONTROLS.cpp
    src/Original/Combat/BTECH_0810_AI_MOTION.cpp
    src/Original/Combat/BTECH_0810_AI_WEAPONS.cpp
    src/Original/Combat/BTECH_0810_NAVIGATION.cpp
    src/Original/Combat/BTECH_0810_TACTICS.cpp
    src/Original/Combat/BTECH_0810_TACTICAL_NAVIGATION.cpp
    src/Original/Combat/BTECH_0810_AI.cpp
    src/Original/Combat/BTECH_0810_MAIN.cpp
    src/Original/Combat/BTECH_17FB_TACTICS.cpp
    src/Original/Combat/BTECH_0810_EFFECTS.cpp
    src/Original/Combat/BTECH_0810_SCENE.cpp
    src/Original/Memory/BTECH_304E_POSE.cpp
    src/Original/Memory/BTECH_304E_SCENE.cpp
    src/Original/Combat/BTECH_0810_ORDERING.cpp
    src/Original/Combat/BTECH_17FB_ORDERING.cpp
    src/Original/Combat/BTECH_17FB_ORDERS.cpp
    src/Original/Combat/BTECH_17FB_PANELS.cpp
    src/Original/Combat/BTECH_17FB_SCREEN.cpp
    src/Original/Combat/BTECH_17FB_MAP.cpp
    src/Original/Combat/BTECH_0810_COCKPIT.cpp
    src/Original/Combat/BTECH_0810_HUD.cpp
    src/Original/Combat/BTECH_0810_HUD_TEXT.cpp
    src/Original/Combat/BTECH_0810_RADAR.cpp
    src/Original/Combat/BTECH_0810_WEAPON_HUD.cpp
    src/Original/Combat/BTECH_0810_TARGET_HUD.cpp
    src/Original/Combat/BTECH_17FB_RADAR.cpp
    src/Original/Video/BTECH_2A36_PRESENT.cpp
    src/Original/Combat/BTECH_0810_GAUGES.cpp
    src/Original/Combat/BTECH_17FB_HUD.cpp
    src/Original/Combat/BTECH_0810_BRIEFING.cpp
    src/Original/Combat/BTECH_0810_DIALOGS.cpp
    src/Original/Combat/BTECH_0810_INSPECTION.cpp
    src/Original/Combat/BTECH_0810_STAGE_RECOVERY.cpp
    src/Original/Combat/BTECH_0810_RESULTS.cpp
    src/Original/Combat/BTECH_0810_RENDER.cpp
    src/Original/Combat/BTECH_0810_CONTROLS.cpp
    src/Original/Combat/BTECH_17FB_STATE.cpp
    src/Original/Audio/BTECH_1ADF.cpp
    src/Original/Audio/BTECH_1ADF_INSTALL.cpp
    src/Original/Audio/BTECH_229C.cpp
    src/Original/Audio/BTECH_21CA.cpp
    src/Original/Audio/BTECH_2176.cpp
    src/Original/Memory/BTECH_304E_ADLIB.cpp
    src/Original/Memory/BTECH_304E_MPU.cpp
    src/Original/Audio/BTECH_0810_SOUND.cpp
    src/Original/Audio/BTECH_2134.cpp
    src/Original/Runtime/BTECH_0810_STREAMS.cpp
    src/Original/Runtime/BTECH_0810_DIVISION.cpp
    src/Original/Runtime/BTECH_0810_EXIT.cpp
    src/Original/Runtime/BTECH_0810_HEAP.cpp
    src/Original/Runtime/BTECH_1BF5_TAGS.cpp
    src/Original/Runtime/BTECH_1BF5_CLEANUP.cpp
    src/Original/Runtime/BTECH_1BF5_INSTALL.cpp
    src/Original/Runtime/BTECH_1BF5_DIVIDE.cpp
    src/Original/Runtime/BTECH_1BF5_ARCHIVE.cpp
    src/Original/Runtime/BTECH_1BF5_PLATFORM.cpp
    src/Original/Geometry/BTECH_22B6_RESOURCES.cpp
    src/Original/Geometry/BTECH_22B6_WORLD.cpp
    src/Original/Geometry/BTECH_22B6_STARTUP.cpp
    src/Original/Geometry/BTECH_22B6_CAMERA.cpp
    src/Original/Geometry/BTECH_22B6_COMPOSITION.cpp
    src/Original/Geometry/BTECH_22B6_RESOURCE_RENDER.cpp
    src/Original/Geometry/BTECH_22B6_VERTEX_CACHE.cpp
    src/Original/Geometry/BTECH_22B6_POLYGON_COLOURS.cpp
    src/Original/Geometry/BTECH_22B6_WIDE_POLYGONS.cpp
    src/Original/Geometry/BTECH_22B6_WORD_POLYGONS.cpp
    src/Original/Geometry/BTECH_22B6_WIDE_CLIP.cpp
    src/Original/Combat/BTECH_0810_GEOMETRY.cpp
    src/Original/Geometry/BTECH_22B6_HORIZON.cpp
    src/Original/Geometry/BTECH_22B6_PROJECTION.cpp
    src/Original/Geometry/BTECH_22B6_DIVISION.cpp
    src/Original/Video/BTECH_1BF5.cpp
    src/Original/Video/BTECH_2A36.cpp
    src/Original/Video/BTECH_1BF5_BITMAPS.cpp
    src/Original/Video/BTECH_2A36_BITMAPS.cpp
    src/Original/Video/BTECH_2A36_BLIT.cpp
    src/Original/Video/BTECH_2A36_CAPTURE.cpp
    src/Original/Video/BTECH_1BF5_SCREENS.cpp
    src/Original/Video/BTECH_1BF5_SHAPES.cpp
    src/Original/Video/BTECH_1BF5_ELLIPSE.cpp
    src/Original/Video/BTECH_1BF5_POLYGONS.cpp
    src/Original/Video/BTECH_1BF5_POLYGON_CLIP.cpp
    src/Original/Video/BTECH_1BF5_POLYGON_EDGES.cpp
    src/Original/Video/BTECH_1BF5_LINES.cpp
    src/Original/Video/BTECH_2A36_POLYGONS.cpp
    src/Original/Video/BTECH_2A36_LINES.cpp
    src/Original/Video/BTECH_2A36_PIXELS.cpp
    src/Original/Video/BTECH_1BF5_PIXELS.cpp
    src/Original/Video/BTECH_2A36_SPANS.cpp
    src/Original/Video/BTECH_2A36_SCREENS.cpp
    src/Original/Combat/BTECH_CAMPAIGN_TRANSFER.cpp
    src/Original/Memory/BTECH_304E.cpp
    src/Original/Memory/BTECH_304E_WORLD.cpp
    src/Original/Memory/BTECH_304E_SOUND.cpp
    src/Original/Memory/BTECH_304E_STARTUP.cpp
    src/Original/Memory/BTECH_304E_ORDERS.cpp
    src/Original/Memory/BTECH_304E_RUNTIME.cpp
    src/Original/Memory/BTECH_304E_VIDEO.cpp
    src/Original/Memory/BTECH_304E_COCKPIT.cpp
    src/Original/Memory/BTECH_304E_RENDER.cpp
    src/Original/Memory/BTECH_304E_POLYGONS.cpp
    src/Original/Memory/BTECH_2FEB.cpp
    src/Original/Memory/BTECH_1ADF_STATE.cpp
    src/Original/Memory/BTECH_0810_STATE.cpp
    src/Original/Memory/BTECH_1BF5_STATE.cpp
    src/Original/Memory/BTECH_22B6_DATA.cpp
    src/Support/CombatPlatform.cpp
    src/Support/CombatInstallation.cpp
    src/Support/BtechImage.cpp
    src/Support/CombatNearHeap.cpp
    src/Original/Memory/MW_MAIN_1090_EVENTS.cpp
    src/Original/Memory/MW_MAIN_1AA5.cpp
    src/Original/Memory/MW_MAIN_28BE.cpp
    src/Original/Memory/MW_MAIN_0F41_STATE.cpp
    src/Original/Runtime/MW_MAIN_0F41.cpp
    src/Original/Runtime/MW_MAIN_0F41_SYNTH.cpp
    src/Original/Runtime/MW_MAIN_0F41_MPU.cpp
    src/Original/Runtime/MW_MAIN_0F41_ADLIB.cpp
    src/Original/Runtime/MW_MAIN_0800.cpp
    src/Original/Memory/MW_EGA_0800_STATE.cpp
    src/Original/Video/MW_EGA_0800.cpp
    src/SDL/SDLBackend_Files.cpp
    src/Support/NativeAddress.cpp
    src/Support/PortDiagnostics.cpp
    src/Support/MidiAddress.cpp
    src/Support/Json.cpp
    src/Support/Sha256.cpp
    src/Support/Hmac.cpp
    src/Import/import_table.cpp
    src/Import/Story.cpp
    src/Import/Records.cpp
    src/Import/Events.cpp
    src/Import/Music.cpp
    src/Import/StoryHost.cpp
    src/Rules/rule_table.cpp
    src/Rules/new_campaign_fields.cpp
    src/Rules/Rules.cpp
    src/Rules/RulesHost.cpp
    src/Support/NativePieces.cpp
    src/Ui/ui_table.cpp
    src/Ui/ui_actions.cpp
    src/Ui/Ui.cpp
    src/Ui/UiHost.cpp)
target_include_directories(mechwarrior_original PUBLIC src/include)
set_target_properties(mechwarrior_original PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Unit tests for our own support code (no SDL, no game files): ctest.
option(MECHWARRIOR_BUILD_TESTS "Build the unit tests" ON)
if(MECHWARRIOR_BUILD_TESTS)
    enable_testing()
    add_executable(json_test tests/json_test.cpp src/Support/Json.cpp)
    add_test(NAME json COMMAND json_test)
    add_executable(story_test tests/story_test.cpp src/Import/Story.cpp src/Import/Records.cpp src/Import/Events.cpp src/Import/Music.cpp src/Support/Json.cpp src/Support/Sha256.cpp src/Support/Hmac.cpp
        src/Support/BtechImage.cpp)
    target_include_directories(story_test PRIVATE src/include)
    add_test(NAME story COMMAND story_test)
    add_executable(records_test tests/records_test.cpp src/Import/Records.cpp src/Import/Events.cpp src/Import/Music.cpp src/Import/Story.cpp src/Support/Json.cpp
        src/Support/Sha256.cpp src/Support/Hmac.cpp src/Support/BtechImage.cpp)
    target_include_directories(records_test PRIVATE src/include)
    add_test(NAME records COMMAND records_test)
    # Needs the player's files (MW1O_GAME, MW1O_BTECH_REFERENCE, MW1O_BTECH_1990); skipped without them.
    add_executable(btech_revisions_test tests/btech_revisions_test.cpp src/Import/Story.cpp src/Import/Records.cpp src/Import/Events.cpp src/Import/Music.cpp
        src/Import/import_table.cpp src/Support/Json.cpp src/Support/Sha256.cpp src/Support/Hmac.cpp src/Support/BtechImage.cpp)
    target_include_directories(btech_revisions_test PRIVATE src/include)
    add_test(NAME btech_revisions COMMAND btech_revisions_test)
    set_tests_properties(btech_revisions PROPERTIES SKIP_RETURN_CODE 77)
    # The UI words (story/words.json) against the player's exes, in memory (MW1O_GAME,
    # optionally MW1O_BTECH_REFERENCE); and the words' doors and refusals on made-up files.
    add_executable(words_exe_test tests/words_exe_test.cpp src/Import/Story.cpp src/Import/Records.cpp src/Import/Events.cpp src/Import/Music.cpp
        src/Import/import_table.cpp src/Support/Json.cpp src/Support/Sha256.cpp src/Support/Hmac.cpp src/Support/BtechImage.cpp)
    target_include_directories(words_exe_test PRIVATE src/include)
    add_test(NAME words_exe COMMAND words_exe_test)
    set_tests_properties(words_exe PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(words_test tests/words_test.cpp src/Import/Story.cpp src/Import/Records.cpp src/Import/Events.cpp src/Import/Music.cpp
        src/Support/Json.cpp src/Support/Sha256.cpp src/Support/Hmac.cpp src/Support/BtechImage.cpp)
    target_include_directories(words_test PRIVATE src/include)
    add_test(NAME words COMMAND words_test)
    # data/rules against the rule table's hashes (no game files), and the
    # rules' strict reading on made-up files.
    add_executable(rules_test tests/rules_test.cpp src/Rules/Rules.cpp src/Rules/rule_table.cpp src/Rules/new_campaign_fields.cpp src/Support/Json.cpp
        src/Support/Sha256.cpp)
    add_test(NAME rules COMMAND rules_test "${CMAKE_CURRENT_SOURCE_DIR}/data")
    # The rule table against the player's exes (MW1O_GAME, optionally MW1O_BTECH_REFERENCE); skipped without them.
    add_executable(rules_exe_test tests/rules_exe_test.cpp src/Rules/rule_table.cpp src/Support/BtechImage.cpp src/Support/Sha256.cpp)
    target_include_directories(rules_exe_test PRIVATE src/include)
    add_test(NAME rules_exe COMMAND rules_exe_test)
    set_tests_properties(rules_exe PROPERTIES SKIP_RETURN_CODE 77)
    # data/ui against the layout table's hashes (no game files), and its strict reading.
    add_executable(ui_test tests/ui_test.cpp src/Ui/Ui.cpp src/Ui/ui_table.cpp src/Ui/ui_actions.cpp src/Import/import_table.cpp src/Support/Json.cpp src/Support/Sha256.cpp)
    target_include_directories(ui_test PRIVATE src/include)
    add_test(NAME ui COMMAND ui_test "${CMAKE_CURRENT_SOURCE_DIR}/data")
    # The layout table against the player's exes; skipped without them.
    add_executable(ui_exe_test tests/ui_exe_test.cpp src/Ui/ui_table.cpp src/Support/BtechImage.cpp src/Support/Sha256.cpp)
    target_include_directories(ui_exe_test PRIVATE src/include)
    add_test(NAME ui_exe COMMAND ui_exe_test)
    set_tests_properties(ui_exe PROPERTIES SKIP_RETURN_CODE 77)
    # The whole import at start (story, rules, ui, MW_MAIN's and BTECH's
    # importers) on the player's files; any refusal fails. Skipped without them.
    add_executable(import_exe_test tests/import_exe_test.cpp)
    target_link_libraries(import_exe_test PRIVATE mechwarrior_original)
    add_test(NAME import_exe COMMAND import_exe_test "${CMAKE_CURRENT_SOURCE_DIR}/data" "${CMAKE_CURRENT_BINARY_DIR}/import_exe_work")
    set_tests_properties(import_exe PROPERTIES SKIP_RETURN_CODE 77)
endif()

# BTECH.EXE's unpacked image on stdout, for tools/import/make_import_table.py.
add_executable(btech_image tools/import/btech_image.cpp src/Support/BtechImage.cpp)
target_include_directories(btech_image PRIVATE src/include)

# No text from MW1's exes in the tracked tree (docs/phase2/PLAN.md 7.4): the
# player's exes (MW1O_GAME, MW1O_BTECH_REFERENCE, MW1O_BTECH_1990) against
# git ls-files. Prints addresses and lengths, never the text; skipped without them.
if(MECHWARRIOR_BUILD_TESTS)
    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_FOUND)
        add_test(NAME text_audit COMMAND Python3::Interpreter -I "${CMAKE_CURRENT_SOURCE_DIR}/tools/phase2/text_audit.py" $<TARGET_FILE:btech_image>)
        set_tests_properties(text_audit PROPERTIES SKIP_RETURN_CODE 77)
    endif()
endif()


option(MECHWARRIOR_BUILD_SDL "Build the playable SDL3 game" ON)
if(MECHWARRIOR_BUILD_SDL)
    add_subdirectory(src/MT32)
    add_subdirectory(src/Adlib)
    add_subdirectory(src/Audio)
    set(MECHWARRIOR_SDL_SOURCE_DIR "" CACHE PATH "Optional existing SDL3 source tree; never downloaded")
    if(MECHWARRIOR_SDL_SOURCE_DIR)
        set(SDL_SHARED ON CACHE BOOL "" FORCE)
        set(SDL_STATIC OFF CACHE BOOL "" FORCE)
        set(SDL_TESTS OFF CACHE BOOL "" FORCE)
        set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
        add_subdirectory("${MECHWARRIOR_SDL_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/sdl3")
    else()
        find_package(SDL3 3.2 CONFIG REQUIRED)
    endif()
    add_library(mechwarrior_sdl STATIC
        src/SDL/SDLBackend_Main.cpp src/SDL/SDLBackend_Input.cpp src/SDL/SDLBackend_Time.cpp
        src/SDL/SDLBackend_Config.cpp src/MT32/MPU401.cpp
        src/SDL/SDLBackend_DebugControl.cpp
        src/SDL/SDLBackend_Pilot.cpp src/Support/SupportPilot.cpp src/Support/SupportPilotObservation.cpp
        src/SDL/SDLBackend_Video.cpp src/SDL/SDLBackend_CombatVideo.cpp src/SDL/SDLBackend_CombatPlatform.cpp
        src/SDL/SDLBackend_CombatProcess.cpp src/SDL/SDLBackend_Audio.cpp)
    target_link_libraries(mechwarrior_sdl PUBLIC mechwarrior_original SDL3::SDL3 mechwarrior_audio mechwarrior_mt32 mechwarrior_adlib)
    if(NOT MSVC)
        target_link_libraries(mechwarrior_sdl PUBLIC m)
    endif()
    target_include_directories(mechwarrior_sdl PRIVATE src/SDL)
    add_executable(mechwarrior src/SDL/SDLMain.cpp)
    target_link_libraries(mechwarrior PRIVATE mechwarrior_sdl)
    # Our data/ (the rules) beside the program, where the game looks for it;
    # copied on every build so an edited file is picked up.
    add_custom_target(mechwarrior_data ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/data" "$<TARGET_FILE_DIR:mechwarrior>/data")
    add_dependencies(mechwarrior mechwarrior_data)
    add_custom_command(TARGET mechwarrior POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:mt32emu>" "$<TARGET_FILE_DIR:mechwarrior>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${munt_SOURCE_DIR}/mt32emu/COPYING.LESSER.txt" "$<TARGET_FILE_DIR:mechwarrior>/libmt32emu-LICENSE.txt")
    add_custom_command(TARGET mechwarrior POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:nukedopl>" "$<TARGET_FILE_DIR:mechwarrior>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${nukedopl_SOURCE_DIR}/LICENSE" "$<TARGET_FILE_DIR:mechwarrior>/Nuked-OPL3-LICENSE.txt")
    if(TARGET SDL3::SDL3-shared)
        add_custom_command(TARGET mechwarrior POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:SDL3::SDL3-shared>" "$<TARGET_FILE_DIR:mechwarrior>")
    endif()
    if(MSVC)
        target_link_libraries(mechwarrior PRIVATE dbghelp)
        # Retain source locations in optimized builds for user crash reports.
        foreach(diagnostic_target mechwarrior_original mechwarrior_sdl mechwarrior)
            target_compile_options(${diagnostic_target} PRIVATE $<$<CONFIG:Release>:/Zi>)
        endforeach()
        target_link_options(mechwarrior PRIVATE $<$<CONFIG:Release>:/DEBUG:FULL>)
    endif()
endif()
