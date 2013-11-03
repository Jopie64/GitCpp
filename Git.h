#pragma once
#include <vector>
#include <list>
#include <string>
#include <iostream>
//#include "jstd/Sha1.h"
#include <functional>
#include <memory>
#include "libgit2\include\git2.h"

namespace Git
{
using std::string;

class CRepo;

typedef std::vector<string> StringVector;

class CGitException : public std::runtime_error
{
public:
	CGitException(int errorCode, const char* P_szDoingPtr);
	CGitException(const char* P_szDoingPtr);
	int m_errorCode;
	string m_errorText;
	string m_doing;
};
void ThrowIfError(int P_iGitReturnCode, const char* P_szDoingPtr);

template<class T_GitObj, void (*CloseFunc)(T_GitObj*) >
class CLibGitObjWrapper
{
public:
	CLibGitObjWrapper():m_obj(NULL), m_isOwner(false)			{}
	CLibGitObjWrapper(T_GitObj* obj, bool makeOwner = true):m_obj(obj), m_isOwner(makeOwner)	{}
	CLibGitObjWrapper(CLibGitObjWrapper&& that):m_obj(NULL), m_isOwner(false) { using std::swap; swap(m_obj, that.m_obj); swap(m_isOwner, that.m_isOwner); }
	CLibGitObjWrapper& operator=(CLibGitObjWrapper&& that) { using std::swap; swap(m_obj, that.m_obj); swap(m_isOwner, that.m_isOwner); return *this; }
	virtual ~CLibGitObjWrapper()								{ Close(); }

	void		Attach(T_GitObj* obj, bool makeOwner = true)	{ Close(); m_obj = obj; m_isOwner = makeOwner; }
	void		Close()											{ if(!IsValid()) return; if(m_isOwner && CloseFunc) CloseFunc(m_obj); m_obj = NULL; m_isOwner = false; }
	bool		IsValid() const									{ return m_obj != NULL; }
	void		CheckValid() const								{ if(!IsValid()) 
		throw std::runtime_error("libgit object not valid"); }
	T_GitObj*	GetInternalObj() const							{ CheckValid(); return m_obj; }

	typedef bool (CLibGitObjWrapper::*T_IsValidFuncPtr)() const;
	operator T_IsValidFuncPtr () const {return IsValid() ? &CLibGitObjWrapper::IsValid : NULL;}

private:
	CLibGitObjWrapper(const CLibGitObjWrapper&); //Libgit objects are not copyable
	CLibGitObjWrapper& operator=(const CLibGitObjWrapper&);

protected:
	T_GitObj*	m_obj;
	bool		m_isOwner;
};

template<class T_GitObj>
class CLibGitCopyableObjWrapper : public CLibGitObjWrapper<T_GitObj, NULL>
{
public:
	CLibGitCopyableObjWrapper(){}
	CLibGitCopyableObjWrapper(T_GitObj* obj):CLibGitObjWrapper(obj, false){}

	CLibGitCopyableObjWrapper(const CLibGitCopyableObjWrapper& obj):CLibGitObjWrapper(obj.IsValid() ? obj.GetInternalObj() : NULL, false){}
	CLibGitCopyableObjWrapper& operator=(const CLibGitCopyableObjWrapper& obj){ if(obj.IsValid()) Attach(obj.GetInternalObj(), false); return *this; }

	void		Attach(T_GitObj* obj){ CLibGitObjWrapper::Attach(obj, false); }
};

class CConfig : public CLibGitObjWrapper<git_config, git_config_free>
{
public:
	CConfig(){}
	void		Open(const wchar_t* file);
	void		Open(const char* file);
	void		OpenDefault();
	void		Open(CRepo& repo);

	bool		BoolVal(const char* name) const;
	string	StringVal(const char* name) const;
	int			IntVal(const char* name) const;
	long long	Int64Val(const char* name) const;

	void		BoolVal(const char* name, bool val);
	void		StringVal(const char* name, const char* val);
	void		IntVal(const char* name, int val);
	void		Int64Val(const char* name, long long val);


	template<class T>
	struct CVal
	{
		CVal():m_config(NULL){}
		void Init(CConfig* config, const char* name){ m_config=config; m_name = name; }
	protected:
		CConfig*	Config() const { if(!m_config) throw std::logic_error("CConfig::CVal::Config() CVal was not initialized."); return m_config; }
		CConfig*	m_config;
		string m_name;
	};

	template<>
	struct CVal<string> : CVal<void>
	{
		const string& operator*()const { m_val = Config()->StringVal(m_name.c_str()); return m_val; }
		const string* operator->()const{ m_val = Config()->StringVal(m_name.c_str()); return &m_val; }
		void operator =(const string& val){ Config()->StringVal(m_name.c_str(), val.c_str()); }
		void operator =(const char* val){ Config()->StringVal(m_name.c_str(), val); }
		mutable string m_val;
	};

	template<>
	struct CVal<bool> : CVal<void>
	{
		bool operator*()const{ return Config()->BoolVal(m_name.c_str()); }
		void operator =(bool val){ Config()->BoolVal(m_name.c_str(), val); }
	};

	template<>
	struct CVal<int> : CVal<void>
	{
		int operator*()const { return Config()->IntVal(m_name.c_str()); }
		void operator =(int val){ Config()->IntVal(m_name.c_str(), val); }
	};


	template<class T>
	CVal<T>		Val(const char* name){ CVal<T> val; val.Init(this, name); return val; }

};

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

	int compare(const COid& that) const;
	bool operator<(const COid& that) const;
	bool operator==(const COid& that) const;
	bool operator!=(const COid& that) const;

	bool isNull() const;
private:
	git_oid m_oid;
};

std::ostream& operator<<(std::ostream& str, const COid& oid);

class COids
{
public:
	typedef std::vector<COid> VectorCOid;

	COids();
	COids(const COid& oid);

	COids& Add(const COid& oid);
	COids& operator<<(const COid& oid);


	VectorCOid m_oids;
};

std::ostream& operator<<(std::ostream& str, const COid& oid);

class CRef : public CLibGitCopyableObjWrapper<git_reference>//Is copyable because pointer is owned by the repo.
{
public:
	CRef(){}
	CRef(git_reference* ref):CLibGitCopyableObjWrapper(ref){}
	CRef(const CRepo& repo, const char* name);

	string  Name() const ;
	COid	Oid(bool forceResolve = false) const ;
	bool	IsSymbolic() const;
	void	Resolve();

};

typedef std::vector<CRef> RefVector;

class CSignature : public CLibGitObjWrapper<git_signature, &git_signature_free>
{
public:
	CSignature(){}
	CSignature(CRepo& repo);
	CSignature(const char* name, const char* email);
	CSignature(const char* name, const char* email, git_time_t time, int offset);

	void Create(const char* name, const char* email);

};


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
void CloseWithObjectClose(T_GitObj* obj)	{ git_object_free((git_object*)obj); }

template<class T_GitObj, void (*CloseFunc)(T_GitObj*) >
class CObjectTempl : public CLibGitObjWrapper<T_GitObj, CloseFunc>, public CObject
{
public:
	CObjectTempl(){}
	CObjectTempl(T_GitObj* obj):CLibGitObjWrapper(obj){}
};

class CRawObject : public CObjectTempl<git_odb_object, &git_odb_object_free>
{
public:
	CRawObject();
	CRawObject(git_odb_object* obj);
	CRawObject(CRepo& repo, const COid& oid);

	const char* Data() const;
	size_t		Size() const;
	CObjType	Type() const;
};

std::ostream& operator<<(std::ostream& P_os, const CRawObject& P_Obj);

class CObjectBase : public CObjectTempl<git_object, &git_object_free>
{
public:
	CObjectBase();
	CObjectBase(git_object* obj);
	CObjectBase(CRepo& repo, const COid& oid, git_otype type = GIT_OBJ_ANY);

	COid	Oid() const;
};

class CCommit : public CObjectTempl<git_commit, &CloseWithObjectClose<git_commit> >
{
public:
	CCommit();
	CCommit(git_commit* obj);
	CCommit(CRepo& repo, const COid& oid);
	virtual ~CCommit();

	string				Message() const;
//	string				MessageShort() const;
	const git_signature*	Author() const;
	const git_signature*	Committer() const;
	time_t					Time() const;
	COid					Tree() const;
};

typedef std::vector<std::tr1::shared_ptr<CCommit> > VectorCommit;

class CTreeEntry : public CLibGitCopyableObjWrapper<const git_tree_entry>
{
public:
	CTreeEntry();
	CTreeEntry(const git_tree_entry* entry);

	COid			Oid() const;
	string			Name() const;
	git_filemode_t	FileMode() const;
	bool			IsFile() const;

};

class CTree : public CObjectTempl<git_tree, &CloseWithObjectClose<git_tree> >
{
public:
	CTree();
	CTree(CRepo& repo, const COid& oid);

	size_t		EntryCount() const;
	CTreeEntry	Entry(size_t i) const;
	CTreeEntry	Entry(const char* name) const;
	COid		ID() const;

};

class CBlob : public CObjectTempl<git_blob, &CloseWithObjectClose<git_blob> >
{
public:
	CBlob();
	CBlob(CRepo& repo, const COid& oid);
	const void* Content()const;
	git_off_t	Size()const;
};

class CTreeBuilder : public CObjectTempl<git_treebuilder, &git_treebuilder_free>
{
public:
	CTreeBuilder();
	CTreeBuilder(const CTree* source);
	CTreeBuilder(const CTree& source);

	void		Reset(const CTree* source = NULL);
	void		Clear();
	CTreeEntry	Insert(const wchar_t* filename, const COid& id, git_filemode_t attributes = GIT_FILEMODE_BLOB);
};


class CTreeNode
{
public:
	typedef std::list<CTreeNode> list_t;

	CTreeNode();
	CTreeNode(const string& name);
	virtual ~CTreeNode();

	CTreeNode*	GetByPath(const char* name, bool createIfNotExist = true);
	void		Insert(const char* name, COid oid, git_filemode_t attributes = GIT_FILEMODE_BLOB);
	bool		Delete(const char* name);

	COid		Write(CRepo& repo);
	git_filemode_t			GetAttributes()const;
	bool		IsFile()const;

	string m_name;
	COid		m_oid;
	list_t		m_subTree; //when this one is not empty, its not a leaf
	git_filemode_t	m_attributes;
};

class COdb
{
public:
	COdb(git_odb* odb);

	void Read(CRawObject& obj,	const COid& oid);
	COid Write(const CObjType& ot, const void* data, size_t size);

private:

	git_odb* m_odb;
};

class CRemote : public CLibGitObjWrapper<git_remote, &git_remote_free>
{
public:
	CRemote();
	CRemote(CRepo& repo, const char* name);

	void Connect(git_direction direction = GIT_DIRECTION_FETCH);
	bool IsConnected() const;
	void Download();
};

typedef std::tr1::function<void (CRef&)> T_forEachRefCallback;
class CRepo : public CLibGitObjWrapper<git_repository, &git_repository_free>
{
public:
	CRepo();
	CRepo(const wchar_t* path);
	virtual ~CRepo();

	void			Open(const wchar_t* path);
	void			Create(const wchar_t* path, bool isBare);

	static std::wstring	DiscoverPath(const wchar_t* startPath, bool acrossFs = false, const wchar_t* ceilingDirs = NULL);

	void			Read(CObjectBase& obj,	const COid& oid, git_otype type = GIT_OBJ_ANY);
	void			Read(CCommit& obj,		const COid& oid);
	void			Read(CTree& obj,		const COid& oid);
	void			Read(CBlob& obj,		const COid& oid);
	COid			WriteBlob(const void* data, size_t size);
	COid			WriteBlob(const string& data);
	COid			Write(CTreeBuilder& tree);

	std::wstring	GetWPath() const;
	string		GetPath() const;
	COdb			Odb();

	bool			IsBare()const;

	void			GetReferences(RefVector& refs) const;
	void			ForEachRef(const T_forEachRefCallback& callback) const;

	VectorCommit	ToCommits(const COids& oids);

	CRef			GetRef(const char* name) const;
	CRef			MakeRef(const char* name, const COid& oid, bool force = false);

	COid			Commit(const char* updateRef, const CSignature& author, const CSignature& committer, const char* msg, const COid& tree, const COids& parents);
	COid			Commit(const char* updateRef, const CSignature& author, const CSignature& committer, const char* msg, const CTree& tree, const VectorCommit& parents);

	COid			CreateTag(const char* name, const CObjectBase& target, bool force);
	COid			CreateTag(const char* name, const COid& target, bool force);

	CTreeEntry		TreeFind(const CTree& start, const char* path);
	CTreeEntry		TreeFind(const COid& treeStart, const char* path);
	void			BuildTreeNode(CTreeNode& node, const CTree& tree);
	void			BuildTreeNode(CTreeNode& node, const COid& tree);

	void			DefaultSig(CSignature& sig);
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

