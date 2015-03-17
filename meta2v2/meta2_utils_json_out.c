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

#include <glib.h>

#include <metautils/lib/hc_url.h>
#include <metautils/lib/metautils.h>

#include <meta2v2/generic.h>
#include <meta2v2/autogen.h>

#include <meta2v2/meta2_utils_json.h>

static void
_json_BEAN_only(GString *gstr, GSList *l, gconstpointer selector,
		void (*code)(GString*,gpointer))
{
	gboolean first = TRUE;

	for (; l ;l=l->next) {
		if (DESCR(l->data) != selector)
			continue;
		if (!first)
			g_string_append_c(gstr, ',');
		first = FALSE;
		code(gstr, l->data);
	}
}

void
meta2_json_alias_only(GString *gstr, GSList *l)
{
	void code(GString *g, gpointer bean) {
		g_string_append_printf(g,
				"{\"name\":\"%s\","
				"\"ver\":%"G_GINT64_FORMAT","
				"\"ctime\":%"G_GINT64_FORMAT","
				"\"system_metadata\":\"%s\","
				"\"header\":\"",
				ALIASES_get_alias(bean)->str,
				ALIASES_get_version(bean),
				ALIASES_get_ctime(bean),
				ALIASES_get_mdsys(bean)->str);
		metautils_gba_to_hexgstr(g, ALIASES_get_content_id(bean));
		g_string_append(g, "\"}");
	}
	_json_BEAN_only(gstr, l, &descr_struct_ALIASES, code);
}

void
meta2_json_headers_only(GString *gstr, GSList *l)
{
	void code(GString *g, gpointer bean) {
		g_string_append(g, "{\"id\":\"");
		metautils_gba_to_hexgstr(g, CONTENTS_HEADERS_get_id(bean));
		g_string_append_printf(g, "\",\"hash\":\"");
		metautils_gba_to_hexgstr(g, CONTENTS_HEADERS_get_hash(bean));
		g_string_append_printf(g, "\",\"size\":%"G_GINT64_FORMAT,
				CONTENTS_HEADERS_get_size(bean));
		g_string_append_printf(g, ",\"policy\":\"%s\"}",
				CONTENTS_HEADERS_get_policy(bean)->str);
	}
	_json_BEAN_only(gstr, l, &descr_struct_CONTENTS_HEADERS, code);
}

void
meta2_json_contents_only(GString *gstr, GSList *l)
{
	void code(GString *g, gpointer bean) {
		g_string_append(g, "{\"hdr\":\"");
		metautils_gba_to_hexgstr(g, CONTENTS_get_content_id(bean));
		g_string_append_printf(g,
				"\",\"chunk\":\"%s\",\"pos\":\"%s\"}",
				CONTENTS_get_chunk_id(bean)->str,
				CONTENTS_get_position(bean)->str);
	}
	_json_BEAN_only(gstr, l, &descr_struct_CONTENTS, code);
}

void
meta2_json_chunks_only(GString *gstr, GSList *l)
{
	void code(GString *g, gpointer bean) {
		g_string_append_printf(g, "{\"id\":\"%s\",\"hash\":\"",
				CHUNKS_get_id(bean)->str);
		metautils_gba_to_hexgstr(g, CHUNKS_get_hash(bean));
		g_string_append_printf(g, "\",\"size\":%"G_GINT64_FORMAT"}",
				CHUNKS_get_size(bean));
	}
	_json_BEAN_only(gstr, l, &descr_struct_CHUNKS, code);
}

void
meta2_json_dump_all_beans(GString *gstr, GSList *beans)
{
	g_string_append(gstr, "\"aliases\":[");
	meta2_json_alias_only(gstr, beans);
	g_string_append(gstr, "],\"headers\":[");
	meta2_json_headers_only(gstr, beans);
	g_string_append(gstr, "],\"contents\":[");
	meta2_json_contents_only(gstr, beans);
	g_string_append(gstr, "],\"chunks\":[");
	meta2_json_chunks_only(gstr, beans);
	g_string_append(gstr, "]");
}


