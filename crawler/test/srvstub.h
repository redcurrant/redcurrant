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

#ifndef __SRVSTUB_H
#define __SRVSTUB_H



typedef enum {
	SSCMD_ALL_OK = 0,
	SSCMD_ALL_NONE,
	SSCMD_ALL_OK_WITHOUTDATA,
	SSCMD_ALL_ERR_WITHOUTDATA,
    SSCMD_ONE_ERR_WITHOUTDATA,

	SSCMD_max
} ESrvStubCmd;

typedef struct SSrvStubHandle TSrvStubHandle;

TSrvStubHandle* srvstub_init( char* url, ESrvStubCmd sscmd, char* name, void* responsedata);
GError*         srvstub_run(  TSrvStubHandle* s);
int             srvstub_close(TSrvStubHandle** s);




#endif

