
/*
 * Copyright (C) 2022  SCS Lab <scslab@iit.edu>,
 * Luke Logan <llogan@hawk.iit.edu>,
 * Jaime Cernuda Garcia <jcernudagarcia@hawk.iit.edu>
 * Jay Lofstead <gflofst@sandia.gov>,
 * Anthony Kougkas <akougkas@iit.edu>,
 * Xian-He Sun <sun@iit.edu>
 *
 * This file is part of LabStor
 *
 * LabStor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE
//#define DISABLE_LABSTOR

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>
#include <search.h>
#include <assert.h>
#include <libgen.h>
#include <pthread.h>
#include <stdint.h>
#include <limits.h>

#include <dlfcn.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>
#include <aio.h>

#include "generic_posix_client.h"

/**
 * PROTOTYPES
 * */

#define WRAPPER_FUN(x) x
#define REAL_FUN(x) __real_##x
#define FNAME_TYPE(T, fname, ...) T (*)(__VA_ARGS__)
#define FORWARD_DECL(T, fname, ...) \
    T (*REAL_FUN(fname))(__VA_ARGS__) = NULL;
#define GETFUN(T, fname, ...) \
    if(!REAL_FUN(fname)) { REAL_FUN(fname) = (FNAME_TYPE(T, fname, __VA_ARGS__))dlsym(RTLD_NEXT, #fname); }

FORWARD_DECL(int, open, const char *path, int oflag, ...)
FORWARD_DECL(int, close, int fd)
FORWARD_DECL(ssize_t, read, int fd, void *buf, size_t size)
FORWARD_DECL(ssize_t, write, int fd, void *buf, size_t size)
FORWARD_DECL(ssize_t, pread, int fd, void *buf, size_t size, labstor::off_t offset)
FORWARD_DECL(ssize_t, pwrite, int fd, const void *buf, size_t size, labstor::off_t offset)
FORWARD_DECL(ssize_t, pread64, int fd, void *buf, size_t size, off64_t offset)
FORWARD_DECL(ssize_t, pwrite64, int fd, const void *buf, size_t size, off64_t offset)
FORWARD_DECL(ssize_t, readv, int fd, const struct iovec *iov, int iovcnt)
FORWARD_DECL(ssize_t, preadv, int fd, const struct iovec *iov, int iovcnt, labstor::off_t offset)
FORWARD_DECL(ssize_t, preadv64, int fd, const struct iovec *iov, int iovcnt, off64_t offset)
FORWARD_DECL(ssize_t, preadv2, int fd, const struct iovec *iov, int iovcnt, labstor::off_t offset, int flags)
FORWARD_DECL(ssize_t, preadv64v2, int fd, const struct iovec *iov, int iovcnt, off64_t offset, int flags)
FORWARD_DECL(ssize_t, writev, int fd, const struct iovec *iov, int iovcnt)
FORWARD_DECL(ssize_t, pwritev, int fd, const struct iovec *iov, int iovcnt, labstor::off_t offset)
FORWARD_DECL(ssize_t, pwritev64, int fd, const struct iovec *iov, int iovcnt, off64_t offset)
FORWARD_DECL(ssize_t, pwritev2, int fd, const struct iovec *iov, int iovcnt, labstor::off_t offset, int flags)
FORWARD_DECL(ssize_t, pwritev64v2, int fd, const struct iovec *iov, int iovcnt, off64_t offset, int flags)

#define LABSTOR_GENERIC_POSIX_CLIENT_CLASS labstor::GenericPosix::Client
#define LABSTOR_GENERIC_POSIX_CLIENT_T SINGLETON_T(LABSTOR_GENERIC_POSIX_CLIENT_CLASS)
#define LABSTOR_GENERIC_POSIX_CLIENT_SINGLETON scs::Singleton<LABSTOR_GENERIC_POSIX_CLIENT_CLASS>
#define LABSTOR_GENERIC_POSIX_CLIENT LABSTOR_GENERIC_POSIX_CLIENT_SINGLETON::GetInstance()
DEFINE_SINGLETON(GENERIC_POSIX_CLIENT);
bool initialized_;

/**
 * POSIX FUNCTIONS
 * */

static __attribute__((constructor)) void Init() {
    initialized_ = false;
    GETFUN(int, open, const char *path, int oflag, ...);
    GETFUN(int, close, int fd);
    GETFUN(ssize_t, read, int fd, void *buf, size_t size);
    GETFUN(ssize_t, write, int fd, void *buf, size_t size);
    GETFUN(ssize_t, pread, int fd, void *buf, size_t size, labstor::off_t offset);
    GETFUN(ssize_t, pwrite, int fd, const void *buf, size_t size, labstor::off_t offset);
    GETFUN(ssize_t, pread64, int fd, void *buf, size_t size, off64_t offset);
    GETFUN(ssize_t, pwrite64, int fd, const void *buf, size_t size, off64_t offset);
    GETFUN(ssize_t, readv, int fd, const struct iovec *iov, int iovcnt);
    GETFUN(ssize_t, preadv, int fd, const struct iovec *iov, int iovcnt, labstor::off_t offset);
    GETFUN(ssize_t, preadv64, int fd, const struct iovec *iov, int iovcnt, off64_t offset);
    GETFUN(ssize_t, preadv2, int fd, const struct iovec *iov, int iovcnt, labstor::off_t offset, int flags);
    GETFUN(ssize_t, preadv64v2, int fd, const struct iovec *iov, int iovcnt, off64_t offset, int flags);
    GETFUN(ssize_t, writev, int fd, const struct iovec *iov, int iovcnt);
    GETFUN(ssize_t, pwritev, int fd, const struct iovec *iov, int iovcnt, labstor::off_t offset);
    GETFUN(ssize_t, pwritev64, int fd, const struct iovec *iov, int iovcnt, off64_t offset);
    GETFUN(ssize_t, pwritev2, int fd, const struct iovec *iov, int iovcnt, labstor::off_t offset, int flags);
    GETFUN(ssize_t, pwritev64v2, int fd, const struct iovec *iov, int iovcnt, off64_t offset, int flags);
    LABSTOR_GENERIC_POSIX_CLIENT;
    initialized_ = true;
}

int WRAPPER_FUN(open)(const char *path, int oflag, ...) {
    AUTO_TRACE("")
    int fd = LABSTOR_GENERIC_FS_PATH_NOT_FOUND;
    mode_t mode = 0;
    va_list args;
    va_start(args, oflag);
    if(oflag & O_CREAT) {
        mode = va_arg(args, mode_t);
    }
    va_end(args);
    if(initialized_) {
        fd = LABSTOR_GENERIC_POSIX_CLIENT->Open(path, oflag);
    }
    if(fd == LABSTOR_GENERIC_FS_PATH_NOT_FOUND) {
        fd = REAL_FUN(open)(path, oflag, mode);
    }
    return fd;
}

int WRAPPER_FUN(close)(int fd) {
    AUTO_TRACE("")
    int ret = LABSTOR_GENERIC_FS_INVALID_FD;
    if(initialized_) {
        ret = LABSTOR_GENERIC_POSIX_CLIENT->Close(fd);
    }
    if(ret == LABSTOR_GENERIC_FS_INVALID_FD) {
        ret = REAL_FUN(close)(fd);
    }
    return ret;
}

ssize_t WRAPPER_FUN(read)(int fd, void *buf, size_t size) {
    AUTO_TRACE("")
    ssize_t ret_size = LABSTOR_GENERIC_FS_INVALID_FD;
    if(initialized_) {
        ret_size = LABSTOR_GENERIC_POSIX_CLIENT->Read(fd, buf, size);
    }
    if(ret_size == LABSTOR_GENERIC_FS_INVALID_FD) {
        ret_size = REAL_FUN(read)(fd, buf, size);
    }
    return ret_size;
}

ssize_t WRAPPER_FUN(write)(int fd, void *buf, size_t size) {
    AUTO_TRACE("")
    ssize_t ret_size = LABSTOR_GENERIC_FS_INVALID_FD;
    if(initialized_) {
        ret_size = LABSTOR_GENERIC_POSIX_CLIENT->Write(fd, buf, size);
    }
    if(ret_size == LABSTOR_GENERIC_FS_INVALID_FD) {
        ret_size = REAL_FUN(write)(fd, buf, size);
    }
    return ret_size;
}

ssize_t WRAPPER_FUN(pread)(int fd, void *buf, size_t size, labstor::off_t offset)
{
    return REAL_FUN(pread)(fd, buf, size, offset);
}

ssize_t WRAPPER_FUN(pwrite)(int fd, const void *buf, size_t size, labstor::off_t offset)
{
    return REAL_FUN(pwrite)(fd, buf, size, offset);
}

ssize_t WRAPPER_FUN(pread64)(int fd, void *buf, size_t size, off64_t offset)
{
    return REAL_FUN(pread64)(fd, buf, size, offset);
}

ssize_t WRAPPER_FUN(pwrite64)(int fd, const void *buf, size_t size, off64_t offset)
{
    return REAL_FUN(pwrite64)(fd, buf, size, offset);
}

ssize_t WRAPPER_FUN(readv)(int fd, const struct iovec *iov, int iovcnt)
{
    return REAL_FUN(readv)(fd, iov, iovcnt);
}

ssize_t WRAPPER_FUN(preadv)(int fd, const struct iovec *iov, int iovcnt, labstor::off_t offset)
{
    return REAL_FUN(preadv)(fd, iov, iovcnt, offset);
}

ssize_t WRAPPER_FUN(preadv64)(int fd, const struct iovec *iov, int iovcnt, off64_t offset)
{
    return REAL_FUN(preadv64)(fd, iov, iovcnt, offset);
}

ssize_t WRAPPER_FUN(preadv2)(int fd, const struct iovec *iov, int iovcnt, labstor::off_t offset, int flags)
{
    return REAL_FUN(preadv2)(fd, iov, iovcnt, offset, flags);
}

ssize_t WRAPPER_FUN(preadv64v2)(int fd, const struct iovec *iov, int iovcnt, off64_t offset, int flags)
{
    return REAL_FUN(preadv64v2)(fd, iov, iovcnt, offset, flags);
}

ssize_t WRAPPER_FUN(writev)(int fd, const struct iovec *iov, int iovcnt)
{
    return REAL_FUN(writev)(fd, iov, iovcnt);
}

ssize_t WRAPPER_FUN(pwritev)(int fd, const struct iovec *iov, int iovcnt, labstor::off_t offset)
{
    return REAL_FUN(pwritev)(fd, iov, iovcnt, offset);
}

ssize_t WRAPPER_FUN(pwritev64)(int fd, const struct iovec *iov, int iovcnt, off64_t offset)
{
    return REAL_FUN(pwritev64)(fd, iov, iovcnt, offset);
}

ssize_t WRAPPER_FUN(pwritev2)(int fd, const struct iovec *iov, int iovcnt, labstor::off_t offset, int flags)
{
    return REAL_FUN(pwritev2)(fd, iov, iovcnt, offset, flags);
}

ssize_t WRAPPER_FUN(pwritev64v2)(int fd, const struct iovec *iov, int iovcnt, off64_t offset, int flags)
{
    return REAL_FUN(pwritev64v2)(fd, iov, iovcnt, offset, flags);
}