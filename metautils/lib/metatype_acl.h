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

#ifndef __REDCURRANT_metatype_acl__h
#define __REDCURRANT_metatype_acl__h 1
#include <glib/gtypes.h>

/**
 * @defgroup metautils_utils_acl ACL 
 * @ingroup metautils_utils
 * @brief ACL utils
 * @details Handles access control lists got from the conscience.
 * @{
 */

/**
 * @param addr
 * @param acl
 * @return
 */
gboolean authorized_personal_only(const gchar* addr, GSList* acl);


/**
 * @param acl_byte
 * @param authorize
 * @return
 */
GSList* parse_acl(const GByteArray* acl_byte, gboolean authorize);


/**
 * @param file_path
 * @param error
 * @return
 */
GSList* parse_acl_conf_file(const gchar* file_path, GError **error);


/**
 * @param addr_rule
 * @return
 */
gchar* access_rule_to_string(const addr_rule_t* addr_rule);


/**
 * @param data
 */
void addr_rule_g_free(gpointer data);

#endif // __REDCURRANT_metatype_acl__h
