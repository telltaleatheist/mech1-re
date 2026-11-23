# Our own synthesis, with no output device and no SDL: the speaker voice, the
# music (TinySoundFont, docs/phase5/NO_EMULATION_PLAN.md S3) and the mixer on
# PIT time (docs/phase5/AUDIO_PLAN.md). The card synths it calls are still the
# adapters in src/Adlib and src/MT32 until slice S9.
add_library(mechwarrior_audio STATIC Mixer.cpp SpeakerVoice.cpp Music.cpp)
target_include_directories(mechwarrior_audio PUBLIC . ../include)
target_include_directories(mechwarrior_audio SYSTEM PRIVATE ../../extern/TinySoundFont)
target_compile_features(mechwarrior_audio PUBLIC cxx_std_20)
target_link_libraries(mechwarrior_audio PUBLIC mechwarrior_mt32 mechwarrior_adlib mechwarrior_json)
