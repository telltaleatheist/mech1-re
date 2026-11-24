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

# A package build (tools/build_package.py) keeps the build machine's folders
# out of the program: __FILE__ in failsafes and in SDL's asserts names
# mech1-re/, SDL/ and build/ instead, and on Windows the PDB is named without
# its folder (tools/package/check_paths.py checks every shipped file). It must
# come before any target, SDL's and the audio libraries' included.
option(MECHWARRIOR_TRIM_PATHS "Name source files relative to the tree in the program (packages)" OFF)
if(MECHWARRIOR_TRIM_PATHS)
    set(mechwarrior_path_maps "${CMAKE_SOURCE_DIR}=mech1-re")
    if(MECHWARRIOR_SDL_SOURCE_DIR)
        list(APPEND mechwarrior_path_maps "${MECHWARRIOR_SDL_SOURCE_DIR}=SDL")
    endif()
    list(APPEND mechwarrior_path_maps "${CMAKE_BINARY_DIR}=build")  # last: it may sit inside the tree, and the last match wins
    foreach(path_map IN LISTS mechwarrior_path_maps)
        string(REGEX REPLACE "=.*" "" from "${path_map}")
        if(MSVC)
            file(TO_NATIVE_PATH "${from}/" native_from)
            add_compile_options("/d1trimfile:${native_from}" "/d1trimfile:${from}/")
        else()
            add_compile_options("-ffile-prefix-map=${path_map}")
        endif()
    endforeach()
    if(MSVC)
        add_link_options("/PDBALTPATH:%_PDB%")
    endif()
endif()

# Our JSON reader (data/, the story file), shared by the game library and the music (src/Audio).
add_library(mechwarrior_json STATIC src/Support/Json.cpp)
target_compile_features(mechwarrior_json PUBLIC cxx_std_20)

# Our own screen (src/Video): the canvas the game draws into.
add_subdirectory(src/Video)

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
    src/Original/Memory/NativeStart.cpp
    src/Original/Memory/BtechStartImage.cpp
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
    src/Original/Video/BTECH_2A36_CANVAS.cpp
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
    src/Original/Memory/BTECH_1BF5_STATE.cpp
    src/Original/Memory/BTECH_22B6_DATA.cpp
    src/Support/CombatPlatform.cpp
    src/Support/VideoMode.cpp
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
    src/Support/GameFiles.cpp
    src/Support/NativeAddress.cpp
    src/Support/PortDiagnostics.cpp
    src/Support/MidiAddress.cpp
    src/Support/Sha256.cpp
    src/Support/EventLog.cpp
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
target_link_libraries(mechwarrior_original PUBLIC mechwarrior_json mechwarrior_video)
set_target_properties(mechwarrior_original PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Unit tests for our own support code (no SDL, no game files): ctest.
option(MECHWARRIOR_BUILD_TESTS "Build the unit tests" ON)
if(MECHWARRIOR_BUILD_TESTS)
    enable_testing()
    add_executable(json_test tests/json_test.cpp src/Support/Json.cpp)
    add_test(NAME json COMMAND json_test)
    # The canvas (src/Video, G1): pages, palette codes' colours, present().
    add_executable(canvas_test tests/canvas_test.cpp)
    target_link_libraries(canvas_test PRIVATE mechwarrior_video)
    add_test(NAME canvas COMMAND canvas_test)
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
    # optionally MW1O_BTECH_REFERENCE and MW1O_MAIN_1989); and the words' doors and refusals on made-up files.
    add_executable(words_exe_test tests/words_exe_test.cpp src/Import/Story.cpp src/Import/Records.cpp src/Import/Events.cpp src/Import/Music.cpp
        src/Import/import_table.cpp src/Ui/ui_table.cpp src/Support/Json.cpp src/Support/Sha256.cpp src/Support/Hmac.cpp src/Support/BtechImage.cpp)
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
    # The program's start constants and file names against the player's exes; skipped without them.
    add_executable(start_exe_test tests/start_exe_test.cpp)
    target_link_libraries(start_exe_test PRIVATE mechwarrior_original)
    add_test(NAME start_exe COMMAND start_exe_test)
    set_tests_properties(start_exe PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(import_exe_test tests/import_exe_test.cpp)
    target_link_libraries(import_exe_test PRIVATE mechwarrior_original)
    # Combat's drawing on the canvas (G3, G4): the pen and the game's planar form.
    add_executable(ega_canvas_writer_test tests/ega_canvas_writer_test.cpp)
    target_include_directories(ega_canvas_writer_test PRIVATE src/Original/Video)
    target_link_libraries(ega_canvas_writer_test PRIVATE mechwarrior_original)
    add_test(NAME ega_canvas_writer COMMAND ega_canvas_writer_test)
    # The C runtime's kept rules (B5): the near heap's exhaustion and fragmentation, the 1989 long divide.
    add_executable(runtime_test tests/runtime_test.cpp)
    target_link_libraries(runtime_test PRIVATE mechwarrior_original)
    add_test(NAME runtime COMMAND runtime_test)
    # The game's files at play (B6): each rule, on files the test makes; the save's file half, by field.
    add_executable(game_files_test tests/game_files_test.cpp)
    target_link_libraries(game_files_test PRIVATE mechwarrior_original)
    add_test(NAME game_files COMMAND game_files_test "${CMAKE_CURRENT_BINARY_DIR}/game_files_work")
    add_executable(save_roundtrip_test tests/save_roundtrip_test.cpp)
    target_link_libraries(save_roundtrip_test PRIVATE mechwarrior_original)
    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_FOUND)
        add_test(NAME save_roundtrip COMMAND Python3::Interpreter -I "${CMAKE_CURRENT_SOURCE_DIR}/tests/save_roundtrip.py"
            --program $<TARGET_FILE:save_roundtrip_test> --data "${CMAKE_CURRENT_SOURCE_DIR}/data" --work "${CMAKE_CURRENT_BINARY_DIR}/save_roundtrip_work")
    endif()
    # A connected joystick in both games (H3): calibration and reads on fake axes.
    add_executable(joystick_test tests/joystick_test.cpp)
    target_link_libraries(joystick_test PRIVATE mechwarrior_original)
    add_test(NAME joystick COMMAND joystick_test)
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
        # The trace tools' palette rule against the port's, over every code (G1b).
        add_executable(palette_rule tests/palette_rule.cpp)
        target_link_libraries(palette_rule PRIVATE mechwarrior_video)
        add_test(NAME palette_rule COMMAND Python3::Interpreter -I "${CMAKE_CURRENT_SOURCE_DIR}/tools/trace/test_palette.py" $<TARGET_FILE:palette_rule>)
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
    # The shared setup kit (extern/re-setup-kit, a submodule; DESIGN.md there): finding the player's copy in a
    # folder or an archive, the launcher contract (--status, --setup). It reads archives through libarchive with
    # zlib and liblzma, static in packages (built from pinned sources, as mech3-re's), else the system's; without
    # it setup reads folders only.
    # tools/deps/build_archive.sh's prefix first (mac or linux), then tools/windows/build_deps_archive.cmd's.
    if(APPLE)
        set(MECHWARRIOR_DEPS_PLATFORM mac)
    else()
        set(MECHWARRIOR_DEPS_PLATFORM linux)
    endif()
    find_path(MECHWARRIOR_LIBARCHIVE_INCLUDE archive.h
              HINTS "${CMAKE_SOURCE_DIR}/deps/archive/include" "${CMAKE_SOURCE_DIR}/build-deps/${MECHWARRIOR_DEPS_PLATFORM}/prefix/include" /opt/homebrew/opt/libarchive/include /usr/local/opt/libarchive/include)
    find_library(MECHWARRIOR_LIBARCHIVE_LIB NAMES archive archive_static
                 HINTS "${CMAKE_SOURCE_DIR}/deps/archive/lib" "${CMAKE_SOURCE_DIR}/build-deps/${MECHWARRIOR_DEPS_PLATFORM}/prefix/lib" /opt/homebrew/opt/libarchive/lib /usr/local/opt/libarchive/lib)
    if(MECHWARRIOR_LIBARCHIVE_INCLUDE AND MECHWARRIOR_LIBARCHIVE_LIB)
        set(REKIT_LIBARCHIVE_INCLUDE "${MECHWARRIOR_LIBARCHIVE_INCLUDE}" CACHE PATH "" FORCE)
        set(REKIT_LIBARCHIVE_LIB "${MECHWARRIOR_LIBARCHIVE_LIB}" CACHE FILEPATH "" FORCE)
    endif()
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/re-setup-kit/CMakeLists.txt")
        message(FATAL_ERROR "extern/re-setup-kit is missing: git submodule update --init")
    endif()
    add_subdirectory(extern/re-setup-kit "${CMAKE_CURRENT_BINARY_DIR}/re-setup-kit")
    set(MECHWARRIOR_VERSION "dev" CACHE STRING "The program's version, as --status reports it (packages set it)")
    add_library(mechwarrior_setup STATIC src/Setup/Setup.cpp)
    target_include_directories(mechwarrior_setup PUBLIC src)
    target_link_libraries(mechwarrior_setup PUBLIC rekit mechwarrior_original)
    add_library(mechwarrior_sdl STATIC
        src/SDL/SDLBackend_Main.cpp src/SDL/SDLBackend_Input.cpp src/SDL/SDLBackend_Time.cpp
        src/SDL/SDLBackend_Config.cpp src/MT32/MPU401.cpp
        src/SDL/SDLBackend_DebugControl.cpp
        src/SDL/SDLBackend_Pilot.cpp src/Support/SupportPilot.cpp src/Support/SupportPilotObservation.cpp
        src/SDL/SDLBackend_Video.cpp src/SDL/SDLBackend_CombatPlatform.cpp
        src/SDL/SDLBackend_CombatProcess.cpp src/SDL/SDLBackend_Audio.cpp)
    target_link_libraries(mechwarrior_sdl PUBLIC mechwarrior_original SDL3::SDL3 mechwarrior_audio mechwarrior_mt32 mechwarrior_adlib
                          mechwarrior_setup)  # the player's folder's names and order (src/Setup/Setup.h)
    if(NOT MSVC)
        target_link_libraries(mechwarrior_sdl PUBLIC m)
    endif()
    target_include_directories(mechwarrior_sdl PRIVATE src/SDL)
    add_executable(mechwarrior src/SDL/SDLMain.cpp src/SDL/SDLSetup.cpp)
    target_link_libraries(mechwarrior PRIVATE mechwarrior_sdl mechwarrior_setup)
    if(TARGET rekit_sdl)
        target_link_libraries(mechwarrior PRIVATE rekit_sdl)
    endif()
    target_compile_definitions(mechwarrior PRIVATE MECHWARRIOR_VERSION="${MECHWARRIOR_VERSION}")
    if(MECHWARRIOR_BUILD_TESTS)
        # The recipe: the walk's verdicts, the install from a folder and from an archive, the carry-over, --status's
        # answer. Its game parts need the player's files (MW1O_GAME); without them only the game-free parts run.
        add_executable(setup_test tests/setup_test.cpp)
        target_link_libraries(setup_test PRIVATE mechwarrior_setup)
        add_test(NAME setup COMMAND setup_test "${CMAKE_CURRENT_BINARY_DIR}/setup_work")
        # Controllers through the real SDL host (H3b): virtual gamepads and joysticks, plugged in and taken out.
        add_executable(sdl_joystick_test tests/sdl_joystick_test.cpp)
        target_link_libraries(sdl_joystick_test PRIVATE mechwarrior_sdl)
        target_include_directories(sdl_joystick_test PRIVATE src/SDL)
        add_test(NAME sdl_joystick COMMAND sdl_joystick_test)
        set_tests_properties(sdl_joystick PROPERTIES SKIP_RETURN_CODE 77)
    endif()
    if(WIN32)
        # The icon Explorer, the taskbar and the installer's shortcuts show.
        target_sources(mechwarrior PRIVATE tools/package/mechwarrior.rc)
    endif()
    # A package's program opens no console window (tools/build_package.py
    # sets it). It still writes to redirected stdout/stderr, and a start
    # without a game folder logs to mechwarrior.log beside the saves.
    option(MECHWARRIOR_WINDOWS_GUI "Windows: link mechwarrior as a GUI program (no console)" OFF)
    if(MSVC AND MECHWARRIOR_WINDOWS_GUI)
        target_link_options(mechwarrior PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup)
    endif()
    # The music soundfont (docs/phase5/NO_EMULATION_PLAN.md, S2): the programs in data/music/soundfont_programs.txt,
    # cut from FluidR3 GM by tools/soundfont/subset.py and refused unless it hashes as data/music pins. Its source is
    # fetched once per machine (tools/deps/fetch_soundfont.sh), as libarchive's is: a build never downloads.
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    # MW1O_SOUNDFONT_SOURCE when set, else the cached choice (a reconfigure the build itself runs sees no
    # environment), else build-deps.
    set(MECHWARRIOR_SOUNDFONT_SOURCE "" CACHE FILEPATH "A copy of fluid-soundfont_3.1.orig.tar.gz (default build-deps/soundfont)")
    if(DEFINED ENV{MW1O_SOUNDFONT_SOURCE})
        set(MECHWARRIOR_SOUNDFONT_SOURCE "$ENV{MW1O_SOUNDFONT_SOURCE}" CACHE FILEPATH "" FORCE)
    elseif(NOT MECHWARRIOR_SOUNDFONT_SOURCE)
        set(MECHWARRIOR_SOUNDFONT_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/build-deps/soundfont/fluid-soundfont_3.1.orig.tar.gz"
            CACHE FILEPATH "" FORCE)
    endif()
    if(NOT EXISTS "${MECHWARRIOR_SOUNDFONT_SOURCE}")
        message(FATAL_ERROR "The music soundfont's source is missing: ${MECHWARRIOR_SOUNDFONT_SOURCE}\n"
            "Fetch it once: tools/deps/fetch_soundfont.sh (Windows: python tools\\soundfont\\subset.py fetch), "
            "or set MW1O_SOUNDFONT_SOURCE to a copy of fluid-soundfont_3.1.orig.tar.gz.")
    endif()
    set(MECHWARRIOR_SOUNDFONT "${CMAKE_CURRENT_BINARY_DIR}/soundfont/FluidR3_GM-subset.sf2")
    add_custom_command(OUTPUT "${MECHWARRIOR_SOUNDFONT}"
        COMMAND Python3::Interpreter -I "${CMAKE_CURRENT_SOURCE_DIR}/tools/soundfont/subset.py" generate
            --source "${MECHWARRIOR_SOUNDFONT_SOURCE}" --out "${MECHWARRIOR_SOUNDFONT}"
        DEPENDS tools/soundfont/subset.py data/music/soundfont_programs.txt data/music/soundfont_subset.sha256
            "${MECHWARRIOR_SOUNDFONT_SOURCE}"
        COMMENT "Cutting the music soundfont from FluidR3 GM" VERBATIM)
    add_custom_target(mechwarrior_soundfont DEPENDS "${MECHWARRIOR_SOUNDFONT}")
    # Our data/ (the rules, the soundfont) beside the program, where the game looks for it;
    # copied on every build so an edited file is picked up.
    add_custom_target(mechwarrior_data ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/data" "$<TARGET_FILE_DIR:mechwarrior>/data"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MECHWARRIOR_SOUNDFONT}" "$<TARGET_FILE_DIR:mechwarrior>/data/music")
    add_dependencies(mechwarrior_data mechwarrior_soundfont)
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
    # Play without the game's programs (docs/phase2/PLAN.md 8.5, 6f): the
    # traces on a copy of MW1O_GAME with its exes, then with every *.EXE and
    # *.COM deleted, each the same. Minutes, not seconds: skipped unless
    # MW1O_EXE_FREE=1 (and MW1O_GAME) is set; the gate runs it as its own step.
    if(MECHWARRIOR_BUILD_TESTS)
        find_package(Python3 COMPONENTS Interpreter)
        if(Python3_FOUND)
            add_test(NAME status_setup COMMAND Python3::Interpreter -I "${CMAKE_CURRENT_SOURCE_DIR}/tests/status_setup_test.py"
                --program $<TARGET_FILE:mechwarrior> --work "${CMAKE_CURRENT_BINARY_DIR}/status_setup_work")
            add_test(NAME read_only_install COMMAND Python3::Interpreter -I "${CMAKE_CURRENT_SOURCE_DIR}/tests/read_only_install_test.py"
                --port $<TARGET_FILE:mechwarrior> --work "${CMAKE_CURRENT_BINARY_DIR}/read_only_install_work")
            set_tests_properties(read_only_install PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 600)
            add_test(NAME exe_free COMMAND Python3::Interpreter -I "${CMAKE_CURRENT_SOURCE_DIR}/tools/trace/exe_free.py"
                --port $<TARGET_FILE:mechwarrior> --work "${CMAKE_CURRENT_BINARY_DIR}/exe_free_work")
            set_tests_properties(exe_free PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 3600)
            # The game-event logs (docs/phase5/NO_EMULATION_PLAN.md, T1): opt-in, MW1O_EVENTS=1.
            add_test(NAME events COMMAND Python3::Interpreter -I "${CMAKE_CURRENT_SOURCE_DIR}/tools/trace/events.py"
                check --opt-in --port $<TARGET_FILE:mechwarrior> --work "${CMAKE_CURRENT_BINARY_DIR}/events_work")
            set_tests_properties(events PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 3600)
            # song: and sound: (event-log test aids) stay out of the game checks.
            add_test(NAME feed_calls COMMAND Python3::Interpreter -I "${CMAKE_CURRENT_SOURCE_DIR}/tools/trace/test_feed_calls.py")
            set_tests_properties(feed_calls PROPERTIES ENVIRONMENT "MW1O_FEED_PORT=$<TARGET_FILE:mechwarrior>" TIMEOUT 600)
            # The snapshot schema (B1): its exchange fields are the save format's encoder's and cover every byte.
            add_test(NAME snap_schema COMMAND Python3::Interpreter -I "${CMAKE_CURRENT_SOURCE_DIR}/tools/trace/test_schema.py")
            # The soundfont subset: the tool on a bank it builds, then the real subset against the pinned source
            # (its hash, its licence texts, and through TinySoundFont the same regions and samples as the full bank).
            add_executable(soundfont_render_test tests/soundfont_render_test.cpp)
            target_include_directories(soundfont_render_test SYSTEM PRIVATE extern/TinySoundFont)
            target_compile_definitions(soundfont_render_test PRIVATE _CRT_SECURE_NO_WARNINGS)
            add_test(NAME soundfont_subset COMMAND Python3::Interpreter -I "${CMAKE_CURRENT_SOURCE_DIR}/tools/soundfont/test_subset.py"
                --renderer $<TARGET_FILE:soundfont_render_test>)
            add_test(NAME soundfont_bank COMMAND Python3::Interpreter -I "${CMAKE_CURRENT_SOURCE_DIR}/tools/soundfont/subset.py"
                check --source "${MECHWARRIOR_SOUNDFONT_SOURCE}" --renderer $<TARGET_FILE:soundfont_render_test>)
            set_tests_properties(soundfont_bank PROPERTIES TIMEOUT 600)
            # The music (src/Audio/Music, S3): the instrument table read strictly, and played on the subset.
            add_executable(music_test tests/music_test.cpp)
            target_link_libraries(music_test PRIVATE mechwarrior_audio)
            add_dependencies(music_test mechwarrior_soundfont)
            add_test(NAME music COMMAND music_test "${MECHWARRIOR_SOUNDFONT}" "${CMAKE_CURRENT_SOURCE_DIR}/data/music/instruments.json")
        endif()
        # The tone voice (src/Audio/ToneVoice, S5): our square, its place kept, no aliasing.
        add_executable(tone_voice_test tests/tone_voice_test.cpp)
        target_link_libraries(tone_voice_test PRIVATE mechwarrior_audio)
        add_test(NAME tone_voice COMMAND tone_voice_test)
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
# Every test runs as a background app on macOS (SDL reads the hint from the
# environment): a test's SDL never becomes a foreground app or takes the
# keyboard from whoever is at the machine.
if(APPLE AND MECHWARRIOR_BUILD_TESTS)
    get_property(mw1_tests DIRECTORY PROPERTY TESTS)
    if(mw1_tests)
        set_property(TEST ${mw1_tests} APPEND PROPERTY ENVIRONMENT "SDL_MAC_BACKGROUND_APP=1")
    endif()
endif()
