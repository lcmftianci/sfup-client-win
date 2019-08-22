//#ifndef _VIDEO_PUSHER_H__
//#define _VIDEO_PUSHER_H__
//
//#include "talk/media/base/videocapturer.h"
//#include "talk/media/base/screencastid.h"
//#include "talk/media/base/videocapturerfactory.h"
//#include "talk/media/base/videocommon.h"
//#include "webrtc/base/scoped_ptr.h"
//#include "webrtc/base/thread.h"
//#include "webrtc/base/asyncinvoker.h"
//#include "webrtc/modules/desktop_capture/screen_capturer.h"
//#include "webrtc/modules/desktop_capture/desktop_frame.h"
//
//#define USE_OPENCV
//
//#ifdef USE_OPENCV
//#include <opencv2/opencv.hpp>
//#include <opencv2/core.hpp>
//#include <opencv2/highgui.hpp>
//#endif
//
//namespace cricket {
//
//	class VideoPusher : public VideoCapturer, webrtc::DesktopCapturer::Callback
//	{
//	public:
//		VideoPusher();
//		virtual ~VideoPusher();
//
//		void ResetSupportedFormats(const std::vector<cricket::VideoFormat>& formats) {
//			SetSupportedFormats(formats);
//		}
//
//		// Override virtual methods of parent class VideoCapturer.
//		virtual cricket::CaptureState Start(const cricket::VideoFormat& format);
//		virtual void Stop();
//		virtual bool IsRunning();
//		virtual void SetScreencast(bool is_screencast);
//		virtual bool IsScreencast() const;
//		virtual bool GetPreferredFourccs(std::vector<uint32>* fourccs);
//		// DesktopCapturer::Callback interface.
//		virtual webrtc::SharedMemory* CreateSharedMemory(size_t size);
//
//		virtual void OnCaptureCompleted(webrtc::DesktopFrame* frame);
//
//#ifdef USE_OPENCV
//		virtual void OnOpenCvCamture(IplImage* pImg);
//#else
//#endif
//
//		// implements the MessageHandler interface
//		virtual void OnMessage(rtc::Message* msg);
//	protected:
//		void CaptureFrame();
//
//	private:
//
//		bool running_;
//		int64 initial_unix_timestamp_;
//		int64 next_timestamp_;
//		bool is_screencast_;
//		webrtc::VideoRotation rotation_;
//
//		cricket::ScreencastId screencastId_;
//		rtc::scoped_ptr<webrtc::DesktopFrame> desktop_frame;
//		// 		webrtc::DesktopCapturer::SourceList desktop_screens;
//		rtc::scoped_ptr<webrtc::ScreenCapturer> screen_capturer;
//
//#ifdef USE_OPENCV
//		cv::VideoCapture cvc;
//#endif
//	};
//}
//
//#endif