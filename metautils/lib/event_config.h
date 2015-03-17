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

/*!
 * @file
 */

#ifndef GRID__EVTCONFIG_H
# define GRID__EVTCONFIG_H 1
# include <glib.h>

/**
 * @defgroup metautils_evtconfig Configuration for container notification events
 * @ingroup metautils_utils
 * @{
 */

/* hidden structures */
struct event_config_s;

/*!
 * @return
 */
struct event_config_s* event_config_create(void);

/*!
 * @param evt_config
 */
void event_config_destroy(struct event_config_s *evt_config);

GError* event_config_reconfigure(struct event_config_s *ec, const gchar *cfg);

/**
 * @param evt_config
 * @return
 */
gboolean event_is_enabled(struct event_config_s *evt_config);

/**
 * @param evt_config
 * @return
 */
gboolean event_is_aggregate(struct event_config_s *evt_config);

/**
 *
 *
 */
const gchar* event_get_dir(struct event_config_s *evt_config);

/**
 *
 *
 */
GMutex* event_get_lock(struct event_config_s *evt_config);

/**
 *
 *
 */
gint64 event_get_and_inc_seq(struct event_config_s *evt_config);

/*!
 * @param evt_config
 * @return
 */
gchar* event_config_dump(struct event_config_s *evt_config);

/*! @} */

#endif /* GRID__EVTCONFIG_H */
