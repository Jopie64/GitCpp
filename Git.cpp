#include "stdafx.h"
#include "Git.h"
#include "jstd\JStd.h"
#include "jstd\Sha1.h"
#include <fstream>
#include <windows.h>
#include "jstd\DirIterator.h"
#include <vector>
#include <algorithm>

#pragma comment(lib, "Ws2_32.lib")

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

CRef::CRef()
{
}

CRef::CRef(const char* refHash)
{
	Set(refHash);
}

CRef::CRef(const std::string& refHash)
{
	Set(refHash);
}

void CRef::Set(const char* refHash)
{
	m_Hash = refHash;
	JStd::String::TrimRight(m_Hash, "\r\n\t ");
}
void CRef::Set(const std::string& refHash){Set(refHash.c_str());}

bool CRef::IsSymbolic() const
{
	return strncmp(m_Hash.c_str(), "ref: ", 5) == 0;
}

bool CRef::IsValid() const
{
	return !m_Hash.empty();
}


const std::string& CRef::Hash()const
{
	return m_Hash;
}


std::ostream& operator<<(std::ostream& str, const CRef& ref)
{
	str << ref.Hash();
	return str;
}



CObject::CObject()
{
}

CObject::CObject(const JStd::CSha1Hash& hash)
:	m_Hash(hash)
{
}



CRepo::CRepo(const wchar_t* path)
:	m_Path(path)
{
	if(!IsGitDir(m_Path.c_str()))
		m_Path += L"\\.git";
	if(!IsGitDir(m_Path.c_str()))
		throw std::runtime_error("Not a git repository directory.");
		
}


CRef CRepo::GetRef(const wchar_t* refName)
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
	if(!refReturn.IsValid())
		throw std::runtime_error(JStd::String::Format("Ref \'%s\' not found.",multRefName.c_str()));
	return refReturn;
}

void CRepo::LoadPackedRefs(MapRef& refMap)
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

void CRepo::LoadFileRefs(MapRef& refMap, const wchar_t* subPath)
{
	if(subPath == NULL)
		subPath = L"refs";
	for(JStd::CDirIterator refDir((m_Path + L"/" + subPath + L"/*").c_str()); refDir; ++refDir)
	{
		std::wstring refPart = wstring(subPath) + L"/" + refDir.File().name;
		if(refDir.IsDirectory())
		{
			if (wcscmp(refDir.File().name, L".") == 0 || 
				wcscmp(refDir.File().name, L"..") == 0)
				continue;
			LoadFileRefs(refMap, refPart.c_str());
			continue;
		}
		refMap[JStd::String::ToMult(refPart.c_str(), CP_UTF8)] = GetRef(refPart.c_str());
	}
}

void CRepo::LoadRefs(MapRef& refMap)
{
	LoadPackedRefs(refMap);
	LoadFileRefs(refMap);
}

void CRepo::EnsureNotSymbolic(CRef& ref)
{
	while(ref.IsSymbolic())
		ref = GetRef(JStd::String::ToWide(ref.Hash().substr(5), CP_UTF8).c_str());
}

struct idxFileHeader
{
	int version1;
	int version2;

	int fanout[256];
};

typedef vector<JStd::CSha1Hash> vectorSha1;


struct packFileHeader
{
	char	pack[4];
	int		version;
	int		entries;
	char	something;

};



bool CRepo::Open(CObject& obj,const wchar_t* basePath)
{


	return false;
}



}

