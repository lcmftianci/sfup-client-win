#ifndef __FUNC_H_
#define __FUNC_H_

#pragma once

extern bool isAllDigit(const string& str);

// �жϲ���ϵͳ�����Ƿ���64λ
extern BOOL OSVerIs64Bit();


//����UUID
extern AString CreateUUID();

extern BOOL GetJarList(CString &strPath, std::vector<CString> &vecJar, std::vector<CString> &vecPath);

extern AString& replace_all(AString& str, const AString& old_value, const AString& new_value);

extern AString& replace_all_distinct(AString& str, const AString& old_value, const AString& new_value);

extern AString UTF8ToGBK(const AString& strUTF8);

extern AString GBKToUTF8(const AString& strGBK);

extern AString GenRoomName(const int &size = 10);

#endif