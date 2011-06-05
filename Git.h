#pragma once
#include <map>
#include <string>
#include <iostream>
#include "jstd/Sha1.h"
#include "git2.h"

namespace Git
{

class CGitException : public std::runtime_error
{
public:
	CGitException(int P_iErrorCode, const char* P_szDoingPtr);
	int			m_iErrorCode;
	std::string m_csDoing;
};

bool IsGitDir(const wchar_t* path);

class CRef
{
public:
	CRef();
	CRef(const char* refHash);
	CRef(const std::string& refHash);

	void Set(const char* refHash);
	void Set(const std::string& refHash);

	bool IsSymbolic() const;
	bool IsValid() const;

	const std::string& Hash() const;

private:

	std::string m_Hash;
};

typedef std::map<std::string, CRef> MapRef;

std::ostream& operator<<(std::ostream& str, const CRef& ref);

class CObject
{
public:
	CObject();
	CObject(const JStd::CSha1Hash& hash);

	JStd::CSha1Hash GetHash()const{return m_Hash;}



	JStd::CSha1Hash m_Hash;
};

class CRepo
{
public:
	CRepo(const wchar_t* P_szPathPtr);

	CRef			GetRef(const wchar_t* refName);
	void			LoadPackedRefs(MapRef& refMap);
	void			LoadFileRefs(MapRef& refMap, const wchar_t* subPath = NULL);
	void			LoadRefs(MapRef& refMap);
	void			EnsureNotSymbolic(CRef& ref);
	bool			Open(CObject& obj, const wchar_t* basePath = NULL);
	std::wstring	GetWPath(git_repository_pathid id = GIT_REPO_PATH) const;
	std::string		GetPath(git_repository_pathid id = GIT_REPO_PATH) const;
	static void		ThrowIfError(int P_iGitReturnCode, const char* P_szDoingPtr);

private:
	CRepo(const CRepo&); //Non copyable
	CRepo& operator=(const CRepo&);

	git_repository* m_pRepo;
};


}

