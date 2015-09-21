#ifndef G_LOG_DOMAIN
# define G_LOG_DOMAIN "sqlx.remote"
#endif

#include <errno.h>
#include <stdlib.h>

#include <glib.h>

#include <metautils/lib/metautils.h>
#include <metautils/lib/metacomm.h>
#include <sqliterepo/sqlite_utils.h>
#include <sqliterepo/sqlx_remote.h>
#include <sqliterepo/sqlx_remote_ex.h>

static gboolean on_reply_gba(gpointer out, MESSAGE reply)
{
	void *b = NULL;
	gsize bsize = 0;
	GByteArray *out_gba = (GByteArray *) out;

	if (0 < message_get_BODY(reply, &b, &bsize, NULL)) {
		if (!out_gba)
			return TRUE;
		g_byte_array_append(out_gba, b, bsize);
	}
	return TRUE;
}

GError*
sqlx_remote_execute_DESTROY(const gchar *target, GByteArray *sid,
		struct sqlxsrv_name_s *name, gboolean local)
{
	(void) sid;
	GError *err = NULL;
	GByteArray *req = sqlx_pack_DESTROY(name, local);

	struct client_s *client = gridd_client_create(target, req, NULL, NULL);
	g_byte_array_unref(req);

	gridd_client_start(client);
	if (!(err = gridd_client_loop(client))) {
		err = gridd_client_error(client);
	}

	gridd_client_free(client);
	return err;
}

GError*
sqlx_remote_execute_packed_DESTROY_many(gchar **targets, GByteArray *sid,
		GByteArray *req)
{
	(void) sid;
	GError *err = NULL, *local_err = NULL;
	GPtrArray *targets_ok = g_ptr_array_new();

	struct client_s **clients = gridd_client_create_many(targets, req,
			NULL, NULL);
	metautils_gba_unref(req);
	req = NULL;

	if (clients == NULL) {
		err = NEWERROR(0, "Failed to create gridd clients");
		return err;
	}

	gridd_clients_start(clients);
	err = gridd_clients_loop(clients);

	for (gchar **cursor = targets; cursor && *cursor; cursor++) {
		metautils_str_clean(cursor);
	}

	if (err)
		goto end;

	for (struct client_s **p = clients; p && *p; p++) {
		if (!(local_err = gridd_client_error(*p))) {
			g_ptr_array_add(targets_ok, g_strdup(gridd_client_url(*p)));
			continue;
		}
		if (local_err->code == CODE_CONTAINER_NOTFOUND ||
					local_err->code == CODE_NOT_FOUND) {
			g_clear_error(&local_err);
			g_ptr_array_add(targets_ok, g_strdup(gridd_client_url(*p)));
			continue;
		}
		GRID_DEBUG("Database destruction attempts failed: (%d) %s",
				local_err->code, local_err->message);
		if (!err)
			g_propagate_prefixed_error(&err, local_err,
					"Failed to destroy base on %s: ",
					gridd_client_url(*p));
		else
			g_clear_error(&local_err);
	}

	for (guint i = 0; i < targets_ok->len; i++) {
		targets[i] = g_ptr_array_index(targets_ok, i);
	}

end:
	g_ptr_array_free(targets_ok, TRUE);
	gridd_clients_free(clients);
	return err;
}

GError*
sqlx_remote_execute_DESTROY_many(gchar **targets, GByteArray *sid,
		struct sqlxsrv_name_s *name)
{
	GByteArray *req = sqlx_pack_DESTROY(name, TRUE);
	return sqlx_remote_execute_packed_DESTROY_many(targets, sid, req);
}

GError*
sqlx_remote_execute_RESTORE_many(gchar **targets, GByteArray *sid,
		struct sqlx_name_s *name, GByteArray *dump)
{
	(void) sid;
	GError *err = NULL;
	GByteArray *req = sqlx_pack_RESTORE(name, dump->data, dump->len);
	struct client_s **clients = gridd_client_create_many(targets, req,
			NULL, NULL);
	metautils_gba_unref(req);
	req = NULL;

	if (clients == NULL) {
		err = NEWERROR(0, "Failed to create gridd clients");
		return err;
	}

	gridd_clients_start(clients);
	err = gridd_clients_loop(clients);

	for (struct client_s **p = clients; !err && p && *p; p++) {
		err = gridd_client_error(*p);
	}

	gridd_clients_free(clients);
	return err;
}

GError*
sqlx_remote_execute_ADMGET(const gchar *target, GByteArray *sid,
		struct sqlx_name_s *name, const gchar *k, gchar **v)
{
	(void) sid;
	GError *err = NULL;
	GByteArray *encoded = sqlx_pack_ADMGET(name, k);
	GByteArray *gba_buf = g_byte_array_new();
	struct client_s *client = gridd_client_create(target, encoded,
			gba_buf, on_reply_gba);
	g_byte_array_unref(encoded);

	gridd_client_start(client);
	if (!(err = gridd_client_loop(client))) {
		if (!(err = gridd_client_error(client))) {
			gchar *buf = g_malloc0(gba_buf->len + 1);
			metautils_gba_data_to_string(gba_buf, buf, gba_buf->len + 1);
			*v = buf;
		}
	}

	gridd_client_free(client);
	metautils_gba_unref(gba_buf);
	return err;
}

GError*
sqlx_remote_execute_ADMSET(const gchar *target, GByteArray *sid,
		struct sqlx_name_s *name, const gchar *k, const gchar *v)
{
	(void) sid;
	GError *err = NULL;
	GByteArray *encoded = sqlx_pack_ADMSET(name, k, v);
	struct client_s *client = gridd_client_create(target, encoded, NULL, NULL);
	g_byte_array_unref(encoded);
	gridd_client_start(client);
	if (!(err = gridd_client_loop(client)))
		err = gridd_client_error(client);
	gridd_client_free(client);
	return err;
}

GError*
sqlx_get_admin_status(const gchar *target, struct sqlx_name_s *name,
		guint32 *status)
{
	GError *err = NULL;
	gchar *str_status = NULL;
	err = sqlx_remote_execute_ADMGET(target, NULL, name,
			ADMIN_STATUS_KEY, &str_status);
	if (err == NULL) {
		gchar *tmp = NULL;
		errno = 0;
		guint32 res = strtoul(str_status, &tmp, 10);
		if (tmp == str_status) {
			err = NEWERROR(0, "Failed to parse '%s': %s",
					str_status, g_strerror(errno));
		} else {
			*status = res;
		}
	}
	g_free(str_status);
	return err;
}

GError*
sqlx_set_admin_status(const gchar *target, struct sqlx_name_s *name,
		guint32 status)
{
	GError *err = NULL;
	gchar str_status[16];
	g_snprintf(str_status, 16, "%u", status);

	err = sqlx_remote_execute_ADMSET(target, NULL, name,
			ADMIN_STATUS_KEY, str_status);

	return err;
}

