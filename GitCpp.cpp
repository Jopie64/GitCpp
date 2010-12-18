// GitCpp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string>

namespace Git
{

class CRepo
{
public:
	CRepo(const wchar_t* P_szPathPtr);

	std::wstring m_Path;
}

CRepo::CRepo(const wchar_t* path)
:	m_Path(path)
{
	
}

}


int _tmain(int argc, _TCHAR* argv[])
{
	return 0;
}

