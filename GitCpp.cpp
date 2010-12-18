// GitCpp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string>
#include "jstd\JStd.h"

namespace Git
{

bool IsGitDir(const wchar_t* path)
{
	_stat dummy;
	if(_wstat(JStd::String::Format(L"%s\\HEAD", path).c_str(), &dummy) != 0)
		return false;

	return true;
}

class CRepo
{
public:
	CRepo(const wchar_t* P_szPathPtr);


	std::wstring m_Path;
};

CRepo::CRepo(const wchar_t* path)
:	m_Path(path)
{
	if(!IsGitDir(m_Path.c_str()))
		m_Path += L"\\.git";
	if(!IsGitDir(m_Path.c_str()))
		throw std::runtime_error("Not a git repository directory.");
		
}

}


int _tmain(int argc, _TCHAR* argv[])
{
	Git::CRepo repoTest(L"d:\\develop\\tortoisegit2");

	return 0;
}

