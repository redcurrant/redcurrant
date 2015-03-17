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
# define G_LOG_DOMAIN "metautils.url"
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <glib.h>

#include "./metautils.h"

static void
_dummy_gba(GByteArray *gba, guint v, register guint len)
{
	g_byte_array_set_size(gba, 0);
	for (register guint i=0; i<len ;++i)
		gba = g_byte_array_append(gba, (guint8*)&v, sizeof(v));
}

#define COUNT 65536

static void
test_gba_cmp(void)
{
	GByteArray *a = g_byte_array_new();
	GByteArray *b = g_byte_array_new();

	guint v = 0;
	for (register guint i=0; i<COUNT ;++i) {
		_dummy_gba(a, v++, 4);
		_dummy_gba(b, v++, 4);
		g_assert(0 != metautils_gba_cmp(a, b));
	}
	for (register guint i=0; i<COUNT ;++i) {
		_dummy_gba(a, v++, 4);
		_dummy_gba(b, v++, 5);
		g_assert(0 != metautils_gba_cmp(a, b));
	}
	for (register guint i=0; i<COUNT ;++i) {
		_dummy_gba(a, v, 4);
		_dummy_gba(b, v, 5);
		v++;
		g_assert(0 != metautils_gba_cmp(a, b));
	}
	for (register guint i=0; i<COUNT ;++i) {
		_dummy_gba(a, v, 4);
		_dummy_gba(b, v, 4);
		v++;
		g_assert(0 == metautils_gba_cmp(a, b));
	}
	for (register guint i=0; i<COUNT ;++i) {
		_dummy_gba(a, v, 4);
		v++;
		g_assert(0 == metautils_gba_cmp(a, a));
	}

	g_byte_array_free(a, TRUE);
	g_byte_array_free(b, TRUE);
}

int
main(int argc, char **argv)
{
	HC_TEST_INIT(argc, argv);
	g_test_add_func("/metautils/gba/cmp", test_gba_cmp);
	return g_test_run();
}

