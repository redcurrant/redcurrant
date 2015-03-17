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

#ifndef redcurrant__metautils_lib_test_addr_h
#define redcurrant__metautils_lib_test_addr_h 1

static gchar *bad_urls[] =
{
	"",
	"6000",
	":6000",
	"127.0.0.1",
	"127.0.0.1:6000:",
	" 127.0.0.1:6000",
	"127.0.0.1:6000 ",
	"127.0.0.1 :6000",
	"127.0.0.1: 6000",
	":127.0.0.1:6000:",
	":127.0.0.1:6000",
	"127.0.0.1::6000",
	"127.0.0.1:0",
	"0.0.0.0:0",
	"0.0.0.0:6000",
	"1|meta2|127.0.0.1:6000|",
	"1|meta2|127.0.0.1:6000",

	"::",
	"[:::0",
	"::]:6000",
	"::]:0",
	"[::]:6000",
	"[::] :6000",
	"[::] :6000",
	"[::]: 6000 ",
	" [::]: 6000",
	" [::]: 6000 ",
	" [::] : 6000 ",
	" [::]:6000 ",

	NULL
};

static gchar *good_urls[] =
{
	"127.0.0.1:6000",
	"[::1]:6000",

	NULL
};

static inline void
test_on_urlv(gchar **urlv, void (*test)(const gchar *))
{
	for (; *urlv ;++urlv)
		test(*urlv);
}

#define URL_ASSERT(C) do { \
	if (!BOOL(C)) \
		g_error("<%s> Failed with [%s]", pProc, url); \
} while (0)

#endif
