
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

#ifndef LABSTOR_SOCKET_H
#define LABSTOR_SOCKET_H

#include <labstor/userspace/util/errors.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>

namespace labstor {

class UnixSocket {
private:
    int fd_;
public:
    UnixSocket() {}
    UnixSocket(int fd) { SetFd(fd); }

    inline void SetFd(int fd) {
        fd_ = fd;
        fcntl(fd_, F_SETFL, fcntl(fd_, F_GETFL, 0) | O_NONBLOCK);
    }

    inline int GetFd() { return fd_; }

    bool RecvMSG(void *buf, int size, bool do_wait = true) {
        int cnt, net = 0;
        cnt = safe_recv(buf, size, 0);
        if (!do_wait && cnt == 0) { return false; }
        net += cnt;
        while (net < size) {
            net += safe_recv((char *) buf + net, size - net, 0);
        }
        return true;
    }

    bool RecvMSGPeek(void *buf, int size, bool do_wait = true) {
        int cnt, net = 0;
        cnt = safe_recv(buf, size, MSG_PEEK);
        if (!do_wait && cnt == 0) { return false; }
        net = cnt;
        while (net < size) {
            net = safe_recv((char *) buf + net, size - net, MSG_PEEK);
        }
        return true;
    }

    bool SendMSG(void *buf, int size, bool do_wait = true) {
        int cnt, net = 0;
        cnt = safe_send(buf, size, 0);
        if (!do_wait && cnt == 0) { return false; }
        net += cnt;
        while (net < size) {
            net += safe_send((char *) buf + net, size - net, 0);
        }
        return true;
    }
private:
    //NOTE: if ret == 0, means socket is closed
    inline int safe_recv(void *buf, int size, int flags) {
        int ret = recv(fd_, buf, size, flags);
        if (ret > 0) { return ret; }
        if (errno == EAGAIN || errno == EWOULDBLOCK) { return 0; }
        throw labstor::UNIX_RECVMSG_FAILED.format(strerror(errno));
    }

    inline int safe_send(void *buf, int size, int flags) {
        int ret = send(fd_, buf, size, flags);
        if (ret > 0) { return ret;  }
        if (errno == EAGAIN || errno == EWOULDBLOCK) { return 0; }
        throw labstor::UNIX_SENDMSG_FAILED.format(strerror(errno));
    }
};

}

#endif //LABSTOR_SOCKET_H