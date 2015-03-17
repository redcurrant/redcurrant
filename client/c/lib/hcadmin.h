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

#ifndef __HC_ADMIN__H__
#define __HC_ADMIN__H__

gs_error_t * hcadmin_meta1_policy_update(char *ns, gchar *action, gboolean checkonly, gchar **globalresult, gchar ***result, char ** args);
gs_error_t * hcadmin_touch(              char *url,gchar *action, gboolean checkonly, gchar **globalresult, gchar ***result, char ** args);






#endif /* __HC_ADMIN__H__ */

