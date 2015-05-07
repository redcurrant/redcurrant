#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION

#include <glib.h>

#include <metautils/lib/metautils.h>
#include <cluster/lib/gridcluster.h>

#include "rainx_config.h"
#include "rainx_internals.h"

#define RAINX_CONF_UPDATE_DELAY 10

static void
_addr_rule_gclean(gpointer data, gpointer udata)
{
	(void) udata;
	addr_rule_g_free(data);
}

/**********************************************************************/

char *
_get_compression_algorithm(apr_pool_t *p, namespace_info_t *ns_info)
{
	gchar *found_algo;
	char *res;

	found_algo = namespace_info_get_srv_param_str(ns_info, NULL, NAME_SRVTYPE_RAWX,
			CONF_KEY_RAWX_COMPRESSION_ALGORITHM, DEFAULT_COMPRESSION_ALGO);
	res = apr_pstrdup(p, found_algo);
	g_free(found_algo);

	return res;
}

apr_int64_t
_get_compression_block_size(apr_pool_t *p, namespace_info_t *ns_info)
{
	(void) p;
	return namespace_info_get_srv_param_i64(ns_info, NULL, NAME_SRVTYPE_RAWX,
			CONF_KEY_RAWX_COMPRESSION_BLOCKSIZE, DEFAULT_STREAM_BUFF_SIZE);
}

GSList*
_get_acl(apr_pool_t *p, namespace_info_t *ns_info)
{
	GSList *acl = NULL;
	GByteArray* acl_allow, *acl_deny;

	if (!ns_info || !ns_info->options)
		return NULL;

	acl_allow = g_hash_table_lookup(ns_info->options, NS_ACL_ALLOW_OPTION);
	acl_deny = g_hash_table_lookup(ns_info->options, NS_ACL_DENY_OPTION);

	acl = g_slist_concat(parse_acl(acl_allow, TRUE), parse_acl(acl_deny, FALSE));
	if (!acl)
		return NULL;

	GSList *src, *dst;
	guint i, list_length;

	list_length = g_slist_length(acl);

	/* Copy the list content */
	dst = apr_pcalloc(p, sizeof(GSList) * list_length);
	if (list_length > 1) {
		for (i=0; i < list_length - 2; i++)
			dst[i].next = dst + i + 1;
	}

	/* copy the original data */
	for (i=0, src=acl; src ;src=src->next,i++) {
		if (src->data)
			dst[i].data = apr_pmemdup(p, src->data, sizeof(addr_rule_t));
	}

	g_slist_foreach(acl, _addr_rule_gclean, NULL);
	g_slist_free(acl);
	return dst;
}

gboolean
update_rainx_conf_if_necessary(apr_pool_t* p, rawx_conf_t **rainx_conf)
{
	time_t now = time(0);
	if ((*rainx_conf)->last_update + RAINX_CONF_UPDATE_DELAY < now) {
		// (*rainx_conf)->ni->name will be freed, we must make a copy
		gchar ns_name[LIMIT_LENGTH_NSNAME] = {0};
		g_strlcpy(ns_name, (*rainx_conf)->ni->name, LIMIT_LENGTH_NSNAME);
		gboolean res = update_rainx_conf(p, rainx_conf, ns_name);
		return res;
	}
	return FALSE;
}

gboolean
update_rainx_conf(apr_pool_t* p, rawx_conf_t **rainx_conf, const gchar* ns_name)
{
	GError *local_error = NULL;
	namespace_info_t* ns_info = NULL;
	struct storage_policy_s *stgpol = NULL;
	GSList *acls = NULL;

	if (!ns_name || !ns_name[0]) {
		DAV_ERROR_POOL(p, 0,
				"Namespace is null or empty string, cannot update conf");
		return FALSE;
	}

	ns_info = get_namespace_info(ns_name, &local_error);
	if (!ns_info) {
		if (local_error != NULL) {
			DAV_ERROR_POOL(p, 0, "%s", local_error->message);
			g_clear_error(&local_error);
		}
		return FALSE;
	}

	char *polname = NULL;
	if(!(polname = namespace_storage_policy(ns_info, ns_info->name)))
		goto error_label;

	stgpol = storage_policy_init(ns_info, polname);
	g_free(polname);
	if (stgpol == NULL) {
		goto error_label;
	}

	// FIXME: free ACLs somewhere
	acls = _get_acl(p, ns_info);

	if (*rainx_conf != NULL) {
		/* ACLs are allocated with APR, we must prevent them from being
		 * cleaned by g_free. */
		(*rainx_conf)->acl = NULL;
		/* Do not free, just clean and reuse the memory */
		rawx_conf_clean(*rainx_conf);
	} else {
		/* Allocate on server's pool */
		*rainx_conf = apr_palloc(p, sizeof(rawx_conf_t));
	}

	/* Copy references that were allocated with glib */
	(*rainx_conf)->ni = ns_info;
	(*rainx_conf)->sp = stgpol;
	(*rainx_conf)->acl = acls;
	(*rainx_conf)->last_update = time(0);

	return TRUE;

error_label:
	if (ns_info != NULL) {
		namespace_info_free(ns_info);
	}
	if (stgpol != NULL) {
		storage_policy_clean(stgpol);
	}
	return FALSE;
}

