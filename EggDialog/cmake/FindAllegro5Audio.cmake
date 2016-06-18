# Find Allegro 5

if(ALLEGRO5AUDIO_INCLUDE_DIR)
    # Already in cache, be silent
    set(ALLEGRO5AUDIO_FIND_QUIETLY TRUE)
endif(ALLEGRO5AUDIO_INCLUDE_DIR)

include(FindPkgConfig)

pkg_search_module(ALLEGRO5AUDIO allegro_audio-${ALLEGRO5_VERSION} allegro_audio-debug-${ALLEGRO5_VERSION})

mark_as_advanced(ALLEGRO5AUDIO_CFLAGS ALLEGRO5AUDIO_LDFLAGS ALLEGRO5AUDIO_LIBRARIES ALLEGRO5AUDIO_INCLUDE_DIRS)

