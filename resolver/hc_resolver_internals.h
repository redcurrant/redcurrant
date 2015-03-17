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

#ifndef HC_RESOLVER__INTERNALS__H
# define HC_RESOLVER__INTERNALS__H 1
# include <resolver/hc_resolver.h>
# include <glib.h>

#ifndef  HC_RESOLVER_DEFAULT_MAX_SERVICES
# define HC_RESOLVER_DEFAULT_MAX_SERVICES 200000
#endif

#ifndef  HC_RESOLVER_DEFAULT_TTL_SERVICES
# define HC_RESOLVER_DEFAULT_TTL_SERVICES 3600
#endif

// No expiration and no max for content of META0 & Conscience
#ifndef  HC_RESOLVER_DEFAULT_MAX_CSM0
# define HC_RESOLVER_DEFAULT_MAX_CSM0 0
#endif

#ifndef  HC_RESOLVER_DEFAULT_TTL_CSM0
# define HC_RESOLVER_DEFAULT_TTL_CSM0 0
#endif

struct lru_tree_s;

struct cached_element_s
{
	time_t use;
	guint32 count_elements;
	gchar s[]; /* Must be the last! */
};

struct lru_ext_s
{
	struct lru_tree_s *cache;
	time_t ttl;
	guint max;
};

struct hc_resolver_s
{
	GMutex *lock;
	struct lru_ext_s services;
	struct lru_ext_s csm0;
	time_t bogonow;
	enum hc_resolver_flags_e flags;
};

#endif /* HC_RESOLVER__INTERNALS__H */
