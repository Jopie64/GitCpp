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
#pragma comment(lib, "git2-0.lib")

namespace Git
{
using namespace std;

CGitException::CGitException(int P_iErrorCode, const char* P_szDoingPtr)
:	m_iErrorCode(P_iErrorCode),
	std::runtime_error(JStd::String::Format("Git error code %d received during %s. %s", P_iErrorCode, P_szDoingPtr, git_strerror(P_iErrorCode)))
{
}

void ThrowIfError(int P_iGitReturnCode, const char* P_szDoingPtr)
{
	if(P_iGitReturnCode != 0)
		throw CGitException(P_iGitReturnCode, P_szDoingPtr);
}

bool IsGitDir(const wchar_t* path)
{
//	_stat dummy;
//	if(_wstat(JStd::String::Format(L"%s\\HEAD", path).c_str(), &dummy) != 0)
//		return false;

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

COid COid::FromHexString(const char* hexStr)
{
	COid ret;
	ThrowIfError(git_oid_fromstr(&ret.m_oid, hexStr), "git_oid_fromstr()");
	return ret;
}

COid::COid()
{
}

COid::COid(const char* hexStr)
{
	*this = FromHexString(hexStr);
}

COid& COid::operator=(const char* hexStr)
{
	*this = FromHexString(hexStr);
	return *this;
}


std::ostream& operator<<(std::ostream& str, const COid& oid)
{
	char out[40];
	git_oid_fmt(out,&oid.m_oid);
	str.write(out,40);
	return str;
}

COdb::COdb(git_odb* odb)
:	m_odb(odb)
{
}

CObjType::CObjType(git_otype otype)
:	m_otype(otype)
{
}

CRawObject::CRawObject(git_odb_object* obj)
:	m_obj(obj)
{
}


CRawObject::CRawObject()
:	m_obj(NULL)
{
}

CRawObject::~CRawObject()
{
	Close();
}

void CRawObject::Attach(git_odb_object* obj)
{
	Close();
	m_obj = obj;
}

void CRawObject::Close()
{
	if(!IsValid())
		return;
	git_odb_object_close(m_obj);
	m_obj = NULL;
}

bool CRawObject::IsValid() const
{
	return m_obj != NULL;
}

void CRawObject::CheckValid() const
{
	if(!IsValid())
		throw std::runtime_error("Object not valid");
}

const char* CRawObject::Data()const
{
	CheckValid();
	return (const char*)git_odb_object_data(m_obj);
}

size_t CRawObject::Size() const
{
	CheckValid();
	return git_odb_object_size(m_obj);
}

CObjType CRawObject::Type()const
{
	CheckValid();
	return git_odb_object_type(m_obj);
}

ostream& operator<<(ostream& P_os, const CRawObject& P_Obj)
{
	return P_os.write(P_Obj.Data(), P_Obj.Size());
}

const char* CObjType::AsString() const
{
	return git_object_type2string(m_otype);
}

std::ostream& operator<<(std::ostream& str, const CObjType& ot)
{
	return str << ot.AsString();
}

void COdb::Read(CRawObject& obj, const COid& oid)
{
	git_odb_object* pobj = NULL;
	ThrowIfError(git_odb_read(&pobj, m_odb, &oid.m_oid), "git_odb_read()");
	obj.Attach(pobj);
}

COid COdb::Write(const CObjType& ot, const void* data, size_t size)
{
	COid ret;
	ThrowIfError(git_odb_write(&ret.m_oid, m_odb, data, size, ot.Get()), "git_odb_write()");
	return ret;
}

CRepo::CRepo(const wchar_t* path)
:	m_pRepo(NULL)
{
	ThrowIfError(git_repository_open(&m_pRepo, JStd::String::ToMult(path, CP_UTF8).c_str()), "git_repository_open()");
}

CRepo::~CRepo()
{
	git_repository_free(m_pRepo);
}



std::wstring CRepo::GetWPath(git_repository_pathid id) const
{
	return JStd::String::ToWide(GetPath(id), CP_UTF8);
}

std::string CRepo::GetPath(git_repository_pathid id) const
{
	const char* pRet = git_repository_path(m_pRepo, id);
	if(pRet == NULL)
		throw CGitException(0, "git_repository_path()");
	return pRet;
}


CRef CRepo::GetRef(const wchar_t* refName)
{
	ifstream file((GetWPath() + L"\\" + refName).c_str());
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
	ifstream file((GetWPath() + L"\\packed-refs").c_str());
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
	for(JStd::CDirIterator refDir((GetWPath() + L"/" + subPath + L"/*").c_str()); refDir; ++refDir)
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


bool CRepo::Open(CRawObject& obj,const wchar_t* basePath)
{


	return false;
}

COdb CRepo::Odb()
{
	return git_repository_database(m_pRepo);
}



}

