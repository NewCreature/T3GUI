# Find Allegro 5

if(ALLEGRO5_INCLUDE_DIR)
    # Already in cache, be silent
    set(ALLEGRO5_FIND_QUIETLY TRUE)
endif(ALLEGRO5_INCLUDE_DIR)

include(FindPkgConfig)

pkg_search_module(ALLEGRO5 allegro-${ALLEGRO5_VERSION} allegro-debug-${ALLEGRO5_VERSION})

#message(STATUS "include:    ${ALLEGRO5_INCLUDE_DIRS}")
#message(STATUS "link flags: ${ALLEGRO5_LDFLAGS}")
#message(STATUS "cflags:     ${ALLEGRO5_CFLAGS}")
#message(STATUS "lib dir:    ${ALLEGRO5_LIBRARY_DIRS}")

mark_as_advanced(ALLEGRO5_CFLAGS ALLEGRO5_LDFLAGS ALLEGRO5_LIBRARIES ALLEGRO5_INCLUDE_DIRS)

