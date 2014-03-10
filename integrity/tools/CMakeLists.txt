include_directories(BEFORE . .. ../..
		../../client/c/lib
		${CMAKE_CURRENT_BINARY_DIR}/../..)

include_directories(AFTER
		${PYTHON_INCLUDE_PATH}
		${DB_INCLUDE_DIRS}
		${LZO_INCLUDE_DIRS}
		${ZLIB_INCLUDE_DIRS}
		${ATTR_INCLUDE_DIRS})

link_directories(
		${LZO_LIBRARY_DIRS}
		${ZLIB_LIBRARY_DIRS}
		${ATTR_LIBRARY_DIRS})

add_executable(gs_rebuild gs_rebuild.c repair.c repair.h)
target_link_libraries(gs_rebuild
		metautils rawx rawxclient meta2remote gridclient gridcluster
		gridcluster-events gridcluster-remote integrity
		${GLIB2_LIBRARIES})

add_executable(gs_rawx_list gs_rawx_list.c)
target_link_libraries(gs_rawx_list
		metautils integrity gridcluster gridcluster-events
		gridcluster-remote
		${GLIB2_LIBRARIES})

#add_executable(hc_policycheck hc_policycheck.c)
#target_link_libraries(hc_policycheck
#		metautils integrity gridcluster gridcluster-events
#		gridcluster-remote
#		${GLIB2_LIBRARIES})

add_executable(hc_policycheck
		hc_policycheck_v2.c
		policycheck_repair.c
		policycheck_repair.h)
target_link_libraries(hc_policycheck
		metautils integrity gridcluster gridcluster-events
		meta2v2lbutils
		gridcluster-remote hcresolve gridclient
		${GLIB2_LIBRARIES})

add_executable(gs_break gs_break.c)
target_link_libraries(gs_break
		metautils meta2remote gridclient gridcluster
		gridcluster-events gridcluster-remote
		${GLIB2_LIBRARIES})

add_executable(gs_path2container gs_path2container.c)
target_link_libraries(gs_path2container
		metautils gridcluster gridcluster-events gridcluster-remote
		${GLIB2_LIBRARIES})

add_executable(gs_chunk_crawler gs_chunk_crawler.c lock.c lock.h volume_scanner.c volume_scanner.h)
target_link_libraries(gs_chunk_crawler
		metautils rawx rawxclient meta2remote gridclient gridcluster
		integrity gridcluster gridcluster-events gridcluster-remote
		rulesmotorc2py rulesmotorpy2c
		${GLIB2_LIBRARIES})

add_executable(gs_meta2_crawler gs_meta2_crawler.c lock.c lock.h volume_scanner.c volume_scanner.h)
target_link_libraries(gs_meta2_crawler
		integrity metautils meta2remote rulesmotorc2py
		-llog4c ${GLIB2_LIBRARIES})

install(TARGETS
		gs_rebuild gs_rawx_list gs_break gs_path2container gs_chunk_crawler hc_policycheck
		gs_meta2_crawler
		RUNTIME DESTINATION bin)

