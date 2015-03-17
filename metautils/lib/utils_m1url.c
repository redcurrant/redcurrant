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
# define G_LOG_DOMAIN "metautils.tools"
#endif

#include <json.h>

#include "metautils.h"
#include "metautils_internals.h"


struct meta1_service_url_s*
meta1_unpack_url(const gchar *url)
{
	gchar *type = NULL, *host = NULL, *args = NULL;

	EXTRA_ASSERT(url != NULL);

	int len = strlen(url);
	gchar *tmp = g_alloca(len+1);
	g_strlcpy(tmp, url, len+1);

	if (!(type = strchr(tmp, '|')))
		return NULL;
	*(type++) = '\0';

	if (!(host = strchr(type, '|')))
		return NULL;
	*(host++) = '\0';

	if (!(args = strchr(host, '|')))
		return NULL;
	*(args++) = '\0';
	if (strlen(args) >= LIMIT_LENGTH_SRVARGS)
		return NULL;

	struct meta1_service_url_s *result;
	result = g_malloc0(sizeof(*result) + strlen(args) + 1);
	result->seq = g_ascii_strtoll(url, NULL, 10);
	g_strlcpy(result->srvtype, type, sizeof(result->srvtype));
	g_strlcpy(result->host, host, sizeof(result->host));
	strcpy(result->args, args);

	return result;
}

void
meta1_service_url_clean(struct meta1_service_url_s *u)
{
	if (u) {
		u->seq = 0;
		u->srvtype[0] = u->host[0] = u->args[0] = 0;
		g_free(u);
	}
}

void
meta1_service_url_vclean(struct meta1_service_url_s **uv)
{
	struct meta1_service_url_s **p;

	if (!uv)
		return;
	for (p=uv; *p ;p++)
		meta1_service_url_clean(*p);
	g_free(uv);
}

gchar*
meta1_pack_url(struct meta1_service_url_s *u)
{
	return (NULL == u) ? NULL : g_strdup_printf(
			"%"G_GINT64_FORMAT"|%s|%s|%s",
			u->seq, u->srvtype, u->host, u->args);
}

gboolean
meta1_url_get_address(struct meta1_service_url_s *u,
		struct addr_info_s *dst)
{
	return l4_address_init_with_url(dst, u->host, NULL);
}

gboolean
meta1_strurl_get_address(const gchar *str, struct addr_info_s *dst)
{
	gboolean rc;
	struct meta1_service_url_s *u;

	u = meta1_unpack_url(str);
	rc = meta1_url_get_address(u, dst);
	g_free(u);

	return rc;
}

GError*
meta1_service_url_load_json_object(struct json_object *obj,
		struct meta1_service_url_s **out)
{
	EXTRA_ASSERT(out != NULL);
	if (!json_object_is_type(obj, json_type_object))
		return NEWERROR(CODE_BAD_REQUEST, "Invalid object type");
	struct json_object *s, *t, *h, *a;
	s = t = h = a = NULL;
	if (!json_object_object_get_ex(obj, "seq", &s)
		|| !json_object_object_get_ex(obj, "type", &t)
		|| !json_object_object_get_ex(obj, "host", &h)
		|| !json_object_object_get_ex(obj, "args", &a))
		return NEWERROR(CODE_BAD_REQUEST, "Missing field");
	if (!json_object_is_type(s, json_type_int)
		|| !json_object_is_type(t, json_type_string)
		|| !json_object_is_type(h, json_type_string)
		|| !json_object_is_type(a, json_type_string))
		return NEWERROR(CODE_BAD_REQUEST, "Invalid field type");
	struct meta1_service_url_s *m1u;
	size_t argslen = strlen(json_object_get_string(a));
	m1u = g_malloc0(sizeof(struct meta1_service_url_s) + 1 + argslen),
	m1u->seq = json_object_get_int64(s);
	g_strlcpy(m1u->srvtype, json_object_get_string(t), sizeof(m1u->srvtype));
	g_strlcpy(m1u->host, json_object_get_string(h), sizeof(m1u->host));
	g_strlcpy(m1u->args, json_object_get_string(a), argslen+1);
	*out = m1u;
	return NULL;
}

void
meta1_service_url_encode_json (GString *gstr, struct meta1_service_url_s *m1u)
{
	if (!m1u) {
		g_string_append(gstr, "nil");
	} else {
		g_string_append_printf(gstr, "{\"seq\":%"G_GINT64_FORMAT",",
				m1u->seq);
		g_string_append_printf(gstr, "\"type\":\"%.*s\",",
				(int)sizeof(m1u->srvtype), m1u->srvtype);
		g_string_append_printf(gstr, "\"host\":\"%.*s\",",
				(int)sizeof(m1u->host), m1u->host);
		g_string_append_printf(gstr, "\"args\":\"%.*s\"}",
				LIMIT_LENGTH_SRVARGS, m1u->args);
	}
}

