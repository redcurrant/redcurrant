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

#include "internals.h"

MESSAGE
meta2_remote_build_request(GError **err, GByteArray *id, char *name)
{
        MESSAGE msg=NULL;

        message_create(&msg, err);
	if (!msg)
		return NULL;

        if (id)
                message_set_ID (msg, id->data, id->len, err);
        if (name)
                message_set_NAME (msg, name, strlen(name), err);

        return msg;
}

