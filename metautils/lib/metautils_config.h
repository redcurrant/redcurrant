#ifndef HC_metautils_config__h
#define HC_metautils_config__h 1

#define CONF_KEY_MAX_VERSIONS    "max_versions"

#define CONF_KEY_STORAGE_POLICY  "storage_policy"
#define CONF_KEY_DATA_SECURITY   "data_security"
#define CONF_KEY_DATA_TREATMENTS "data_treatments"
#define CONF_KEY_STORAGE_CLASS   "storage_class"

/** Timeout when opening bases in use by another thread
 * -1 (infinite), 0 (immediate), or milliseconds */
#define CONF_KEY_SQLX_TIMEOUT_OPEN       "timeout_open"
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


#endif
