#ifndef G_LOG_DOMAIN
# define G_LOG_DOMAIN "sqlx.sync"
#endif

#include <metautils/lib/metautils.h>
#include <metautils/lib/metacomm.h>

#include <zookeeper.h>
#include <zookeeper_log.h>
#include <errno.h>

#include "synchro.h"

struct sqlx_sync_s
{
	struct sqlx_sync_vtable_s *vtable;

	void (*on_exit) (void *ctx);
	void *on_exit_ctx;

	gchar *zk_prefix;
	gchar *zk_url;
	zhandle_t *zh;
	GMutex *zh_lock;
	clientid_t zk_id;

	guint hash_width;
	guint hash_depth;
};

static void _clear(struct sqlx_sync_s *ss);

static GError* _open(struct sqlx_sync_s *ss);

static void _close(struct sqlx_sync_s *ss);

static int _acreate (struct sqlx_sync_s *ss, const char *path, const char *v,
        int vlen, int flags, string_completion_t completion, const void *data);

static int _adelete (struct sqlx_sync_s *ss, const char *path, int version,
        void_completion_t completion, const void *data);

static int _awexists (struct sqlx_sync_s *ss, const char *path,
		watcher_fn watcher, void* watcherCtx,
		stat_completion_t completion, const void *data);

static int _awget (struct sqlx_sync_s *ss, const char *path,
        watcher_fn watcher, void* watcherCtx,
        data_completion_t completion, const void *data);

static int _awget_children (struct sqlx_sync_s *ss, const char *path,
        watcher_fn watcher, void* watcherCtx,
        strings_completion_t completion, const void *data);

static int _awget_siblings (struct sqlx_sync_s *ss, const char *path,
        watcher_fn watcher, void* watcherCtx,
        strings_completion_t completion, const void *data);

static void _set_exit_hook(struct sqlx_sync_s *ss, void (*on_exit_hook) (void*),
		void *on_exit_ctx);

static struct sqlx_sync_vtable_s VTABLE =
{
	_clear,
	_open,
	_close,
	_acreate,
	_adelete,
	_awexists,
	_awget,
	_awget_children,
	_awget_siblings,
	_set_exit_hook
};

struct sqlx_sync_s*
sqlx_sync_create(const char *url)
{
	struct sqlx_sync_s *ss = g_malloc0(sizeof(struct sqlx_sync_s));
	ss->vtable = &VTABLE;
	ss->zk_url = g_strdup(url);
	ss->zk_prefix = g_strdup("/NOTSET");
	ss->zh_lock = g_mutex_new();
	return ss;
}

gboolean
sqlx_sync_is_connected(struct sqlx_sync_s *ss)
{
	return ss->zh != NULL && ss->zk_id.client_id != 0;
}

void
sqlx_sync_set_prefix(struct sqlx_sync_s *ss, const gchar *prefix)
{
	EXTRA_ASSERT(ss != NULL);
	EXTRA_ASSERT(ss->vtable == &VTABLE);
	metautils_str_replace(&ss->zk_prefix, prefix);
	GRID_NOTICE("SYNC prefix set to [%s]", prefix);
}

void
sqlx_sync_set_hash(struct sqlx_sync_s *ss, guint w, guint d)
{
	EXTRA_ASSERT(ss != NULL);
	EXTRA_ASSERT(ss->vtable == &VTABLE);
	ss->hash_width = CLAMP(w,1,3);
	ss->hash_depth = MIN(d,2);
	GRID_NOTICE("SYNC hash set to [%ux%u]", w, d);
}

static void
_set_exit_hook(struct sqlx_sync_s *ss, void (*on_exit_hook) (void*),
		void *on_exit_ctx)
{
	EXTRA_ASSERT(ss != NULL);
	EXTRA_ASSERT(ss->vtable == &VTABLE);
	ss->on_exit = on_exit_hook;
	ss->on_exit_ctx = on_exit_ctx;
}

//------------------------------------------------------------------------------

static gchar *
_realpath(const struct sqlx_sync_s *ss, const gchar *path)
{
	guint w;
	switch (ss->hash_depth) {
		case 0:
			return g_strdup_printf("%s/%s", ss->zk_prefix, path);
		case 1:
			w = CLAMP(ss->hash_width, 1, 3);
			return g_strdup_printf("%s/%.*s/%s", ss->zk_prefix, w, path, path);
		default:
			w = CLAMP(ss->hash_width, 1, 2);
			return g_strdup_printf("%s/%.*s/%.*s/%s", ss->zk_prefix,
					w, path, w, path + w, path);
	}
}

static gchar *
_realdirname(const struct sqlx_sync_s *ss, const gchar *path)
{
	guint w;
	switch (ss->hash_depth) {
		case 0:
			return g_strdup(ss->zk_prefix);
		case 1:
			w = CLAMP(ss->hash_width, 1, 3);
			return g_strdup_printf("%s/%.*s", ss->zk_prefix, w, path);
		default:
			w = CLAMP(ss->hash_width, 1, 2);
			return g_strdup_printf("%s/%.*s/%.*s", ss->zk_prefix,
					w, path, w, path + w);
	}
}

//------------------------------------------------------------------------------

static void
zk_main_watch(zhandle_t *zh, int type, int state, const char *path,
		void *watcherCtx);

static const gchar*
_zk_type_to_str(int type)
{
	if (type == ZOO_CREATED_EVENT)
		return "created";
	if (type == ZOO_DELETED_EVENT)
		return "deleted";
	if (type == ZOO_CHANGED_EVENT)
		return "changed";
	if (type == ZOO_CHILD_EVENT)
		return "child";
	if (type == ZOO_SESSION_EVENT)
		return "session";
	if (type == ZOO_NOTWATCHING_EVENT)
		return "notwatching";
	return "unknown";
}

static void
_zk_reconnect(struct sqlx_sync_s *ss, gboolean reuse_session)
{
	clientid_t session_id;

	GRID_WARN("Zookeeper: (re)connecting to [%s]", ss->zk_url);

	if (reuse_session) {
		memcpy(&session_id, &(ss->zk_id), sizeof(session_id));
	} else {
		if (NULL != ss->on_exit)
			ss->on_exit(ss->on_exit_ctx);
		memset(&session_id, 0, sizeof(session_id));
	}

	memset(&(ss->zk_id), 0, sizeof(ss->zk_id));

	g_mutex_lock(ss->zh_lock);
	if (ss->zh) {
		zookeeper_close(ss->zh);
		ss->zh = NULL;
	}
	errno = 0;
	ss->zh = zookeeper_init(ss->zk_url, zk_main_watch,
			SQLX_SYNC_DEFAULT_ZK_TIMEOUT, &session_id, ss, 0);
	if (ss->zh == NULL)
		GRID_WARN("Failed to init Zookeeper session: %s", strerror(errno));
	g_mutex_unlock(ss->zh_lock);
}

static void
zk_main_watch(zhandle_t *zh, int type, int state, const char *path,
		void *watcherCtx)
{
	struct sqlx_sync_s *ss = watcherCtx;
	(void) zh;
	(void) path;

	if (type == ZOO_SESSION_EVENT) {
		if (state == ZOO_CONNECTING_STATE) {
			_zk_reconnect(ss, TRUE);
		}
		else if (state == ZOO_EXPIRED_SESSION_STATE) {
			GRID_WARN("Zookeeper: expired session to [%s]", ss->zk_url);
			_zk_reconnect(ss, FALSE);
		}
		else if (state == ZOO_AUTH_FAILED_STATE) {
			GRID_WARN("Zookeeper: auth problem to [%s]", ss->zk_url);
		}
		else if (state == ZOO_ASSOCIATING_STATE) {
			GRID_DEBUG("Zookeeper: associating to [%s]", ss->zk_url);
		}
		else if (state == ZOO_CONNECTED_STATE) {
			g_mutex_lock(ss->zh_lock);
			memcpy(&(ss->zk_id), zoo_client_id(ss->zh), sizeof(clientid_t));
			g_mutex_unlock(ss->zh_lock);
			GRID_INFO("Zookeeper: connected to [%s] id=%"G_GINT64_FORMAT,
					ss->zk_url, ss->zk_id.client_id);
		}
		else {
			GRID_WARN("Unknown state received from Zookeeper: %i", state);
		}
	} else {
		GRID_WARN("Got event from Zookeeper: %s", _zk_type_to_str(type));
	}
}

static GError*
_open(struct sqlx_sync_s *ss)
{
	EXTRA_ASSERT(ss != NULL);
	EXTRA_ASSERT(ss->vtable == &VTABLE);
	if (NULL != ss->zh)
		return NEWERROR(500, "BUG : ZK connection already initiated");
	g_mutex_lock(ss->zh_lock);
	ss->zh = zookeeper_init(ss->zk_url, zk_main_watch, 4000, NULL, ss, 0);
	g_mutex_unlock(ss->zh_lock);
	if (NULL == ss->zh)
		return NEWERROR(500, "ZK connection failure");
	return NULL;
}

static void
_close(struct sqlx_sync_s *ss)
{
	EXTRA_ASSERT(ss != NULL);
	EXTRA_ASSERT(ss->vtable == &VTABLE);
	if (ss->zh) {
		g_mutex_lock(ss->zh_lock);
		zookeeper_close(ss->zh);
		ss->zh = NULL;
		g_mutex_unlock(ss->zh_lock);
	}
}

static void
_clear(struct sqlx_sync_s *ss)
{
	EXTRA_ASSERT(ss != NULL);
	EXTRA_ASSERT(ss->vtable == &VTABLE);
	_close(ss);
	g_free(ss->zk_prefix);
	g_free(ss->zk_url);
	g_mutex_free(ss->zh_lock);
	memset(ss, 0, sizeof(*ss));
	g_free(ss);
}

static int
_acreate (struct sqlx_sync_s *ss, const char *path, const char *v,
        int vlen, int flags, string_completion_t completion, const void *data)
{
	EXTRA_ASSERT(ss != NULL);
	EXTRA_ASSERT(ss->vtable == &VTABLE);
	gchar *p = _realpath(ss, path);
	g_mutex_lock(ss->zh_lock);
	int rc = zoo_acreate(ss->zh, p, v, vlen, &ZOO_OPEN_ACL_UNSAFE,
			flags, completion, data);
	g_mutex_unlock(ss->zh_lock);
	GRID_TRACE2("SYNC create(%p) = %d", p, rc);
	g_free(p);
	return rc;
}

static int
_adelete (struct sqlx_sync_s *ss, const char *path, int version,
        void_completion_t completion, const void *data)
{
	EXTRA_ASSERT(ss != NULL);
	EXTRA_ASSERT(ss->vtable == &VTABLE);
	gchar *p = _realpath(ss, path);
	g_mutex_lock(ss->zh_lock);
	int rc = zoo_adelete(ss->zh, p, version, completion, data);
	g_mutex_unlock(ss->zh_lock);
	GRID_TRACE2("SYNC delete(%s) = %d", p, rc);
	g_free(p);
	return rc;
}

static int
_awexists (struct sqlx_sync_s *ss, const char *path,
		watcher_fn watcher, void* watcherCtx,
		stat_completion_t completion, const void *data)
{
	EXTRA_ASSERT(ss != NULL);
	EXTRA_ASSERT(ss->vtable == &VTABLE);
	gchar *p = _realpath(ss, path);
	g_mutex_lock(ss->zh_lock);
	int rc = zoo_awexists(ss->zh, p, watcher, watcherCtx, completion, data);
	g_mutex_unlock(ss->zh_lock);
	GRID_TRACE2("SYNC exists(%s) = %d", p, rc);
	g_free(p);
	return rc;
}

static int
_awget (struct sqlx_sync_s *ss, const char *path,
        watcher_fn watcher, void* watcherCtx,
        data_completion_t completion, const void *data)
{
	EXTRA_ASSERT(ss != NULL);
	EXTRA_ASSERT(ss->vtable == &VTABLE);
	gchar *p = _realpath(ss, path);
	g_mutex_lock(ss->zh_lock);
	int rc = zoo_awget(ss->zh, p, watcher, watcherCtx, completion, data);
	g_mutex_unlock(ss->zh_lock);
	GRID_TRACE2("SYNC get(%s) = %d", p, rc);
	g_free(p);
	return rc;
}

static int
_awget_children (struct sqlx_sync_s *ss, const char *path,
        watcher_fn watcher, void* watcherCtx,
        strings_completion_t completion, const void *data)
{
	EXTRA_ASSERT(ss != NULL);
	EXTRA_ASSERT(ss->vtable == &VTABLE);
	gchar *p = _realpath(ss, path);
	g_mutex_lock(ss->zh_lock);
	int rc = zoo_awget_children(ss->zh, p, watcher, watcherCtx, completion,
			data);
	g_mutex_unlock(ss->zh_lock);
	GRID_TRACE2("SYNC children(%s) = %d", p, rc);
	g_free(p);
	return rc;
}

static int
_awget_siblings (struct sqlx_sync_s *ss, const char *path,
        watcher_fn watcher, void* watcherCtx,
        strings_completion_t completion, const void *data)
{
	EXTRA_ASSERT(ss != NULL);
	EXTRA_ASSERT(ss->vtable == &VTABLE);
	gchar *p = _realdirname(ss, path);
	g_mutex_lock(ss->zh_lock);
	int rc = zoo_awget_children(ss->zh, p, watcher, watcherCtx, completion, data);
	g_mutex_unlock(ss->zh_lock);
	GRID_TRACE("SYNC siblings(%s) = %d", p, rc);
	g_free(p);
	return rc;
}

