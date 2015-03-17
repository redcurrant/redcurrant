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

from pygrid.services import Conscience, Meta1, Sqlx
from pygrid.locate import ConscienceLocator, Meta0Locator, Meta1Locator, V2ServiceLocator
from pygrid.gridd import ReplicatedGridd
from pygrid.gridd import make_gridd_from_url
from pygrid.utils import GridUrl, ContainerId
import pygrid.exceptions as exceptions

def check_storage_policy(content):
        print "Checking storage policy of content "+content.namespace+"/"+content.container_id+"/"+content.name
        ns_name = ctypes.c_char_p(content.namespace)
        cid = ctypes.c_char_p(content.container_id)
	cname = ctypes.c_char_p(content.name)
        lib = ctypes.cdll.LoadLibrary("librulesmotorpy2c.so")
        lib.motor_check_storage_policy(ns_name, cid, cname)

def get_m1_locator():
	return Meta1Locator(Meta0Locator(ConscienceLocator()))

# Send UPDATEM1POLICY request to m1
def check_meta1_policy(container, servicetype, m1_locator=None):
	print "Checking meta1 policy of container " + container.id
	if m1_locator is None:
		m1_locator = get_m1_locator()
	cid = ContainerId(hexa=container.id)
	m1_addrs = m1_locator.locate(container.namespace, cid)
	m1 = Meta1(container.namespace, ReplicatedGridd(m1_addrs))
	m1.v2_update_policy(servicetype, cid=container.id)
	
# Send RESYNC to all slaves
def check_sqlx_replication(container, servicetype, m1_locator=None):
	print "Checking sqlx policy of container " + container.id
	if m1_locator is None:
		m1_locator = get_m1_locator()
	cid = ContainerId(hexa=container.id)
	check_meta1_policy(container, servicetype, m1_locator)
	v2_svc_locator = V2ServiceLocator(m1_locator)
	srvs = v2_svc_locator.locate(container.namespace, cid, servicetype)
	for srv in srvs:
		strid, strtype, straddr, strargs = srv.split('|', 4)
		base_name = strid+'@'+container.id
		sqlx = Sqlx(make_gridd_from_url(straddr))
		try:
			sqlx.status(base_name, servicetype)
		except exceptions._GridException as ex:
			if ex.code == 303:
				print "Sending RESYNC on " + srv
				sqlx.resync(base_name, servicetype)
				continue
			else:
				raise ex
		print srv + " is master, not sending RESYNC"
		