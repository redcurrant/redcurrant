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

#ifndef __CRAWLER_TRIP_COMMON_H
#define __CRAWLER_TRIP_COMMON_H


/**
 *  * \brief initialize repository structure
 *   *
 *    * basedir:  path base
 *     * svc_type: SQLX_TYPE / MITA1_TYPE....
 *      * repo:     the final structur initalised
 *       */
GError* tc_sqliterepo_initRepository(const gchar* basedir, gchar* svc_type, sqlx_repository_t **repo);

gchar*  tc_sqliterepo_admget(sqlx_repository_t* repo, gchar* type_, gchar* bddnameWithExtension, gchar* key);

void    tc_sqliterepo_free(sqlx_repository_t** repo);


#endif 

