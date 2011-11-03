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



COid COid::FromHexString(const char* hexStr)
{
	COid ret;
	ThrowIfError(git_oid_fromstr(&ret.m_oid, hexStr), "git_oid_fromstr()");
	return ret;
}

COid::COid()
{
	memset(&m_oid, 0, sizeof(m_oid));
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

int COid::compare(const COid& that) const
{
	return memcmp(&m_oid, &that.m_oid, sizeof(m_oid));
}

bool COid::operator<(const COid& that) const
{
	return compare(that) < 0;
}

bool COid::operator==(const COid& that) const
{
	return compare(that) == 0;
}

bool COid::isNull() const
{
	return compare(COid()) == 0;
}


std::ostream& operator<<(std::ostream& str, const COid& oid)
{
	char out[40];
	git_oid_fmt(out,&oid.m_oid);
	str.write(out,40);
	return str;
}

COids::COids()
{
}

COids::COids(const COid& oid)
{
	Add(oid);
}

COids& COids::Add(const COid& oid)
{
	m_oids.push_back(oid);
	return *this;
}

COids& COids::operator<<(const COid& oid)
{
	return Add(oid);
}

std::string CRef::Name() const
{
	CheckValid(); 
	return git_reference_name(GetInternalObj());
}

COid CRef::Oid(bool forceResolve) const
{
	CheckValid();
	CRef refToUse(*this);
	if(forceResolve)
		refToUse.Resolve();

	const git_oid* oid = git_reference_oid(refToUse.GetInternalObj());
	if(oid == NULL)
		throw std::runtime_error("Reference does not point to a oid. Probably a symbolic ref.");
	return *oid;
}

CRef::CRef(const CRepo& repo, const char* name)
{
	git_reference* ref = NULL;
	ThrowIfError(git_reference_lookup(&ref, repo.GetInternalObj(), name), "git_reference_lookup()");
	Attach(ref);
}

bool CRef::IsSymbolic() const
{
	CheckValid();
	return git_reference_type(GetInternalObj()) == GIT_REF_SYMBOLIC;
}

void CRef::Resolve()
{
	CheckValid();
	git_reference* ref = NULL;
	ThrowIfError(git_reference_resolve(&ref, GetInternalObj()), "git_reference_resolve()");
	Attach(ref);
}


CSignature::CSignature(const char* name, const char* email)
{
	Attach(git_signature_now(name, email));
}

CSignature::CSignature(const char* name, const char* email, git_time_t time, int offset)
{
	Attach(git_signature_new(name, email, time, offset));
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
:	CObjectTempl(obj)
{
}


CRawObject::CRawObject(CRepo& repo, const COid& oid)
{
	repo.Odb().Read(*this, oid);
}

CRawObject::CRawObject()
{
}

const char* CRawObject::Data()const
{
	CheckValid();
	return (const char*)git_odb_object_data(GetInternalObj());
}

size_t CRawObject::Size() const
{
	CheckValid();
	return git_odb_object_size(GetInternalObj());
}

CObjType CRawObject::Type()const
{
	CheckValid();
	return git_odb_object_type(GetInternalObj());
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

CCommit::CCommit()
{
}

CCommit::CCommit(git_commit* obj)
:	CObjectTempl(obj)
{
}

CCommit::CCommit(CRepo& repo, const COid& oid)
{
	repo.Read(*this, oid);
}


CCommit::~CCommit(){}

std::string CCommit::Message() const
{
	return git_commit_message(m_obj);
}

std::string CCommit::MessageShort() const
{
	return git_commit_message_short(m_obj);
}

const git_signature* CCommit::Author() const
{
	return git_commit_author(m_obj);
}

const git_signature* CCommit::Committer() const
{
	return git_commit_committer(m_obj);
}

time_t CCommit::Time() const
{
	return git_commit_time(m_obj);
}

COid CCommit::Tree() const
{
	return *git_commit_tree_oid(m_obj);
}


CTreeEntry::CTreeEntry()
{
}

CTreeEntry::CTreeEntry(const git_tree_entry* entry)
:	CLibGitCopyableObjWrapper(entry)
{
}

COid CTreeEntry::Oid() const
{
	return *git_tree_entry_id(GetInternalObj());
}

std::string CTreeEntry::Name() const
{
	return git_tree_entry_name(GetInternalObj());
}

unsigned int CTreeEntry::Attributes() const
{
	return git_tree_entry_attributes(GetInternalObj());
}

bool CTreeEntry::IsFile() const
{
	return git_tree_entry_type(GetInternalObj()) == GIT_OBJ_BLOB;
}


CTree::CTree()
{
}

CTree::CTree(CRepo& repo, const COid& oid)
{
	repo.Read(*this, oid);
}

size_t CTree::EntryCount() const
{
	return git_tree_entrycount(GetInternalObj());
}

CTreeEntry CTree::Entry(size_t i) const
{
	return git_tree_entry_byindex(GetInternalObj(), i);
}

CTreeEntry CTree::Entry(const char* name) const
{
	return git_tree_entry_byname(GetInternalObj(), name);
}


CBlob::CBlob()
{
}

CBlob::CBlob(CRepo& repo, const COid& oid)
{
	repo.Read(*this, oid);
}

const void* CBlob::Content()const
{
	return git_blob_rawcontent(GetInternalObj());
}

size_t CBlob::Size()const
{
	return git_blob_rawsize(GetInternalObj());
}



CTreeBuilder::CTreeBuilder()
{
	Reset();
}

CTreeBuilder::CTreeBuilder(const CTree* source)
{
	Reset(source);
}

CTreeBuilder::CTreeBuilder(const CTree& source)
{
	Reset(&source);
}

void CTreeBuilder::Reset(const CTree* source)
{
	git_treebuilder* builder = NULL;
	ThrowIfError(git_treebuilder_create(&builder, source ? source->GetInternalObj() : NULL), "git_treebuilder_create");
	Attach(builder);
}

void CTreeBuilder::Clear()
{
	git_treebuilder_clear(GetInternalObj());
}

CTreeEntry CTreeBuilder::Insert(const wchar_t* filename, const COid& id, unsigned int attributes)
{
	git_tree_entry* entry = NULL;
	ThrowIfError(git_treebuilder_insert(&entry, GetInternalObj(), JStd::String::ToMult(filename, CP_UTF8).c_str(), &id.GetInternalObj(), attributes), "git_treebuilder_insert()");
	return CTreeEntry(entry);
}




CTreeNode::CTreeNode()
:	m_attributes(-1)
{
}

CTreeNode::CTreeNode(const std::string& name)
:	m_attributes(-1), m_name(name)
{
}

CTreeNode::~CTreeNode()
{
}

CTreeNode* CTreeNode::GetByPath(const char* name)
{
	const char* nameStart = name;
	if(*nameStart == '/')
		++nameStart;
	const char* nameEnd = strchr(nameStart, '/');
	if(nameEnd == NULL)
		nameEnd = nameStart + strlen(nameStart);
	if(nameEnd == nameStart)
		//Empty name. Its this treenode.
		return this;
	std::string namePart = std::string(nameStart, nameEnd);
	list_t::iterator i;
	for(i = m_subTree.begin(); i != m_subTree.end(); ++i)
		if(i->m_name == namePart)
			break;
	if(i == m_subTree.end())
		i = m_subTree.insert(m_subTree.end(), namePart);
	return i->GetByPath(nameEnd);
}

void CTreeNode::Insert(const char* name, COid oid, int attributes)
{
	CTreeNode* node = GetByPath(name);
	node->m_oid			= oid;
	node->m_attributes	= attributes;
}

bool CTreeNode::Delete(const char* name)
{
	const char* nameStart = name;
	if(*nameStart == '/')
		++nameStart;
	const char* nameEnd = strchr(nameStart, '/');
	if(nameEnd == NULL)
		nameEnd = nameStart + strlen(nameStart);
	if(nameEnd == nameStart)
		//Empty name. Its this treenode.
		return true;
	std::string namePart = std::string(nameStart, nameEnd);
	list_t::iterator i;
	for(i = m_subTree.begin(); i != m_subTree.end(); ++i)
	{
		if(i->m_name != namePart) continue;
		if(!i->Delete(nameEnd)) return false; //Didn't find the node.

		if(*nameEnd == '\0')
			m_subTree.erase(i);//A leaf node. We found it! Erase it.
		return true;
	}
	return false;
}

int CTreeNode::GetAttributes()const
{
	if(m_attributes >= 0)
		return m_attributes;
	if(m_subTree.empty())
		return 0100644;
	return 040000;
}

COid CTreeNode::Write(CRepo& repo)
{
	if(m_subTree.empty())
		return m_oid;
	CTreeBuilder treeB;
	for(list_t::iterator i = m_subTree.begin(); i != m_subTree.end(); ++i)
	{
		COid oid = i->Write(repo);
		if(oid.isNull())
			continue;
		treeB.Insert(JStd::String::ToWide(i->m_name, CP_UTF8).c_str(), oid, i->GetAttributes());
	}
	return repo.Write(treeB);
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

CRepo::CRepo()
{
}

CRepo::CRepo(const wchar_t* path)
{
	Open(path);
}

void CRepo::Open(const wchar_t* path)
{
	git_repository* repo = NULL;
	ThrowIfError(git_repository_open(&repo, JStd::String::ToMult(path, CP_UTF8).c_str()), "git_repository_open()");
	Attach(repo);
}

void CRepo::Create(const wchar_t* path, bool isBare)
{
	git_repository* repo = NULL;
	ThrowIfError(git_repository_init(&repo, JStd::String::ToMult(path, CP_UTF8).c_str(), isBare ? 1 : 0), "git_repository_init()");
	Attach(repo);
}


CRepo::~CRepo()
{
}



std::wstring CRepo::GetWPath(git_repository_pathid id) const
{
	return JStd::String::ToWide(GetPath(id), CP_UTF8);
}

std::string CRepo::GetPath(git_repository_pathid id) const
{
	const char* pRet = git_repository_path(GetInternalObj(), id);
	if(pRet == NULL)
		throw CGitException(0, "git_repository_path()");
	return pRet;
}

std::wstring CRepo::DiscoverPath(const wchar_t* startPath, bool acrossFs, const wchar_t* ceilingDirs)
{
	char result[MAX_PATH];result[sizeof(result) - 1] = '\0';
	ThrowIfError(git_repository_discover(
						result, sizeof(result) - 1,
						JStd::String::ToMult(startPath, CP_UTF8).c_str(),
						acrossFs ? 1 : 0,
						JStd::String::ToMult(ceilingDirs == NULL ? L"" : ceilingDirs, CP_UTF8).c_str()),
				 "git_repository_discover()");
	return JStd::String::ToWide(result, CP_UTF8);
}




void CRepo::Read(CCommit& obj, const COid& oid)
{
	git_commit* pobj = NULL;
	ThrowIfError(git_commit_lookup(&pobj, GetInternalObj(), &oid.m_oid), "git_commit_lookup()");
	obj.Attach(pobj);
}

void CRepo::Read(CTree& obj, const COid& oid)
{
	git_tree* pobj = NULL;
	ThrowIfError(git_tree_lookup(&pobj, GetInternalObj(), &oid.m_oid), "git_tree_lookup()");
	obj.Attach(pobj);
}

void CRepo::Read(CBlob& obj, const COid& oid)
{
	git_blob* pobj = NULL;
	ThrowIfError(git_blob_lookup(&pobj, GetInternalObj(), &oid.m_oid), "git_blob_lookup()");
	obj.Attach(pobj);
}





COdb CRepo::Odb()
{
	return git_repository_database(GetInternalObj());
}

bool CRepo::IsBare()const
{
	CheckValid();
	return git_repository_is_bare(GetInternalObj()) ? true : false;
}

int addRef(const char* name, void* refs)
{
	((StringVector*)refs)->push_back(name);
	return GIT_SUCCESS;
}

void CRepo::GetReferences(StringVector& refs, unsigned int flags) const
{
	ThrowIfError(git_reference_foreach(GetInternalObj(), flags, &addRef, &refs), "git_reference_foreach()");
}

void CRepo::ForEachRef(const T_forEachRefCallback& callback, unsigned int flags) const
{
	struct CCtxt
	{
		CCtxt(const T_forEachRefCallback& callback):m_exceptionThrown(false), m_callback(callback){}

		const T_forEachRefCallback&	m_callback;
		std::string					m_exception;
		bool						m_exceptionThrown;

		static int Call(const char* ref, void* vthis)
		{
			CCtxt* _this = (CCtxt*)vthis;
			try
			{
				_this->m_callback(ref);
			}
			catch(std::exception& e)
			{
				_this->m_exception = e.what();
				_this->m_exceptionThrown = true;
				//Cannot rethrow here... it needs to fall through the c stack.
				//Will be thrown later, but exception is always from type std::exception.
				//It is not possible to template the catch handler to later rethrow the same exception as what was catched here.
				return GIT_ERROR;
			}
			return GIT_SUCCESS;
		}
	} ctxt(callback);

	int error = git_reference_foreach(GetInternalObj(), flags, &CCtxt::Call, &ctxt);
	if(ctxt.m_exceptionThrown)
		throw std::exception(ctxt.m_exception.c_str()); //rethrow exception caught from callback
	ThrowIfError(error, "git_reference_foreach()");
}

COid CRepo::WriteBlob(const void* data, size_t size)
{
	return Odb().Write(GIT_OBJ_BLOB, data, size);
}

COid CRepo::WriteBlob(const std::string& data)
{
	return WriteBlob(data.data(), data.size());
}

COid CRepo::Write(CTreeBuilder& tree)
{
	git_oid oid;
	ThrowIfError(git_treebuilder_write(&oid, GetInternalObj(), tree.GetInternalObj()), "git_treebuilder_write()");
	return oid;
}


VectorCommit CRepo::ToCommits(const COids& oids)
{
	VectorCommit ret;
	ret.reserve(oids.m_oids.size());
	for(COids::VectorCOid::const_iterator i = oids.m_oids.begin(); i != oids.m_oids.end(); ++i)
		ret.push_back(std::tr1::shared_ptr<CCommit>(new CCommit(*this, *i)));
	return ret;
}

CRef CRepo::GetRef(const char* name) const
{
	return CRef(*this, name);
}

CRef CRepo::MakeRef(const char* name, const COid& oid, bool force)
{
	git_reference* ref = NULL;
	ThrowIfError(git_reference_create_oid(&ref, GetInternalObj(), name, &oid.GetInternalObj(), force ? 1 : 0), "git_reference_create_oid()");
	return ref;
}


COid CRepo::Commit(const char* updateRef, const CSignature& author, const CSignature& committer, const char* msg, const COid& tree, const COids& parentIds)
{
	return Commit(updateRef, author, committer, msg, CTree(*this, tree), ToCommits(parentIds));
}

COid CRepo::Commit(const char* updateRef, const CSignature& author, const CSignature& committer, const char* msg, const CTree& tree, const VectorCommit& parents)
{
	std::vector<const git_commit*> rawParents;
	rawParents.reserve(parents.size());
	for(VectorCommit::const_iterator i = parents.begin(); i != parents.end(); ++i)
		rawParents.push_back((*i)->GetInternalObj());

	const git_commit** rawParentsPtr = NULL;
	if(!rawParents.empty())
		rawParentsPtr = &*rawParents.begin();

	COid ret;
	ThrowIfError(git_commit_create(&ret.GetInternalObj(),
									GetInternalObj(),
									updateRef,
									author.GetInternalObj(),
									committer.GetInternalObj(),
									msg,
								    tree.GetInternalObj(),
								    rawParents.size(),
								    rawParentsPtr),
				 "git_commit_create()");
	return ret;
}

CTreeEntry CRepo::TreeFind(const CTree& start, const char* path)
{
	const char* pathStart = path;
	if(*pathStart == '/')
		++pathStart;
	const char* pathEnd = strchr(pathStart, '/');
	if(pathEnd == NULL)
		pathEnd = pathStart + strlen(pathStart);
	if(pathEnd == pathStart)
		//Empty path. Don't use it like this
		throw std::runtime_error("TreeFind() done with invalid path name");

	std::string pathPart(pathStart, pathEnd);
	CTreeEntry entry = start.Entry(pathPart.c_str());

	if(*pathEnd == '/')
		++pathEnd;
	if(*pathEnd == '/0')
		return entry;

	if(!entry.IsValid())
		return entry; //Not found

	if(entry.IsFile())
		throw std::runtime_error(JStd::String::Format("Part %s of %s was a file.", pathPart.c_str(), path));
	return TreeFind(entry.Oid(), pathEnd);
}

CTreeEntry CRepo::TreeFind(const COid& treeStart, const char* path)
{
	return TreeFind(CTree(*this, treeStart), path);
}

void CRepo::BuildTreeNode(CTreeNode& node, const CTree& tree)
{
	node.m_attributes	= 040000;
	node.m_name			= "";
	node.m_oid			= COid();

	size_t entryCount = tree.EntryCount();
	for(size_t i = 0; i < entryCount; ++i)
	{
		CTreeEntry entry = tree.Entry(i);
		if(entry.IsFile())
		{
			node.Insert(entry.Name().c_str(), entry.Oid(), entry.Attributes());
			continue;
		}
		CTree subTree(*this, entry.Oid());
		node.m_subTree.push_back(CTreeNode());
		BuildTreeNode(node.m_subTree.back(), subTree);
		node.m_name			= entry.Name();
		node.m_attributes	= entry.Attributes();
		node.m_oid			= entry.Oid();
	}
}


CCommitWalker::CCommitWalker()
:	m_nextCalled(false)
{
}

CCommitWalker::CCommitWalker(CRepo& repo)
:	m_nextCalled(false)
{
	Init(repo);
}

CCommitWalker::~CCommitWalker()
{
}

void CCommitWalker::Init(CRepo& repo)
{
	git_revwalk* walker;
	ThrowIfError(git_revwalk_new(&walker, repo.GetInternalObj()), "git_revwalk_new()");
	Attach(walker);
	m_nextCalled = false;
}

void CCommitWalker::AddRev(const COid& oid)
{
	ThrowIfError(git_revwalk_push(GetInternalObj(), &oid.GetInternalObj()), "git_revwalk_push()");
	m_nextCalled = false; //git_revwalk_push() resets the walker
}


void CCommitWalker::Sort(unsigned int sort)
{
	git_revwalk_sorting(GetInternalObj(), sort);
}

void CCommitWalker::Reset()
{
	git_revwalk_reset(GetInternalObj());
	m_nextCalled = false; //git_revwalk_push() resets the walker
}

bool CCommitWalker::End() const
{
	if(!m_nextCalled)
		Curr(); //Next will now be called. This will eventually set m_end when the list is empty.
	return m_end;
}

void CCommitWalker::Next()
{
	git_oid oid;
	m_end = false;
	int error;
	switch(error = git_revwalk_next(&oid, GetInternalObj()))
	{
	case GIT_SUCCESS: m_end = false; break;
	case GIT_EREVWALKOVER: m_end = true; break;
	default:
		ThrowIfError(error, "git_revwalk_next()");
	}

	m_curr			= oid;
	m_nextCalled	= true;
}

COid CCommitWalker::Curr() const
{
	if(!m_nextCalled)
		const_cast<CCommitWalker*>(this)->Next();
	if(!*this)
		throw std::runtime_error("Commit walker was already at end when Curr() was called, or it was not initialized.");
	return m_curr;
}



}

