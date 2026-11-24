# Our own synthesis, with no output device and no SDL: the tone voice, the
# music (TinySoundFont, docs/phase5/NO_EMULATION_PLAN.md S3) and the mixer on
# PIT time (docs/phase5/AUDIO_PLAN.md).
add_library(mechwarrior_audio STATIC Mixer.cpp ToneVoice.cpp Music.cpp)
target_include_directories(mechwarrior_audio PUBLIC . ../include)
target_include_directories(mechwarrior_audio SYSTEM PRIVATE ../../extern/TinySoundFont)
target_compile_features(mechwarrior_audio PUBLIC cxx_std_20)
target_link_libraries(mechwarrior_audio PUBLIC mechwarrior_json)
