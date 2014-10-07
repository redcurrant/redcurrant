#ifndef __EVENT_CONFIG_MACROS_H
# define __EVENT_CONFIG_MACROS_H 1


// TODO: make that more readable
#define EVENT_CONFIG_UGLY_MACRO(EVT_CONF_REPO,SVC_TYPE,DEF_TOPIC) \
static GError *_init_events(metautils_notifier_t *notifier, const gchar *topic)\
{\
	GError *err = NULL;\
	err = metautils_notifier_init_kafka(notifier);\
	if (!err) {\
		err = metautils_notifier_prepare_kafka_topic(notifier, topic);\
	}\
	return err;\
}\
\
static void _task_reload_event_config(gpointer p)\
{\
	GError *err = NULL;\
	gboolean must_clear_events = TRUE;\
	metautils_notifier_t *notifier = event_config_repo_get_notifier(EVT_CONF_REPO);\
\
	void _update_each(gpointer k, gpointer v, gpointer ignored) {\
		(void) ignored;\
		if (!err) {\
			struct event_config_s *conf = event_config_repo_get(EVT_CONF_REPO, (char *)k, FALSE);\
			err = event_config_reconfigure(conf, (char *)v);\
			if (!err && event_is_notifier_enabled(conf)) {\
				must_clear_events = FALSE;\
				err = _init_events(notifier, event_get_notifier_topic_name(conf, DEF_TOPIC));\
				if (err) {\
					GRID_WARN("Failed to initialize notifications (will retry soon): %s",\
							err->message);\
					g_clear_error(&err);\
				}\
			}\
		}\
	}\
\
	GHashTable *ht = gridcluster_get_event_config(&(PSRV(p)->nsinfo),\
			SVC_TYPE);\
	if (!ht)\
		err = NEWERROR(EINVAL, "Invalid parameter");\
	else {\
		g_hash_table_foreach(ht, _update_each, NULL);\
		g_hash_table_destroy(ht);\
	}\
\
	if (!err)\
		GRID_TRACE("Event config reloaded");\
	else {\
		GRID_WARN("Event config reload error [%s]: (%d) %s",\
				PSRV(p)->ns_name, err->code, err->message);\
		g_clear_error(&err);\
	}\
\
	if (must_clear_events)\
		metautils_notifier_free_kafka(notifier);\
}

#endif // __EVENT_CONFIG_MACROS_H
