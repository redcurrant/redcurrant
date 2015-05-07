#ifndef HC_metautils_config__h
#define HC_metautils_config__h 1

/* This file inventories the parameters that can be changed globally
 * from the conscience. Some services also support changing these parameters
 * locally, from their commandline. To set a parameter only for a specific
 * type of service, prefix the parameter with the service type followed
 * by an underscore (e.g. "meta2_max_workers"). */


/** Maximum size for chunks, in bytes */
#define CONF_KEY_CHUNK_SIZE      "chunk_size"

/** Maximum number of versions of a content:
 *  0 -> only one (default)
 *  1 -> only one, but no error when overwriting
 *  N -> N versions
 * -1 -> unlimited */
#define CONF_KEY_MAX_VERSIONS    "max_versions"

/** Maximum size of all contents in a container, in bytes */
#define CONF_KEY_MAX_CONTAINER_SIZE "max_container_size"

/** The storage policy to use for new contents */
#define CONF_KEY_STORAGE_POLICY  "storage_policy"

#define CONF_KEY_DATA_SECURITY   "data_security"
#define CONF_KEY_DATA_TREATMENTS "data_treatments"
#define CONF_KEY_STORAGE_CLASS   "storage_class"

/** How long to keep deleted aliases in versioned containers (-1 is forever) */
#define CONF_KEY_KEEP_DELETED_DELAY      "keep_deleted_delay"

/** Prefix for keys configuring the load-balancers.
 * The suffix is usually a service type, eg "lb.rawx". */
#define CONF_KEY_PREFIX_LB "lb."

/** Timeout when opening bases in use by another thread
 * -1 (infinite), 0 (immediate), or milliseconds */
#define CONF_KEY_SQLX_TIMEOUT_OPEN       "timeout_open"

/** The maximum number of threads concurrently trying to open
 * a base currently in use. */
#define CONF_KEY_SQLX_MAX_WAITING        "max_threads_waiting_on_open"

/** Limits the number of worker threads */
#define CONF_KEY_SQLX_MAX_WORKERS        "max_workers"

/** Amount of free memory to keep for future allocations (kB) */
#define CONF_KEY_SQLX_MAX_HEAP_FREE      "max_heap_free"

/** Limits the number of concurrent active connections */
#define CONF_KEY_SQLX_MAX_CNX_ACTIVE     "max_cnx_active"

/** Limits the number of concurrent passive connections */
#define CONF_KEY_SQLX_MAX_CNX_PASSIVE    "max_cnx_passive"

/** Number of connections allowed when all workers are busy */
#define CONF_KEY_SQLX_MAX_CNX_IN_BACKLOG "max_cnx_in_backlog"

//--------
// RAWX
//--------
#define CONF_KEY_RAWX_COMPRESSION_BLOCKSIZE "compression_blocksize"
#define CONF_KEY_RAWX_COMPRESSION_ALGORITHM "compression_algorithm"
#define CONF_KEY_RAWX_HASH_WIDTH            "hash_width"
#define CONF_KEY_RAWX_HEADER_SCHEME         "header_scheme"

/** Do an fsync syscall when closing a just created chunk.
 * The following values are supported:
 *   0 to disable;
 *   1 to fsync the chunk file;
 *   2 to fsync the directory where the chunk is referenced;
 *   3 to fsync the chunk file and its directory. */
#define CONF_KEY_RAWX_FSYNC_ON_CLOSE        "fsync_on_close"

#define CONF_KEY_RAWX_FILE_BUFFER_SIZE      "file_buffer_size"

/** Delay (in seconds) for rawx stat smoothing, and request rate
 * computation. Can be set from 1 to 60. */
#define CONF_KEY_RAWX_FILE_SMOOTHING_DELAY  "file_smoothing_delay"

#define CONF_KEY_RAWX_STREAM_BUFFER_SIZE    "stream_buffer_size"

#endif
