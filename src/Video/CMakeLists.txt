# Our own screen, with no SDL: the canvas the game draws into and the host
# presents (docs/phase5/NO_EMULATION_PLAN.md, G1-G5).
add_library(mechwarrior_video STATIC Canvas.cpp)
target_include_directories(mechwarrior_video PUBLIC .)
target_compile_features(mechwarrior_video PUBLIC cxx_std_20)
