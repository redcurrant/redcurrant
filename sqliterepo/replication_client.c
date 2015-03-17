/*
 * Copyright (C) 2015 Worldline
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef G_LOG_DOMAIN
# define G_LOG_DOMAIN "sqliterepo"
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <metautils/lib/metautils.h>
#include <metautils/lib/metacomm.h>

#include "sqliterepo.h"
#include "sqlx_remote.h"
#include "internals.h"

struct TableSequence;

static GByteArray*
_pack_RESTORE(struct sqlx_name_s *name, GByteArray *dump)
{
	GByteArray *encoded = sqlx_pack_RESTORE(name, dump->data, dump->len);
	g_byte_array_unref(dump);
	return encoded;
}

GError *
peer_restore(const gchar *target, struct sqlx_name_s *name,
		GByteArray *dump)
{
	GError *err = NULL;

	if (!target) {
		g_byte_array_unref(dump);
		return NULL;
	}

	GByteArray *encoded = _pack_RESTORE(name, dump);
	struct client_s *client = gridd_client_create(target, encoded, NULL, NULL);
	g_byte_array_unref(encoded);

	if (!client) {
		return NEWERROR(500, "Failed to create client to [%s], bad address?",
				target);
	}

	gridd_client_set_timeout(client, 5.0, 30.0);
	gridd_client_start(client);
	if (!(err = gridd_client_loop(client)))
		err = gridd_client_error(client);
	gridd_client_free(client);
	return err;
}

void
peers_restore(gchar **targets, struct sqlx_name_s *name,
		GByteArray *dump)
{
	GError *err = NULL;

	if (!targets || !targets[0]) {
		g_byte_array_unref(dump);
		return ;
	}

	GByteArray *encoded = _pack_RESTORE(name, dump);
	struct client_s **clients = gridd_client_create_many(targets, encoded, NULL, NULL);
	g_byte_array_unref(encoded);
	gridd_clients_set_timeout(clients, 5.0, 30.0);

	gridd_clients_start(clients);
	if (!(err = gridd_clients_loop(clients)))
		err = gridd_clients_error(clients);
	gridd_clients_free(clients);

	if (err) {
		GRID_WARN("RESTORE failed [%s][%s] : (%d) %s", name->base, name->type,
				err->code, err->message);
		g_clear_error(&err);
	}
}

GError *
peer_dump(const gchar *target, struct sqlx_name_s *name, GByteArray **result)
{
	GByteArray *dump = NULL;
	struct client_s *client;
	GByteArray *encoded;
	GError *err = NULL;

	gboolean on_reply(gpointer ctx, MESSAGE reply) {
		void *b = NULL;
		gsize bsize = 0;
		(void) ctx;

		if (0 < message_get_BODY(reply, &b, &bsize, NULL)) {
			if (!dump)
				dump = g_byte_array_new();
			g_byte_array_append(dump, b, bsize);
		}
		return TRUE;
	}

	GRID_TRACE2("%s(%s,%p,%p)", __FUNCTION__, target, name, result);

	if (!target)
		return NEWERROR(500, "No target URL");

	encoded = sqlx_pack_DUMP(name);
	client = gridd_client_create(target, encoded, NULL, on_reply);
	g_byte_array_unref(encoded);

	if (!client) {
		return NEWERROR(500, "Failed to create client to [%s], bad address?",
				target);
	}

	// set a long timeout to allow moving large meta2 bases
	gridd_client_set_timeout(client, 3600.0, 4000.0);
	gridd_client_start(client);
	if (!(err = gridd_client_loop(client))) {
		if (!(err = gridd_client_error(client)))
			*result = dump;
	}

	if (err && dump)
		g_byte_array_free(dump, TRUE);

	gridd_client_free(client);
	return err;
}

