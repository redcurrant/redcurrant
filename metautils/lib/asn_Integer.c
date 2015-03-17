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
#define G_LOG_DOMAIN "metacomm.integer.asn"
#endif

#include <glib.h>

#include "./metatypes.h"
#include "./metautils.h"
#include "./metacomm.h"

#include<INTEGER.h>

static int
write_in_gba(const void *b, gsize bSize, void *key)
{
	GByteArray *a = g_byte_array_append((GByteArray *) key, b, bSize);

	return a ? 0 : -1;
}

gboolean
simple_integer_unmarshall(const guint8 * bytes, gsize size, gint64 * result)
{
                void *i = NULL;
		asn_dec_rval_t decRet;
                asn_codec_ctx_t codecCtx;

                codecCtx.max_stack_size = 1 << 19;
                decRet = ber_decode(&codecCtx, &asn_DEF_INTEGER, &i, bytes, size);
		if (decRet.code != RC_OK) {
			ALERT("simple_integer_unmarshall error");
			return FALSE;
		}

                asn_INTEGER_to_int64(((INTEGER_t*)i), result);
		ASN_STRUCT_FREE(asn_DEF_INTEGER, i);
		return TRUE;
}

GByteArray*
simple_integer_marshall_gba(gint64 i64, GError **err)
{
	asn_enc_rval_t encRet;
	GByteArray *result = NULL;
	INTEGER_t asn1_integer;

	/*sanity checks */
	memset(&asn1_integer, 0x00, sizeof(INTEGER_t));
	asn_int64_to_INTEGER(&asn1_integer, i64);

	/*serialize the ASN.1 structure */
	result = g_byte_array_sized_new(12);
	encRet = der_encode(&asn_DEF_INTEGER, &asn1_integer, write_in_gba, result);
	ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_INTEGER, &asn1_integer);

	if (encRet.encoded == -1) {
		GSETERROR(err, "ASN.1 encoding error");
		g_byte_array_free(result, TRUE);
		return NULL;
	}

	/*free the ASN.1 structure */
	return result;
}

