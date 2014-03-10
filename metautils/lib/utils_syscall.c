#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>

#include "metautils_syscall.h"

static struct metautils_syscalls_vtable_s VTABLE = {
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL,
	NULL, NULL,
};

void
metautils_set_vtable_syscall(struct metautils_syscalls_vtable_s *vtable)
{
	memcpy(&VTABLE, vtable, sizeof(VTABLE));
}

int
metautils_syscall_open(const char *p, int flag, int mode)
{
	if (VTABLE.open)
		return VTABLE.open(p, flag, mode);
	return open(p, flag, mode);
}

int
metautils_syscall_unlink(const char *p)
{
	if (VTABLE.unlink)
		return VTABLE.unlink(p);
	return unlink(p);
}

int
metautils_syscall_socket(int domain, int type, int protocol)
{
	if (VTABLE.socket)
		return VTABLE.socket(domain, type, protocol);
	return socket(domain, type, protocol);
}

int
metautils_syscall_connect(int fd, const struct sockaddr *addr, socklen_t alen)
{
	if (VTABLE.connect)
		return VTABLE.connect(fd, addr, alen);
	return connect(fd, addr, alen);
}

int
metautils_syscall_accept(int fd, struct sockaddr *addr, socklen_t * alen)
{
	if (VTABLE.accept)
		return VTABLE.accept(fd, addr, alen);
	return accept(fd, addr, alen);
}

int
metautils_syscall_accept4(int fd, struct sockaddr *addr, socklen_t * alen,
	int flags)
{
	if (VTABLE.accept4)
		return VTABLE.accept4(fd, addr, alen, flags);
#ifdef HAVE_ACCEPT4
	return accept4(fd, addr, alen, flags);
#else
	return metautils_syscall_accept(srv, addr, alen);
#endif
}

ssize_t
metautils_syscall_write(int fd, const void *buf, size_t count)
{
	if (VTABLE.write)
		return VTABLE.write(fd, buf, count);
	return write(fd, buf, count);
}

ssize_t
metautils_syscall_read(int fd, void *buf, size_t count)
{
	if (VTABLE.read)
		return VTABLE.read(fd, buf, count);
	return read(fd, buf, count);
}

int
metautils_syscall_poll(struct pollfd *fds, int nfds, int timeout)
{
	if (VTABLE.poll)
		return VTABLE.poll(fds, nfds, timeout);
	return poll(fds, nfds, timeout);
}

int
metautils_syscall_close(int fd)
{
	if (VTABLE.close)
		return VTABLE.close(fd);
	return close(fd);
}

int
metautils_syscall_shutdown(int fd, int how)
{
	if (VTABLE.shutdown)
		return VTABLE.shutdown(fd, how);
	return shutdown(fd, how);
}


int
metautils_syscall_getsockopt(int fd, int lvl, int opt, void *v, socklen_t * vl)
{
	if (VTABLE.getsockopt)
		return VTABLE.getsockopt(fd, lvl, opt, v, vl);
	return getsockopt(fd, lvl, opt, v, vl);
}

int
metautils_syscall_setsockopt(int fd, int lvl, int opt, const void *v,
	socklen_t vl)
{
	if (VTABLE.setsockopt)
		return VTABLE.setsockopt(fd, lvl, opt, v, vl);
	return setsockopt(fd, lvl, opt, v, vl);
}
