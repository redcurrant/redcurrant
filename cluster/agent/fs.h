#include <sys/vfs.h>

long get_free_space(const char *path, long chunk_size, const struct statfs *stat_fs);
gboolean get_statfs(const char *path, struct statfs *p_sfs);
