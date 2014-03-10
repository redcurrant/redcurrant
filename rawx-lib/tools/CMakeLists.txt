include_directories(BEFORE . .. ../..)

include_directories(AFTER
		${LZO_INCLUDE_DIRS}
		${ZLIB_INCLUDE_DIRS}
		${ATTR_INCLUDE_DIRS})

set(REQLIBS
		${GLIB2_LIBRARIES} metautils rawx)

add_executable(gs_compress gs_compress.c)
target_link_libraries(gs_compress ${REQLIBS})

add_executable(gs_uncompress gs_uncompress.c)
target_link_libraries(gs_uncompress ${REQLIBS})

install(TARGETS 
		gs_compress gs_uncompress
		RUNTIME DESTINATION bin)

