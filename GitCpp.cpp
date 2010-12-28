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
	CRef(){}
	CRef(const char* refHash){Set(refHash);}
	CRef(const std::string& refHash){Set(refHash);}

	void Set(const char* refHash)
	{
		m_Hash = refHash;
		JStd::String::TrimRight(m_Hash, "\r\n\t ");
	}
	void Set(const std::string& refHash){Set(refHash.c_str());}

	bool IsSymbolic() const
	{
		return strncmp(m_Hash.c_str(), "ref: ", 5) == 0;
	}

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
			std::string refReturn;
			getline(file, refReturn);
			return CRef(refReturn);
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
		string ref;
		string refName;
		while(getline(file, ref, ' '))
		{
			if(!getline(file, refName))
				break; //Should not occur. When it does, just ignore
			JStd::String::TrimRight(refName, "\r\n");
			refMap[refName] = ref;
		}
	}

	void	EnsureNotSymbolic(CRef& ref)
	{
		while(ref.IsSymbolic())
			ref = GetRef(JStd::String::ToWide(ref.m_Hash.substr(5), CP_UTF8).c_str());
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

//	Git::CRef ref = repoTest.GetRef(L"HEAD");
	Git::CRef ref = repoTest.GetRef(L"refs/heads/test");

	repoTest.EnsureNotSymbolic(ref);

	cout << ref.m_Hash << endl;




	char c;
	cin >> c;
	return 0;
}

