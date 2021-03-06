#ifndef __REDCURRANT__metautils_notifications__h
# define __REDCURRANT__metautils_notifications__h 1

#include <glib.h>
#include <metautils/lib/metautils.h>

/** Function type for notifier configuration or reconfiguration.
 * handle is an in/out parameter. It is opaque to the caller. */
typedef GError *(*notifier_configure)(const namespace_info_t *nsinfo,
		struct grid_lbpool_s *lbpool, GSList *topics, gint64 timeout,
		gpointer *handle);
/** Function type for sending a notification. */
typedef GError *(*notifier_send)(gpointer handle, const gchar *topic,
		const guint32 *key, GByteArray *data);
/** Function type for freeing the notifier. */
typedef void (*notifier_free)(gpointer handle);

/**
 * Simple vtable for notifier usage.
 */
struct notifier_s
{
	/** Type of the notifier. It is used as a key, so there can be
	 * only one notifier of each type per process. */
	const gchar *type;
	/** Opaque handle that can be set by the notifier. */
	gpointer handle;
	/** Configure or reconfigure the notifier. */
	notifier_configure configure;
	/** Free the notifier. */
	notifier_free free;
	/** Send a notification. */
	notifier_send send;
};


/** A pool a notifiers. */
struct metautils_notif_pool_s;
typedef struct metautils_notif_pool_s metautils_notif_pool_t;

/**
 * Allocate and initialize a notifier pool.
 */
void metautils_notif_pool_init(metautils_notif_pool_t **pool,
	const gchar *ns_name, struct grid_lbpool_s *lbpool);

/**
 * Clear a notifier pool.
 */
void metautils_notif_pool_clear(metautils_notif_pool_t **notifier);

/**
 * Configure or reconfigure a type of notifier inside the pool.
 *
 * @param pool the notifier pool
 * @param nsinfo the namespace information
 * @param type the notifier type to configure (ex: "kafka")
 * @param topics a list of topics the notifier must be prepared
 *   to send notifications to
 * @param timeout Message timeout in milliseconds (-1 for default)
 */
GError *metautils_notif_pool_configure_type(metautils_notif_pool_t *pool,
		namespace_info_t *nsinfo, const gchar *type,
		GSList *topics, gint64 timeout);

/** Remove a notifier type from the notifier pool. */
void metautils_notif_pool_clear_type(metautils_notif_pool_t *pool,
		const gchar *type);

/**
 * Send a raw notification to the specified topic.
 *
 * @param lb_key Pointer to a 32 bit integer helping for
 *   notification load balancing (can be NULL)
 */
GError *metautils_notif_pool_send_raw(metautils_notif_pool_t *pool,
	const gchar *topic, GByteArray *data, const guint32 *lb_key);

/**
 * Send a JSON notification to the specifier topic. The template requires
 * a source address and a type, and includes an autogenerated sequence number.
 *
 * @param lb_key Pointer to a 32 bit integer helping for
 *   notification load balancing (can be NULL)
 */
GError *metautils_notif_pool_send_json(metautils_notif_pool_t *pool,
	const gchar *topic, const gchar *src_addr, const char *notif_type,
	const gchar *notif_data, const guint32 *lb_key);

#endif // __REDCURRANT__metautils_notifications__h
