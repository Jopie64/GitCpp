#pragma once
#include <map>
#include <string>
#include <iostream>
#include "jstd/Sha1.h"
#include "git2.h"

namespace Git
{

class CRepo;

class CGitException : public std::runtime_error
{
public:
	CGitException(int P_iErrorCode, const char* P_szDoingPtr);
	int			m_iErrorCode;
	std::string m_csDoing;
};
void ThrowIfError(int P_iGitReturnCode, const char* P_szDoingPtr);

template<class T_GitObj, void (*CloseFunc)(T_GitObj*) >
class CLibGitObjWrapper
{
public:
	CLibGitObjWrapper():m_obj(NULL)				{}
	CLibGitObjWrapper(T_GitObj* obj):m_obj(obj)	{}
	virtual ~CLibGitObjWrapper()				{ Close(); }

	void		Attach(T_GitObj* obj)			{ Close(); m_obj = obj; }
	void		Close()							{ if(!IsValid()) return; CloseFunc(m_obj); m_obj = NULL; }
	bool		IsValid() const					{ return m_obj != NULL; }
	void		CheckValid() const				{ if(!IsValid()) throw std::runtime_error("libgit object not valid"); }
	T_GitObj*	GetInternalObj() const			{ CheckValid(); return m_obj; }

	typedef bool (CLibGitObjWrapper::*T_IsValidFuncPtr)() const;
	operator T_IsValidFuncPtr () const {return IsValid() ? &CLibGitObjWrapper::IsValid : NULL;}

private:
	CLibGitObjWrapper(const CLibGitObjWrapper&); //Libgit objects are not copyable
	CLibGitObjWrapper& operator=(const CLibGitObjWrapper&);

protected:
	T_GitObj* m_obj;
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


class COid
{
friend std::ostream& operator<<(std::ostream& str, const COid& oid);
friend class COdb;
friend CRepo;

public:
	COid();
	COid(const char* hexStr);
	COid(const git_oid& P_oid):m_oid(P_oid){}

	static COid FromHexString(const char* hexStr);
	COid& operator=(const char* hexStr);
	git_oid& GetInternalObj(){ return  m_oid; }
	const git_oid& GetInternalObj() const { return  m_oid; }
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
class CObjectTempl : public CLibGitObjWrapper<T_GitObj, CloseFunc>, public CObject
{
public:
	CObjectTempl(){}
	CObjectTempl(T_GitObj* obj):CLibGitObjWrapper(obj){}
};

class CRawObject : public CObjectTempl<git_odb_object, &git_odb_object_close>
{
public:
	CRawObject();
	CRawObject(git_odb_object* obj);
	CRawObject(CRepo& repo, const COid& oid);

	const char* Data() const;
	size_t		Size() const;
	CObjType	Type() const;


};

class CCommit : public CObjectTempl<git_commit, &CloseWithObjectClose<git_commit> >
{
public:
	CCommit();
	CCommit(git_commit* obj);
	CCommit(CRepo& repo, const COid& oid);
	virtual ~CCommit();

	std::string				Message() const;
	std::string				MessageShort() const;
	const git_signature*	Author() const;
	const git_signature*	Committer() const;
	time_t					Time() const;
	COid					Tree() const;
};

std::ostream& operator<<(std::ostream& P_os, const CRawObject& P_Obj);

class COdb
{
public:
	COdb(git_odb* odb);

	void Read(CRawObject& obj,	const COid& oid);
	COid Write(const CObjType& ot, const void* data, size_t size);

private:

	git_odb* m_odb;
};

class CRepo : public CLibGitObjWrapper<git_repository, &git_repository_free>
{
public:
	CRepo(const wchar_t* P_szPathPtr);
	virtual ~CRepo();

	static std::wstring	DiscoverPath(const wchar_t* startPath, bool acrossFs = false, const wchar_t* ceilingDirs = NULL);

	CRef			GetRef(const wchar_t* refName);
	void			LoadPackedRefs(MapRef& refMap);
	void			LoadFileRefs(MapRef& refMap, const wchar_t* subPath = NULL);
	void			LoadRefs(MapRef& refMap);
	void			EnsureNotSymbolic(CRef& ref);
	void			Read(CCommit& obj,		const COid& oid);
	std::wstring	GetWPath(git_repository_pathid id = GIT_REPO_PATH) const;
	std::string		GetPath(git_repository_pathid id = GIT_REPO_PATH) const;
	COdb			Odb();

	bool			IsBare()const;

private:
	CRepo(const CRepo&); //Non copyable
	CRepo& operator=(const CRepo&);

};

class CCommitWalker : public CLibGitObjWrapper<git_revwalk, &git_revwalk_free>
{
public:
	CCommitWalker();
	CCommitWalker(CRepo& repo);

	void Init(CRepo& repo);
	void Sort(unsigned int sort);
	void Reset();

	void AddRev(const COid& oid);

	bool End() const;

	void Next();

	COid Curr() const;

	operator T_IsValidFuncPtr () const { return IsValid() && !End() ? &CLibGitObjWrapper::IsValid : NULL; }
	CCommitWalker& operator ++() { Next(); return *this; }
	COid operator*() const { return Curr(); }


	virtual ~CCommitWalker();

private:
	CCommitWalker(const CCommitWalker&); //Non copyable
	CCommitWalker& operator=(const CCommitWalker&);

	bool m_nextCalled;
	bool m_end;
	COid m_curr;

};


}

