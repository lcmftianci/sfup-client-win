/*
* OneToManyProcessor.h
*/

#ifndef ERIZO_SRC_ERIZO_ONETOMANYPROCESSOR_H_
#define ERIZO_SRC_ERIZO_ONETOMANYPROCESSOR_H_

#include <map>
#include <string>

#include "MediaDefinitions.h"
#include "Logger.h"
#include <Media.h>

namespace erizo {

class WebRtcConnection;

/**
* Represents a One to Many connection.
* Receives media from one publisher and retransmits it to every subscriber.
*/
class MEDIA_API OneToManyProcessor : public MediaSink, public FeedbackSink {
  DECLARE_LOGGER();

 public:
  std::map<std::string, boost::shared_ptr<MediaSink> > subscribers;
  boost::shared_ptr<MediaSource> publisher;

  OneToManyProcessor();
  virtual ~OneToManyProcessor();
  /**
  * Sets the Publisher
  * @param webRtcConn The WebRtcConnection of the Publisher
  */
  void setPublisher(MediaSource* webRtcConn);
  /**
  * Sets the subscriber
  * @param webRtcConn The WebRtcConnection of the subscriber
  * @param peerId An unique Id for the subscriber
  */
  void addSubscriber(MediaSink* webRtcConn, const std::string& peerId);
  /**
  * Eliminates the subscriber given its peer id
  * @param peerId the peerId
  */
  void removeSubscriber(const std::string& peerId);

 private:
  typedef boost::shared_ptr<MediaSink> sink_ptr;
  FeedbackSink* feedbackSink_;

  int deliverAudioData_(char* buf, int len);
  int deliverVideoData_(char* buf, int len);
  int deliverFeedback_(char* buf, int len);
  void closeAll();
};

}  // namespace erizo
#endif  // ERIZO_SRC_ERIZO_ONETOMANYPROCESSOR_H_
