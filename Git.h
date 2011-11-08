#pragma once
#include <vector>
#include <list>
#include <string>
#include <iostream>
//#include "jstd/Sha1.h"
#include <functional>
#include <memory>
#include "git2.h"

namespace Git
{

class CRepo;

typedef std::vector<std::string> StringVector;

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
	CLibGitObjWrapper():m_obj(NULL), m_isOwner(false)			{}
	CLibGitObjWrapper(T_GitObj* obj, bool makeOwner = true):m_obj(obj), m_isOwner(makeOwner)	{}
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
	void		OpenGlobal();
	void		Open(CRepo& repo);

	bool		BoolVal(const char* name) const;
	std::string	StringVal(const char* name) const;
	int			IntVal(const char* name) const;

	void		BoolVal(const char* name, bool val);
	void		StringVal(const char* name, const char* val);
	void		IntVal(const char* name, int val);


	template<class T>
	struct CVal
	{
		CVal():m_config(NULL){}
		void Init(CConfig* config, const char* name){ m_config=config; m_name = name; }
	protected:
		CConfig*	Config() const { if(!m_config) throw std::logic_error("CConfig::CVal::Config() CVal was not initialized."); return m_config; }
		CConfig*	m_config;
		std::string m_name;
	};

	template<>
	struct CVal<std::string> : CVal<void>
	{
		const std::string& operator*()const { m_val = Config()->StringVal(m_name.c_str()); return m_val; }
		const std::string* operator->()const{ m_val = Config()->StringVal(m_name.c_str()); return &m_val; }
		void operator =(const std::string& val){ Config()->StringVal(m_name.c_str(), val.c_str()); }
		void operator =(const char* val){ Config()->StringVal(m_name.c_str(), val); }
		mutable std::string m_val;
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

	std::string Name() const ;
	COid		Oid(bool forceResolve = false) const ;
	bool		IsSymbolic() const;
	void		Resolve();

};

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

std::ostream& operator<<(std::ostream& P_os, const CRawObject& P_Obj);

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

typedef std::vector<std::tr1::shared_ptr<CCommit> > VectorCommit;

class CTreeEntry : public CLibGitCopyableObjWrapper<const git_tree_entry>
{
public:
	CTreeEntry();
	CTreeEntry(const git_tree_entry* entry);

	COid			Oid() const;
	std::string		Name() const;
	unsigned int	Attributes() const;
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
	size_t		Size()const;
};

class CTreeBuilder : public CObjectTempl<git_treebuilder, &git_treebuilder_free>
{
public:
	CTreeBuilder();
	CTreeBuilder(const CTree* source);
	CTreeBuilder(const CTree& source);

	void		Reset(const CTree* source = NULL);
	void		Clear();
	CTreeEntry	Insert(const wchar_t* filename, const COid& id, unsigned int attributes = 0100644);
};


class CTreeNode
{
public:
	typedef std::list<CTreeNode> list_t;

	CTreeNode();
	CTreeNode(const std::string& name);
	virtual ~CTreeNode();

	CTreeNode*	GetByPath(const char* name, bool createIfNotExist = true);
	void		Insert(const char* name, COid oid, int attributes = 0100644);
	bool		Delete(const char* name);

	COid		Write(CRepo& repo);
	int			GetAttributes()const;
	bool		IsFile()const;

	std::string m_name;
	COid		m_oid;
	list_t		m_subTree; //when this one is not empty, its not a leaf
	int			m_attributes;
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

typedef std::tr1::function<void (const char*)> T_forEachRefCallback;
class CRepo : public CLibGitObjWrapper<git_repository, &git_repository_free>
{
public:
	CRepo();
	CRepo(const wchar_t* path);
	virtual ~CRepo();

	void			Open(const wchar_t* path);
	void			Create(const wchar_t* path, bool isBare);

	static std::wstring	DiscoverPath(const wchar_t* startPath, bool acrossFs = false, const wchar_t* ceilingDirs = NULL);

	void			Read(CCommit& obj,		const COid& oid);
	void			Read(CTree& obj,		const COid& oid);
	void			Read(CBlob& obj,		const COid& oid);
	COid			WriteBlob(const void* data, size_t size);
	COid			WriteBlob(const std::string& data);
	COid			Write(CTreeBuilder& tree);

	std::wstring	GetWPath(git_repository_pathid id = GIT_REPO_PATH) const;
	std::string		GetPath(git_repository_pathid id = GIT_REPO_PATH) const;
	COdb			Odb();

	bool			IsBare()const;

	void			GetReferences(StringVector& refs, unsigned int flags) const;
	void			ForEachRef(const T_forEachRefCallback& callback, unsigned int flags) const;

	VectorCommit	ToCommits(const COids& oids);

	CRef			GetRef(const char* name) const;
	CRef			MakeRef(const char* name, const COid& oid, bool force = false);

	COid			Commit(const char* updateRef, const CSignature& author, const CSignature& committer, const char* msg, const COid& tree, const COids& parents);
	COid			Commit(const char* updateRef, const CSignature& author, const CSignature& committer, const char* msg, const CTree& tree, const VectorCommit& parents);

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

