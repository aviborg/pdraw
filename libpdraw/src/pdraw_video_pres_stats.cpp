/**
 * Parrot Drones Awesome Video Viewer Library
 * Video presentation statistics
 *
 * Copyright (c) 2018 Parrot Drones SAS
 * Copyright (c) 2016 Aurelien Barre
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the copyright holders nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define ULOG_TAG pdraw_video_pres_stats
#include <ulog.h>
ULOG_DECLARE_TAG(ULOG_TAG);

#include <errno.h>

#include "pdraw_video_pres_stats.hpp"

namespace Pdraw {


VideoPresStats::VideoPresStats(void) :
		timestamp(0), presentationFrameCount(0),
		presentationTimestampDeltaIntegral(0),
		presentationTimestampDeltaIntegralSq(0),
		presentationTimingErrorIntegral(0),
		presentationTimingErrorIntegralSq(0),
		presentationEstimatedLatencyIntegral(0),
		presentationEstimatedLatencyIntegralSq(0),
		playerLatencyIntegral(0), playerLatencyIntegralSq(0),
		estimatedLatencyPrecisionIntegral(0)
{
}


int VideoPresStats::writeMsg(struct pomp_msg *msg, uint32_t msgid)
{
	ULOG_ERRNO_RETURN_ERR_IF(msg == nullptr, EINVAL);

	return pomp_msg_write(msg,
			      msgid,
			      "%" PRIu64 "%" PRIu32 "%" PRIu64 "%" PRIu64
			      "%" PRIu64 "%" PRIu64 "%" PRIu64 "%" PRIu64
			      "%" PRIu64 "%" PRIu64 "%" PRIu64,
			      timestamp,
			      presentationFrameCount,
			      presentationTimestampDeltaIntegral,
			      presentationTimestampDeltaIntegralSq,
			      presentationTimingErrorIntegral,
			      presentationTimingErrorIntegralSq,
			      presentationEstimatedLatencyIntegral,
			      presentationEstimatedLatencyIntegralSq,
			      playerLatencyIntegral,
			      playerLatencyIntegralSq,
			      estimatedLatencyPrecisionIntegral);
}


int VideoPresStats::readMsg(const struct pomp_msg *msg)
{
	ULOG_ERRNO_RETURN_ERR_IF(msg == nullptr, EINVAL);

	return pomp_msg_read(msg,
			     "%" PRIu64 "%" PRIu32 "%" PRIu64 "%" PRIu64
			     "%" PRIu64 "%" PRIu64 "%" PRIu64 "%" PRIu64
			     "%" PRIu64 "%" PRIu64 "%" PRIu64,
			     &timestamp,
			     &presentationFrameCount,
			     &presentationTimestampDeltaIntegral,
			     &presentationTimestampDeltaIntegralSq,
			     &presentationTimingErrorIntegral,
			     &presentationTimingErrorIntegralSq,
			     &presentationEstimatedLatencyIntegral,
			     &presentationEstimatedLatencyIntegralSq,
			     &playerLatencyIntegral,
			     &playerLatencyIntegralSq,
			     &estimatedLatencyPrecisionIntegral);
}

} /* namespace Pdraw */
