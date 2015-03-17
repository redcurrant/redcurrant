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

#ifndef RC_metautils_storage_policy_internals__h
# define RC_metautils_storage_policy_internals__h 1

#include <glib.h>

#include "storage_policy.h"

struct data_security_s
{
	gchar *name;
	enum data_security_e type;
	GHashTable *params;
};

struct data_treatments_s
{
	gchar *name;
	enum data_treatments_e type;
	GHashTable *params;
};

struct storage_class_s
{
	gchar *name;
	GSList *fallbacks;
};

struct storage_policy_s
{
	gchar *name;
	struct data_security_s *datasec;
	struct data_treatments_s *datatreat;
	struct storage_class_s *stgclass;
};

#endif
