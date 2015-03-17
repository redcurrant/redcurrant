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

#include <metautils/lib/metautils.h>
#include "hash.h"

int
main(int argc, char **argv)
{
	if (argc < 2 || 1 != (argc % 2)) {
		g_printerr("Usage: %s (NAME TYPE)...\n", argv[0]);
		return 0;
	}

	for (int i=1; i<argc-1 ;i+=2) {
		const gchar *n = argv[i], *t = argv[i+1];
		struct hashstr_s *h = sqliterepo_hash_name(n, t);
		g_print("%s.%s %s.%s\n", n, t, hashstr_str(h), t);
		g_free(h);
	}
	return 0;
}

