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

"""General Module
It contains a set of generic methods for various data types."""

import time
import os

def do_it_between(start_time = 0, end_time = 0):
	"""Set up an execution time table
	start_time	->	format hhmm
	end_time	->	format hhmm

example:
	do_it_between(0700, 2300)
	This will set up an execution time table from 07:00 to 23:00.

	do_it_between(1800, 0500)
	This will set up an execution time table from 18:00 to next day 05:00."""

	current_time = time.localtime()
	now = current_time.tm_hour*100 + current_time.tm_min
	
	# case1, in the same day
	if start_time < end_time:
		return (now > start_time) and (now < end_time)
	elif start_time > end_time:
		# case2, passing mid-night
		return (now > start_time) or (now < end_time)
	else:
		return True

def is_older_than(time_to_compare, days):
	"""If the passed time argument is older than x days from now,
return True, otherwise False"""
	gap = time.time() - float(time_to_compare)
	gap = gap / 86400
	return gap > days

def is_younger_than(time_to_compare, days):
	"""If the passed time argument is older than x days from now,
return True, otherwise False"""
	gap = time.time() - float(time_to_compare)
	gap = gap / 86400
	return gap <= days

