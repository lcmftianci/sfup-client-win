#include "stdafx.h"
#include "BtVideoDialog.h"
#include <thread>
#include <mutex>

void BtVideoReader(std::string strUrl)
{

}

DuiLib::btVideoDialog::btVideoDialog(unsigned int id)
{

}

DuiLib::btVideoDialog::~btVideoDialog()
{
	m_td->join();
}

int DuiLib::btVideoDialog::OpenVideo(std::string strUrl)
{
	m_td = new thread(BtVideoReader, strUrl);
	//m_td->detach();
	return 0;
}

void DuiLib::btVideoDialog::DoInit()
{
	
}

void DuiLib::btVideoDialog::RenderImage(const BITMAPINFO& bmi, const unsigned int *image)
{

}

