#ifndef ERIZO_SRC_ERIZO_RTP_RTCPFORWARDER_H_
#define ERIZO_SRC_ERIZO_RTP_RTCPFORWARDER_H_

#include <boost/shared_ptr.hpp>

#include <map>
#include <list>
#include <set>

#include "./logger.h"
#include "./MediaDefinitions.h"
#include "./SdpInfo.h"
#include "rtp/RtpHeaders.h"
#include "rtp/RtcpProcessor.h"

namespace erizo {

class RtcpForwarder: public RtcpProcessor{
  DECLARE_LOGGER();

 public:
  RtcpForwarder(MediaSink* msink, MediaSource* msource, uint32_t maxVideoBw = 300000);
  virtual ~RtcpForwarder() {}
  void addSourceSsrc(uint32_t ssrc);
  void setMaxVideoBW(uint32_t bandwidth);
  void setPublisherBW(uint32_t bandwidth);
  void analyzeSr(RtcpHeader* chead);
  int analyzeFeedback(char* buf, int len);
  void checkRtcpFb();

 private:
  static const int RR_AUDIO_PERIOD = 2000;
  static const int RR_VIDEO_BASE = 800;
  static const int REMB_TIMEOUT = 1000;
  std::map<uint32_t, boost::shared_ptr<RtcpData>> rtcpData_;
  boost::mutex mapLock_;
  uint32_t defaultVideoBw_;
  uint8_t packet_[128];
  int addREMB(char* buf, int len, uint32_t bitrate);
  int addNACK(char* buf, int len, uint16_t seqNum, uint16_t blp, uint32_t sourceSsrc, uint32_t sinkSsrc);
};

}  // namespace erizo

#endif  // ERIZO_SRC_ERIZO_RTP_RTCPFORWARDER_H_
