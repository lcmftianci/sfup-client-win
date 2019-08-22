// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
//
#pragma once

#include <Globals.h>
#include <Common.h>
#include "Media.h"
// TODO:  �ڴ˴����ó�����Ҫ������ͷ�ļ�

#ifdef _WIN32
extern int gettimeofday(struct timeval *tp, void *tzp);
extern int rand_r(unsigned int *seed);
#endif

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/asio.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/cstdint.hpp>


