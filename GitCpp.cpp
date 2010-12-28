// GitCpp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string>
#include "jstd\JStd.h"
#include <fstream>
#include <map>
#include <windows.h>
#include <iostream>

namespace Git
{
	using namespace std;

bool IsGitDir(const wchar_t* path)
{
	_stat dummy;
	if(_wstat(JStd::String::Format(L"%s\\HEAD", path).c_str(), &dummy) != 0)
		return false;

	return true;
}

class CRef
{
public:
	std::string m_Hash;
};

typedef std::map<std::string, CRef> MapRef;
class CRepo
{
public:
	CRepo(const wchar_t* P_szPathPtr);

	CRef	GetRef(const wchar_t* refName)
	{
		ifstream file((m_Path + L"\\" + refName).c_str());
		if(file)
		{
			CRef refReturn;
			getline(file, refReturn.m_Hash);
			return refReturn;
		}

		string multRefName = JStd::String::ToMult(refName, CP_UTF8);
		MapRef refMap;
		LoadPackedRefs(refMap);
		CRef refReturn = refMap[multRefName];
		if(refReturn.m_Hash.empty())
			throw std::runtime_error(JStd::String::Format("Ref \'%s\' not found.",multRefName.c_str()));
		return refReturn;		
	}

	void	LoadPackedRefs(MapRef& refMap)
	{
		ifstream file((m_Path + L"\\packed-refs").c_str());
		CRef ref;
		std::string refName;
		while(getline(file, ref.m_Hash, ' '))
		{
			if(!getline(file, refName))
				break; //Should not occur. When it does, just ignore
			refMap[refName] = ref;
		}
	}




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
	using namespace std;

	Git::CRepo repoTest(L"d:\\develop\\tortoisegit2");

	Git::CRef ref = repoTest.GetRef(L"HEAD");

	cout << ref.m_Hash << endl;


	char c;
	cin >> c;
	return 0;
}

