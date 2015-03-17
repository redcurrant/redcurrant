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

#ifndef META2_UTILS_JSON_H
#define META2_UTILS_JSON_H

#include <glib.h>
#include <json.h>
#include <metautils/lib/hc_url.h>

/**
 * Convert alias beans to their JSON representation.
 *
 * @param gstr The output string
 * @param l A list of beans
 */
void meta2_json_alias_only(GString *gstr, GSList *l);

/**
 * Convert header beans to their JSON representation.
 *
 * @param gstr The output string
 * @param l A list of beans
 */
void meta2_json_headers_only(GString *gstr, GSList *l);

/**
 * Convert content beans to their JSON representation.
 *
 * @param gstr The output string
 * @param l A list of beans
 */
void meta2_json_contents_only(GString *gstr, GSList *l);

/**
 * Convert chunk beans to their JSON representation.
 *
 * @param gstr The output string
 * @param l A list of beans
 */
void meta2_json_chunks_only(GString *gstr, GSList *l);

/**
 * Serialize beans to JSON. The output does not contain
 * the initial '{' and final '}' characters, to allow easier
 * inclusion in a dictionary.
 *
 * @param gstr The output string
 * @param l A list of beans
 */
void meta2_json_dump_all_beans(GString *gstr, GSList *beans);

// TODO: add a function to output json_object


/**
 * Extract a list of beans from a JSON object.
 *
 * @param beans The output list of beans
 * @param jbody The JSON object to extract beans from
 * @return A GError in case of error, NULL otherwise
 */
GError *meta2_json_object_to_beans(GSList **beans, struct json_object *jbeans);

#endif // META2_UTILS_JSON_H

