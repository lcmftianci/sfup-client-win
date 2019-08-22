/*
 * OneToManyProcessor.cpp
 */

#include "OneToManyProcessor.h"

#include <map>
#include <string>

#include "./WebRtcConnection.h"
#include "rtp/RtpHeaders.h"

namespace erizo {
  DEFINE_LOGGER(OneToManyProcessor, "OneToManyProcessor");
  OneToManyProcessor::OneToManyProcessor() {
    ELOG_DEBUG("OneToManyProcessor constructor");
    feedbackSink_ = NULL;
  }

  OneToManyProcessor::~OneToManyProcessor() {
    ELOG_DEBUG("OneToManyProcessor destructor");
    this->closeAll();
  }

  int OneToManyProcessor::deliverAudioData_(char* buf, int len) {
    // ELOG_DEBUG("OneToManyProcessor deliverAudio");
    if (len <= 0)
      return 0;

    boost::unique_lock<boost::mutex> lock(myMonitor_);
    if (subscribers.empty())
      return 0;

    std::map<std::string, sink_ptr>::iterator it;
    for (it = subscribers.begin(); it != subscribers.end(); ++it) {
      (*it).second->deliverAudioData(buf, len);
    }

    return 0;
  }

  int OneToManyProcessor::deliverVideoData_(char* buf, int len) {
    if (len <= 0)
      return 0;
    RtcpHeader* head = reinterpret_cast<RtcpHeader*>(buf);
    if (head->isFeedback()) {
      ELOG_WARN("Receiving Feedback in wrong path: %d", head->packettype);
      if (feedbackSink_ != NULL) {
        head->ssrc = htonl(publisher->getVideoSourceSSRC());
        feedbackSink_->deliverFeedback(buf, len);
      }
      return 0;
    }
    boost::unique_lock<boost::mutex> lock(myMonitor_);
    if (subscribers.empty())
      return 0;
    std::map<std::string, sink_ptr>::iterator it;
    for (it = subscribers.begin(); it != subscribers.end(); ++it) {
      if ((*it).second != NULL) {
        (*it).second->deliverVideoData(buf, len);
      }
    }
    return 0;
  }

  void OneToManyProcessor::setPublisher(MediaSource* webRtcConn) {
    boost::mutex::scoped_lock lock(myMonitor_);
    this->publisher.reset(webRtcConn);
    feedbackSink_ = publisher->getFeedbackSink();
  }

  int OneToManyProcessor::deliverFeedback_(char* buf, int len) {
    if (feedbackSink_ != NULL) {
      feedbackSink_->deliverFeedback(buf, len);
    }
    return 0;
  }

  void OneToManyProcessor::addSubscriber(MediaSink* webRtcConn,
      const std::string& peerId) {
    ELOG_DEBUG("Adding subscriber");
    boost::mutex::scoped_lock lock(myMonitor_);
    ELOG_DEBUG("From %u, %u ", publisher->getAudioSourceSSRC(), publisher->getVideoSourceSSRC());
    webRtcConn->setAudioSinkSSRC(this->publisher->getAudioSourceSSRC());
    webRtcConn->setVideoSinkSSRC(this->publisher->getVideoSourceSSRC());
    ELOG_DEBUG("Subscribers ssrcs: Audio %u, video, %u from %u, %u ",
               webRtcConn->getAudioSinkSSRC(), webRtcConn->getVideoSinkSSRC(),
               this->publisher->getAudioSourceSSRC() , this->publisher->getVideoSourceSSRC());
    FeedbackSource* fbsource = webRtcConn->getFeedbackSource();

    if (fbsource != NULL) {
      ELOG_DEBUG("adding fbsource");
      fbsource->setFeedbackSink(this);
    }
    if (this->subscribers.find(peerId) != subscribers.end()) {
        ELOG_WARN("This OTM already has a subscriber with peerId %s, substituting it", peerId.c_str());
        this->subscribers.erase(peerId);
    }
    this->subscribers[peerId] = sink_ptr(webRtcConn);
  }

  void OneToManyProcessor::removeSubscriber(const std::string& peerId) {
    ELOG_DEBUG("Remove subscriber %s", peerId.c_str());
    boost::mutex::scoped_lock lock(myMonitor_);
    if (this->subscribers.find(peerId) != subscribers.end()) {
      this->subscribers.erase(peerId);
    }
  }

  void OneToManyProcessor::closeAll() {
    ELOG_DEBUG("OneToManyProcessor closeAll");
    feedbackSink_ = NULL;
    publisher.reset();
    boost::unique_lock<boost::mutex> lock(myMonitor_);
    std::map<std::string, boost::shared_ptr<MediaSink> >::iterator it = subscribers.begin();
    while (it != subscribers.end()) {
      if ((*it).second != NULL) {
        FeedbackSource* fbsource = (*it).second->getFeedbackSource();
        if (fbsource != NULL) {
          fbsource->setFeedbackSink(NULL);
        }
      }
      subscribers.erase(it++);
    }
    subscribers.clear();
    ELOG_DEBUG("ClosedAll media in this OneToMany");
  }

}  // namespace erizo
