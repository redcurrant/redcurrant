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

#ifndef RC_SVCMONITOR_utils_h
#define RC_SVCMONITOR_utils_h 1

/** @file Extension to the gridinit-utils library.
 * Not destined to be installed.
 */

// Used in rawx-monitor and svc-monitor
static inline void
supervisor_preserve_env (const gchar *child)
{
	gchar **keys = g_listenv();
	if (!keys)
		return;
	for (gchar **pk = keys; *pk ;++pk)
		supervisor_children_setenv(child, *pk, g_getenv(*pk));
	g_strfreev(keys);
}

#endif // RC_SVCMONITOR_utils_h
