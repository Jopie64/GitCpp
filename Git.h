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
	const char* AsString() const;

	git_otype Get() const {return m_otype;}
private:
	git_otype m_otype;
};

std::ostream& operator<<(std::ostream& str, const CObjType& ot);

class CObject { public: virtual ~CObject() {} };

template<class T_GitObj>
void CloseWithObjectClose(T_GitObj* obj)	{ git_object_close((git_object*)obj); }

template<class T_GitObj, void (*CloseFunc)(T_GitObj*) >
class CObjectTempl : public CObject
{
public:
	CObjectTempl():m_obj(NULL)				{}
	CObjectTempl(T_GitObj* obj):m_obj(obj)	{}
	virtual ~CObjectTempl()					{ Close(); }

	void		Attach(T_GitObj* obj)		{ Close(); m_obj = obj; }
	void		Close()						{ if(!IsValid()) return; CloseFunc(m_obj); m_obj = NULL; }
	bool		IsValid() const				{ return m_obj != NULL; }
	void		CheckValid() const			{ if(!IsValid()) throw std::runtime_error("Object not valid"); }
	T_GitObj*	Obj() const					{ CheckValid(); return m_obj; }

private:
	CObjectTempl(const CObjectTempl&);
	CObjectTempl& operator=(const CObjectTempl&);

	T_GitObj* m_obj;
};


class CRawObject : public CObjectTempl<git_odb_object, &git_odb_object_close>
{
public:
	CRawObject();
	CRawObject(git_odb_object* obj);

	const char* Data() const;
	size_t		Size() const;
	CObjType	Type() const;

private:
	CRawObject(const CRawObject&);
	CRawObject& operator=(const CRawObject&);

};

class CCommit : public CObjectTempl<git_commit, &CloseWithObjectClose<git_commit> >
{
public:
	CCommit();
	CCommit(git_commit* obj);
	virtual ~CCommit();

private:
	CCommit(const CRawObject&);
	CCommit& operator=(const CRawObject&);

	git_commit* m_obj;
};

std::ostream& operator<<(std::ostream& P_os, const CRawObject& P_Obj);

class COdb
{
public:
	COdb(git_odb* odb);

	void Read(CRawObject& obj, const COid& oid);
	COid Write(const CObjType& ot, const void* data, size_t size);

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
	bool			Open(CRawObject& obj, const wchar_t* basePath = NULL);
	std::wstring	GetWPath(git_repository_pathid id = GIT_REPO_PATH) const;
	std::string		GetPath(git_repository_pathid id = GIT_REPO_PATH) const;
	COdb			Odb();

private:
	CRepo(const CRepo&); //Non copyable
	CRepo& operator=(const CRepo&);

	git_repository* m_pRepo;
};


}

