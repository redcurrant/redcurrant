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

#ifndef __REDCURRANT_metatype_m1url__h
#define __REDCURRANT_metatype_m1url__h 1
#include <glib/gtypes.h>

/**
 */
struct meta1_service_url_s
{
	gint64 seq;        /**<  */
	gchar srvtype[LIMIT_LENGTH_SRVTYPE]; /**<  */
	gchar host[256];   /**<  */
	gchar args[1];     /**<  */
};

/**
 * @param url
 * @return
 */
struct meta1_service_url_s* meta1_unpack_url(const gchar *url);

/**
 * @param u
 */
void meta1_service_url_clean(struct meta1_service_url_s *u);

/**
 * @param uv
 */
void meta1_service_url_vclean(struct meta1_service_url_s **uv);

/**
 * @param u
 * @return
 */
gchar* meta1_pack_url(struct meta1_service_url_s *u);

/**
 * @param u
 * @param dst
 * @return
 */
gboolean meta1_url_get_address(struct meta1_service_url_s *u,
		struct addr_info_s *dst);

gboolean meta1_strurl_get_address(const gchar *str, struct addr_info_s *dst);

GError* meta1_service_url_load_json_object(struct json_object *obj,
		struct meta1_service_url_s **out);

void meta1_service_url_encode_json (GString *gstr,
		struct meta1_service_url_s *m1u);

/** @} */

#endif // __REDCURRANT_metatype_m1url__h
