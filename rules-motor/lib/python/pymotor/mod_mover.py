# Copyright (C) 2015 Worldline
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import ctypes
import pygrid.sqlx as sqlx
import os.path as ospath

def move_chunk(path):
	print "Moving chunk to another rawx"


def move_container(ns_name, cid):
	print "Moving container " + cid + " to another meta2"
	ns_name = ctypes.c_char_p(ns_name)
	cid = ctypes.c_char_p(cid)
	lib = ctypes.cdll.LoadLibrary("librulesmotorpy2c.so")
	rc = lib.motor_move_container(ns_name, cid)
	return bool(rc)

def move_sqlx(ns_name, sqlx_addr, path, cid, type):
	url=ospath.join(ns_name, cid);
	print "Moving sqlx database (" + url + "|" + type + ") to another sqlx"
	#rc = sqlx.move_base(url, type, None, True); # no delete src file, but a "699|'backup error: SQLITE_?'\n" appears
	rc = sqlx.move_base(url, type, None, True); # delete src file
	return bool(rc)



