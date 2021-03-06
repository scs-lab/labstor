
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <memory>

#include <labstor/kernel/client/kernel_client.h>

static int sockfd;
static struct sockaddr_nl my_addr = {0};
static struct sockaddr_nl kern_addr = {0};

bool labstor::Kernel::NetlinkClient::Connect()
{
    sockfd = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_USER);

    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.nl_family = AF_NETLINK;
    my_addr.nl_pid = getpid();

    memset(&kern_addr, 0, sizeof(kern_addr));
    kern_addr.nl_family = AF_NETLINK;
    kern_addr.nl_pid = 0;

    bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr));
    return true;
}

bool labstor::Kernel::NetlinkClient::SendMSG(void *serialized_buf, size_t buf_size) {
    int num_io_rqs = 0;
    struct nlmsghdr *nlh;
    socklen_t addrlen = sizeof(struct sockaddr_nl);
    int ret;
    void *rq;

    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(buf_size));
    memset(nlh, 0, NLMSG_SPACE(buf_size));
    nlh->nlmsg_len = NLMSG_SPACE(buf_size);
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;

    rq = NLMSG_DATA(nlh);
    memcpy(rq, serialized_buf, buf_size);

    ret = sendto(sockfd, (void*)nlh, NLMSG_SPACE(buf_size), 0, (struct sockaddr *)&kern_addr, addrlen);
    if(ret < 0) {
        perror("Unable to send message to kernel module\n");
        free(nlh);
        return false;
    }
    free(nlh);
    return true;
}

bool labstor::Kernel::NetlinkClient::RecvMSG(void *buf, size_t buf_size) {
    int num_io_rqs = 0;
    struct nlmsghdr *nlh;
    socklen_t addrlen = sizeof(struct sockaddr_nl);
    int ret;
    void *rq;

    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(buf_size));
    memset(nlh, 0, NLMSG_SPACE(buf_size));
    nlh->nlmsg_len = NLMSG_SPACE(buf_size);
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;

    ret = recvfrom(sockfd, (void*)nlh, NLMSG_SPACE(buf_size), 0, (struct sockaddr *)&kern_addr, &addrlen);
    if(ret < 0) {
        perror("Unable to recv count from kernel module\n");
        free(nlh);
        return false;
    }
    rq = NLMSG_DATA(nlh);
    memcpy(buf, rq, buf_size);
    free(nlh);
    return true;
}