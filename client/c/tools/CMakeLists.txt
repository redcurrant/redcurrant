include_directories(BEFORE .
		../../..
		${CMAKE_CURRENT_BINARY_DIR}/../../..
		../../../metautils/lib)

set(REQLIBS
		gridclient metautils metacomm
		gridcluster gridcluster-remote gridcluster-events
		meta2remote meta1remote meta0remote
		${GLIB2_LIBRARIES})

set(SOLRLIBS solrutils)

add_executable(${BINPREFIX}add_event gs_add_event.c)
target_link_libraries(${BINPREFIX}add_event ${REQLIBS})

add_executable(${BINPREFIX}append gs_append.c gs_tools.c)
target_link_libraries(${BINPREFIX}append ${REQLIBS})

add_executable(${BINPREFIX}create gs_create.c gs_tools.c)
target_link_libraries(${BINPREFIX}create ${REQLIBS})

add_executable(${BINPREFIX}destroy gs_destroy.c gs_tools.c)
target_link_libraries(${BINPREFIX}destroy ${REQLIBS})

add_executable(${BINPREFIX}get gs_get.c gs_tools.c)
target_link_libraries(${BINPREFIX}get ${REQLIBS})

add_executable(${BINPREFIX}get_all gs_get_all.c)
target_link_libraries(${BINPREFIX}get_all ${REQLIBS})

add_executable(${BINPREFIX}index gs_index.c)
target_link_libraries(${BINPREFIX}index ${REQLIBS})

add_executable(${BINPREFIX}list_events gs_list_events.c)
target_link_libraries(${BINPREFIX}list_events ${REQLIBS})

add_executable(${BINPREFIX}list_prop gs_list_prop.c gs_tools.c)
target_link_libraries(${BINPREFIX}list_prop ${REQLIBS})

add_executable(${BINPREFIX}locate gs_locate.c gs_tools.c)
target_link_libraries(${BINPREFIX}locate ${REQLIBS})

add_executable(${BINPREFIX}ls gs_ls.c gs_tools.c)
target_link_libraries(${BINPREFIX}ls ${REQLIBS})

add_executable(${BINPREFIX}put gs_put.c gs_tools.c)
target_link_libraries(${BINPREFIX}put ${REQLIBS})

add_executable(${BINPREFIX}rm gs_rm.c gs_tools.c)
target_link_libraries(${BINPREFIX}rm ${REQLIBS})

add_executable(${BINPREFIX}rm_prop gs_rm_prop.c gs_tools.c) 
target_link_libraries(${BINPREFIX}rm_prop ${REQLIBS})

add_executable(${BINPREFIX}set_prop gs_set_prop.c gs_tools.c)
target_link_libraries(${BINPREFIX}set_prop ${REQLIBS})

add_executable(${BINPREFIX}touch gs_touch.c)
target_link_libraries(${BINPREFIX}touch ${REQLIBS})

add_executable(${BINPREFIX}unref_solr gs_unref_solr.c)
target_link_libraries(${BINPREFIX}unref_solr ${REQLIBS} ${SOLRLIBS})

add_executable(${BINPREFIX}set_solr gs_set_solr.c)
target_link_libraries(${BINPREFIX}set_solr ${REQLIBS} ${SOLRLIBS})

add_executable(${BINPREFIX}rm_event gs_rm_event.c)
target_link_libraries(${BINPREFIX}rm_event ${REQLIBS})

add_executable(hcdir hcdir.c)
target_link_libraries(hcdir ${REQLIBS})


add_executable(hc hc.c)
target_link_libraries(hc ${REQLIBS})

add_executable(${BINPREFIX}quota gs_quota.c)
target_link_libraries(${BINPREFIX}quota ${REQLIBS})

add_executable(hcadmin hcadmin.c)
target_link_libraries(hcadmin ${REQLIBS})

add_executable(test_upload test_upload.c)
target_link_libraries(test_upload ${REQLIBS})

#install(TARGETS
#			${BINPREFIX}touch
#			${BINPREFIX}set_prop
#			${BINPREFIX}rm_prop
#			${BINPREFIX}rm
#			${BINPREFIX}put
#			${BINPREFIX}ls
#			${BINPREFIX}locate
#			${BINPREFIX}list_prop
#			${BINPREFIX}list_events
#			${BINPREFIX}rm_event
#			${BINPREFIX}index
#			${BINPREFIX}get_all gs_get
#			${BINPREFIX}destroy
#			${BINPREFIX}create
#			${BINPREFIX}append
#			${BINPREFIX}add_event
#			${BINPREFIX}unref_solr
#			${BINPREFIX}set_solr
#			${BINPREFIX}quota
#		RUNTIME DESTINATION bin)

install(TARGETS hcdir hc hcadmin
		RUNTIME DESTINATION bin)
