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

##################################################################
#
# listener&reduce
#
#===============================================================================
# FORMAT of data on Reduce:go(..) function: JSON
#
# MANDATORY: "HEAD"/KEY name about config, iAND section "DATA"
# ----------
# "HEAD": {
#     "NAME": "action_nam"  -->  the name of proc source msg, define algo of reduce
#     "PID":                --> the pid of  proc src
#     "STATUS":             --> the status of proc src
#     "MSG_ID":             --> counter of msg to received, increase by one each time
#                              for each proc src
# }
# "DATAH": {
# <key/val>                 --> specific key/value for the "HEAD"/"NAME",
#                                data NOT reduce, but used for reduce "DATA" section
#                                     ---
# "DATA": {
# <key/ val>                --> specific key/value for the "HEAD"/"NAME",
#                                data to reduce
# }
#-------------------------------------------------------------------------------
# example:
#{ 
#	"HEAD": { 
#		"PID": 16181, 
#		"STATUS": 0, 
#		"MSG_ID": 19 
#	}, 
#   "DATAH": {
#		"PREFIX": "5C15", 
#		"M1_ID": "127.0.0.1:6002", 
#		"SVC_ID": "", 
#		"SVC_TYPE": ""
#	}
#	"DATAR": { 
#		"SVC_NB_CONTAINER_M2onM1": 0, 
#		"SVC_ERR_REQ_META1": 0, 
#		"M1_NB_PREFIX": 1, 
#		"M1_NB_CONTAINER": 1, 
#		"M1_ERR_BADBDDFILE": 0, 
#		"M1_ERR_BADPREFIX": 0, 
#		"M1_ERR_REQ_META1": 0 
#	} 
#}
#===============================================================================



##################################################################


import sys
from reduce.reducerBase import ReducerBase




##################################################################
# config
##################################################################



##################################################################
##################################################################
# class Reduce / action_test_service
#
#---------------------------------------
#	ReWrite this class for update with your specific reduce data
#---------------------------------------
#
##################################################################
##################################################################
class ReducerActionTestService(ReducerBase):

	ACTION_NAME = "action_test"

	#head of data to reduce
	JSON_KEYNAME_HEAD_NAME = "NAME"

	COLLDATA_KEYNAME_FILES = "FILES"
	COLLDATA_KEYNAME_NBFILES = "NBFILES"

		
	#MANDATORY function
	def __init__(self, bVerbose):
		ReducerBase.__init__(self, self.ACTION_NAME, bVerbose)


	#MANDATORY function
	def reduce(self, datah, datar):
		#get header field
		filename = datah[self.JSON_KEYNAME_HEAD_NAME]

		for key in datar:
			self.aggregate_CONCAT(self.collecter_data, self.COLLDATA_KEYNAME_FILES, datar[key], "|")
			self.aggregate_COUNT(self.collecter_data, key, self.COLLDATA_KEYNAME_NBFILES)
			

	def dumps(self):
		resultm1 = ""
		resultsvc = "" 
		result = "";
		#manage other key after
		for key in self.collecter_data:
			result += "    " + key + " = " + str(self.collecter_data[key]) + "\n"
			
		return result				
		

		def finalize(self):
			self.printmsg( "finalize() data")





