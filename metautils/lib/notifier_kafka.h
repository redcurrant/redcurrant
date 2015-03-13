#ifndef __REDCURRANT__metautils_notifier_kafka__h
# define __REDCURRANT__metautils_notifier_kafka__h 1

#include <glib.h>
#include <metautils/lib/metautils.h>

struct kafka_handle_s;

GError *kafka_configure(const namespace_info_t *nsinfo,
		struct grid_lbpool_s *lb_pool, GSList *topics,
		struct kafka_handle_s **handle);

GError *kafka_send(struct kafka_handle_s *handle, const gchar *topic_name,
		const guint32 *lb_key, GByteArray *data);

void kafka_free(struct kafka_handle_s *handle);

#endif
