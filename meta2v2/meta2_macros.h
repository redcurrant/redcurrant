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

#ifndef HC_M2V2_MACROS__H
#define  HC_M2V2_MACROS__H 1

#define M2V2_PROP_PREFIX_USER "user."
#define M2V2_PROP_PREFIX_SYS "sys."

#define M2_KEY_STORAGE_POLICY "STORAGE_POLICY"
#define M2_KEY_VERSION_POLICY "VERSION_POLICY"
#define M2_KEY_URL "HC_URL"
#define M2_KEY_GET_FLAGS "GET_ALIAS_FLAGS"
#define M2_KEY_ALIAS_VERSION "ALIAS_VERSION"
#define M2_KEY_COPY_SOURCE "COPY_SOURCE"
#define M2_KEY_CHUNK_ID "CHUNK_ID"
#define M2_KEY_OVERWRITE "OVERWRITE"
#define M2_KEY_SPARE "SPARE"
#define M2_KEY_RESTRICT_TO_ALIAS "RESTRICT_TO_ALIAS"
#define M2_KEY_NEW_CHUNKS "NEW_CHUNKS"
#define M2_KEY_OLD_CHUNKS "OLD_CHUNKS"
/* LIST params */
#define M2_KEY_LISTING_TYPE "LIST_TYPE"
#define S3_LISTING_TYPE "S3"
#define REDC_LISTING_TYPE "REDC"
#define M2_KEY_NAME_PATTERN "NAME_PATTERN"
#define M2_KEY_METADATA_PATTERN "METADATA_PATTERN"
#define M2_KEY_PREFIX "PREFIX"
#define M2_KEY_MARKER "MARKER"
#define M2_KEY_DELIMITER "DELIMITER"
#define M2_KEY_MAX_KEYS "MAX_KEYS"
#define M2_KEY_SNAPSHOT_HARDRESTORE "HARD_RESTORE"

/* ---- LEGACY CONSTANTS ---- */

#define M2V1_KEY_VNS "VIRTUAL_NAMESPACE"
#define M2V1_KEY_REF "CONTAINER_NAME"
#define M2V1_KEY_REFID "CONTAINER_ID"
#define M2V1_KEY_PATH "CONTENT_PATH"
#define M2V1_KEY_METADATA_USR "METADATA_USR"
#define M2V1_KEY_WARNING "WARNING"
#define M2V1_KEY_TIMESTAMP "TIMESTAMP"
#define M2V1_KEY_VIRTUAL_NAMESPACE "VIRTUAL_NAMESPACE"
#define M2V1_KEY_METADATA_SYS "METADATA_SYS"
#define M2V1_KEY_CONTAINER_ID "CONTAINER_ID"
#define M2V1_KEY_CONTAINER_NAME "CONTAINER_NAME"
#define M2V1_KEY_CONTENT_PATH "CONTENT_PATH"
#define M2V1_KEY_CONTENT_LENGTH "CONTENT_LENGTH"
#define M2V1_KEY_PROPERTY_NAME "PROPERTY_NAME"
#define M2V1_KEY_PROPERTY_VALUE "PROPERTY_VALUE"
#define M2V1_KEY_METADATA_USER "METADATA_USR"
#define M2V1_KEY_ADMIN_KEY "ADMIN_KEY"
#define M2V1_KEY_ADMIN_VALUE "ADMIN_VALUE"
#define M2V1_KEY_FIELD_ZERO "field_0"
#define M2V1_KEY_FIELD_ONE "field_1"
#define M2V1_KEY_FIELD_TWO "field_2"
#define M2V1_KEY_FIELD_THREE "field_3"
#define M2V1_KEY_FIELD_FOUR "field_4"

#define MDUSR_PROPERTY_KEY "sys.m2v1_mdusr"

#endif
