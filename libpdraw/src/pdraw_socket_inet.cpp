/**
 * Parrot Drones Awesome Video Viewer Library
 * INET Socket implementation
 *
 * Copyright (c) 2016 Aurelien Barre
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "pdraw_socket_inet.hpp"
#include "pdraw_log.hpp"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <futils/futils.h>
#include <string>

namespace Pdraw {


InetSocket::InetSocket(
	const std::string& localAddress,
	int localPort,
	const std::string& remoteAddress,
	int remotePort,
	struct pomp_loop *loop,
	pomp_fd_event_cb_t fdCb,
	void *userdata)
{
	int res = 0;

	mLocalAddressStr = localAddress;
	mLocalPort = localPort;
	mRemoteAddressStr = remoteAddress;
	mRemotePort = remotePort;
	mLoop = loop;
	mFd = -1;
	mFdCb = fdCb;
	mUserdata = userdata;
	memset(&mLocalAddress, 0, sizeof(mLocalAddress));
	memset(&mRemoteAddress, 0, sizeof(mRemoteAddress));
	mRxBuffer = NULL;
	mRxBufferSize = 0;

	/* Create socket */
	mFd = socket(AF_INET, SOCK_DGRAM, 0);
	if (mFd < 0) {
		res = -errno;
		PDRAW_LOG_ERRNO("socket", -res);
		goto error;
	}

	/* Setup flags */
	res = fd_set_close_on_exec(mFd);
	if (res < 0) {
		PDRAW_LOG_FD_ERRNO("fd_set_close_on_exec", mFd, -res);
		goto error;
	}
	res = fd_add_flags(mFd, O_NONBLOCK);
	if (res < 0) {
		PDRAW_LOG_FD_ERRNO("fd_add_flags", mFd, -res);
		goto error;
	}

	/* Setup rx and tx address */
	mLocalAddress.sin_family = AF_INET;
	inet_aton(mLocalAddressStr.c_str(), &mLocalAddress.sin_addr);
	mLocalAddress.sin_port = htons(mLocalPort);
	if ((!mRemoteAddressStr.empty()) && (mRemotePort != 0)) {
		mRemoteAddress.sin_family = AF_INET;
		inet_aton(mRemoteAddressStr.c_str(), &mRemoteAddress.sin_addr);
		mRemoteAddress.sin_port = htons(mRemotePort);
	}

	/* Bind to rx address */
	if (bind(mFd, (const struct sockaddr *)&mLocalAddress,
		sizeof(mLocalAddress)) < 0) {
		res = -errno;
		PDRAW_LOG_FD_ERRNO("bind", mFd, -res);
		goto error;
	}

	/* Get the real rx port */
	if (mLocalPort == 0) {
		struct sockaddr_in addr;
		socklen_t addrlen = sizeof(addr);
		res = getsockname(mFd, (struct sockaddr *)&addr, &addrlen);
		if (res < 0) {
			PDRAW_LOG_FD_ERRNO("getsockname", mFd, -res);
			goto error;
		}
		mLocalPort = ntohs(addr.sin_port);
	}

	/* Create buffer */
	mRxBufferSize = 65536;
	mRxBuffer = malloc(mRxBufferSize);
	if (mRxBuffer == NULL) {
		res = -ENOMEM;
		goto error;
	}

	/* Start monitoring for input */
	res = pomp_loop_add(mLoop, mFd, POMP_FD_EVENT_IN, mFdCb, mUserdata);
	if (res < 0) {
		PDRAW_LOG_FD_ERRNO("pomp_loop_add", mFd, -res);
		goto error;
	}

	/* Success */
	return;

	/* Cleanup in case of error */
error:
	if (mFd >= 0) {
		if (pomp_loop_has_fd(mLoop, mFd))
			pomp_loop_remove(mLoop, mFd);
		close(mFd);
		mFd = -1;
	}
	free(mRxBuffer);
	mRxBuffer = NULL;
}


InetSocket::~InetSocket(
	void)
{
	int res = 0;

	if (mFd >= 0) {
		if (pomp_loop_has_fd(mLoop, mFd)) {
			res = pomp_loop_remove(mLoop, mFd);
			if (res < 0)
				PDRAW_LOG_ERRNO("pomp_loop_remove", -res);
		}
		close(mFd);
	}

	free(mRxBuffer);
	mFd = -1;
}


int InetSocket::setRxBufferSize(
	size_t size)
{
	int res = 0;
	int _size = size;
	void *rxbuf = NULL;

	if (setsockopt(mFd, SOL_SOCKET,
		SO_RCVBUF, &_size, sizeof(_size)) < 0) {
		res = -errno;
		PDRAW_LOG_FD_ERRNO("setsockopt:SO_RCVBUF", mFd, -res);
		return res;
	}

	/* Setup rx buffer accordingly */
	rxbuf = realloc(mRxBuffer, size);
	if (rxbuf == NULL)
		return -ENOMEM;
	mRxBufferSize = size;
	mRxBuffer = rxbuf;

	return 0;
}


int InetSocket::setTxBufferSize(
	size_t size)
{
	int res = 0;
	int _size = size;

	if (setsockopt(mFd, SOL_SOCKET,
		SO_SNDBUF, &_size, sizeof(_size)) < 0) {
		res = -errno;
		PDRAW_LOG_FD_ERRNO("setsockopt:SO_RCVBUF", mFd, -res);
	}

	return res;
}


int InetSocket::setClass(
	int cls)
{
	int res = 0;
	int _cls = cls;

	if (setsockopt(mFd, IPPROTO_IP,
		IP_TOS, &_cls, sizeof(_cls)) < 0) {
		res = -errno;
		PDRAW_LOG_FD_ERRNO("setsockopt:IP_TOS", mFd, -res);
	}

	return res;
}


ssize_t InetSocket::read(
	void)
{
	ssize_t readlen = 0;
	struct sockaddr_in srcaddr;
	socklen_t addrlen = sizeof(srcaddr);
	memset(&srcaddr, 0, sizeof(srcaddr));

	/* Read data, ignoring interrupts */
	do {
		readlen = recvfrom(mFd, mRxBuffer, mRxBufferSize,
			0, (struct sockaddr *)&srcaddr, &addrlen);
	} while ((readlen < 0) && (errno == EINTR));

	if (readlen < 0) {
		readlen = -errno;
		if (errno != EAGAIN)
			PDRAW_LOG_FD_ERRNO("recvfrom", mFd, errno);
	}

	if ((readlen >= 0) && (mRemoteAddress.sin_port == 0)) {
		mRemoteAddress = srcaddr;
	}

	return readlen;
}


ssize_t InetSocket::write(
	const void *buf,
	size_t len)
{
	ssize_t writelen = 0;

	/* Write ignoring interrupts */
	do {
		writelen = sendto(mFd, buf, len, 0,
			(struct sockaddr *)&mRemoteAddress,
			sizeof(mRemoteAddress));
	} while ((writelen < 0) && (errno == EINTR));

	if (writelen >= 0) {
		if ((size_t)writelen != len) {
			PDRAW_LOGW("partial write on fd=%d (%u/%u)", mFd,
				(unsigned int)writelen, (unsigned int)len);
		}
	} else {
		writelen = -errno;
		if (errno != EAGAIN)
			PDRAW_LOG_FD_ERRNO("sendto", mFd, errno);
	}

	return writelen;
}

} /* namespace Pdraw */