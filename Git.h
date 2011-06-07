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
void ThrowIfError(int P_iGitReturnCode, const char* P_szDoingPtr);

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


class COid
{
friend std::ostream& operator<<(std::ostream& str, const COid& oid);
friend class COdb;

public:
	COid();
	COid(const char* hexStr);

	static COid FromHexString(const char* hexStr);
	COid& operator=(const char* hexStr);
private:
	git_oid m_oid;
};


std::ostream& operator<<(std::ostream& str, const COid& oid);

class CObjType
{
public:
	CObjType(git_otype otype);
private:
	git_otype m_otype;
};

class CObject
{
public:
	CObject();
	CObject(git_odb_object* obj);
	virtual ~CObject();

	void		Attach(git_odb_object* obj);
	void		Close();
	bool		IsValid() const;
	void		CheckValid() const;
	const char* Data() const;
	CObjType	Type() const;

private:
	CObject(const CObject&);
	CObject& operator=(const CObject&);

	git_odb_object* m_obj;
};


class COdb
{
public:
	COdb(git_odb* odb);

	void Read(CObject& obj, const COid& oid);

private:

	git_odb* m_odb;
};

class CRepo
{
public:
	CRepo(const wchar_t* P_szPathPtr);
	virtual ~CRepo();

	CRef			GetRef(const wchar_t* refName);
	void			LoadPackedRefs(MapRef& refMap);
	void			LoadFileRefs(MapRef& refMap, const wchar_t* subPath = NULL);
	void			LoadRefs(MapRef& refMap);
	void			EnsureNotSymbolic(CRef& ref);
	bool			Open(CObject& obj, const wchar_t* basePath = NULL);
	std::wstring	GetWPath(git_repository_pathid id = GIT_REPO_PATH) const;
	std::string		GetPath(git_repository_pathid id = GIT_REPO_PATH) const;
	COdb			Odb();

private:
	CRepo(const CRepo&); //Non copyable
	CRepo& operator=(const CRepo&);

	git_repository* m_pRepo;
};


}

