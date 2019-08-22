#ifndef _BT_VIDEO_DIALOG_HPP
#define _BT_VIDEO_DIALOG_HPP

#include "interface.h"
#include <UIContainer.h>
#include <wingdi.h>

namespace DuiLib {
	class btVideoDialog : public CContainerUI, public Renderer
	{
	public:
		btVideoDialog(unsigned int id);
		virtual ~btVideoDialog();
		virtual int OpenVideo(std::string strUrl) = 0;

	protected:
		virtual void DoInit();
		virtual void RenderImage(const BITMAPINFO& bmi, const unsigned int  *image);

	private:
		CControlUI *m_pVideo = nullptr;
		unsigned int m_nId;
		thread *m_td;
	};
}




#endif // _BT_VIDEO_DIALOG_HPP