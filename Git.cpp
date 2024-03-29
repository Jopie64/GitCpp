#include "stdafx.h"
#include "Git.h"
#include "jstd/JStd.h"
#include "jstd/Sha1.h"
#include <fstream>
#ifdef WIN32
#include <windows.h>
#include "jstd/DirIterator.h"
#endif
#include <vector>
#include <algorithm>
#include <cstring>

#pragma comment(lib, "Ws2_32.lib")

namespace Git
{
using namespace std;

class CBuf
{
public:
	CBuf()
	{
		memset(&m_buf, 0, sizeof(m_buf));
	}

	string AsString()
	{
		return string(m_buf.ptr, m_buf.size);
	}

	git_buf* H() { return &m_buf; }

	~CBuf()
	{
		git_buf_free(&m_buf);
	}

private:
	git_buf m_buf;
};

CGitException::CGitException(int errorCode, const char* P_szDoingPtr)
:	m_errorCode(errorCode),
	m_doing(P_szDoingPtr),
	std::runtime_error(JStd::String::Format("Git error code %d received during %s", errorCode, P_szDoingPtr))
{
}

CGitException::CGitException(const char* P_szDoingPtr)
:	m_errorCode(giterr_last()->klass),
	m_errorText(giterr_last()->message),
	m_doing(P_szDoingPtr),
	std::runtime_error(JStd::String::Format("Git error code %d received during %s. %s", giterr_last()->klass, P_szDoingPtr, giterr_last()->message))
{
}


void ThrowIfError(int P_iGitReturnCode, const char* P_szDoingPtr)
{
	if(P_iGitReturnCode != 0)
		throw CGitException(P_szDoingPtr); //So it will throw giterr_last()
}

#ifdef WIN32
void CConfig::Open(const wchar_t* file)
{
	Open(JStd::String::ToMult(file, CP_UTF8).c_str());
}
#endif

void CConfig::Open(const char* file)
{
	git_config* cfg = NULL;
	ThrowIfError(git_config_open_ondisk(&cfg, file), "git_config_open_ondisk()");
	Attach(cfg);	
}

void CConfig::OpenDefault()
{
	git_config* cfg = NULL;
	ThrowIfError(git_config_open_default(&cfg), "git_config_open_default()");
	Attach(cfg);	
}

void CConfig::Open(CRepo& repo)
{
	Open((repo.GetPath() + "/" + "config").c_str());
}



bool CConfig::BoolVal(const char* name) const
{
	int val = 0;
	ThrowIfError(git_config_get_bool(&val, GetInternalObj(), name), "git_config_get_bool()");
	return !!val;
}

std::string CConfig::StringVal(const char* name) const
{
	const char* val = NULL;
	ThrowIfError(git_config_get_string(&val, GetInternalObj(), name), "git_config_get_string()");
	return val;
}

int CConfig::IntVal(const char* name) const
{
	int val = 0;
	ThrowIfError(git_config_get_int32(&val, GetInternalObj(), name), "git_config_get_int()");
	return val;
}

long long CConfig::Int64Val(const char* name) const
{
    int64_t val = 0;
	ThrowIfError(git_config_get_int64(&val, GetInternalObj(), name), "git_config_get_int()");
    return (long long)val;
}

void CConfig::BoolVal(const char* name, bool val)
{
	ThrowIfError(git_config_set_bool(GetInternalObj(), name, val ? 1 : 0), "git_config_set_bool()");
}

void CConfig::StringVal(const char* name, const char* val)
{
	ThrowIfError(git_config_set_string(GetInternalObj(), name, val), "git_config_set_string()");
}

void CConfig::IntVal(const char* name, int val)
{
	ThrowIfError(git_config_set_int32(GetInternalObj(), name, val), "git_config_set_int()");
}

void CConfig::Int64Val(const char* name, long long val)
{
	ThrowIfError(git_config_set_int64(GetInternalObj(), name, val), "git_config_set_int()");
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

bool COid::operator!=(const COid& that) const
{
	return compare(that) != 0;
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

	const git_oid* oid = git_reference_target(refToUse.GetInternalObj());
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
	Create(name, email);
}

CSignature::CSignature(const char* name, const char* email, git_time_t time, int offset)
{
	git_signature* out = NULL;
	ThrowIfError(git_signature_new(&out, name, email, time, offset), "git_signature_new");
	Attach(out);
}

void CSignature::Create(const char* name, const char* email)
{
	git_signature* out = NULL;
	ThrowIfError(git_signature_now(&out, name, email), "git_signature_now");
	Attach(out);
}

CSignature::CSignature(CRepo& repo)
{
	repo.DefaultSig(*this);
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

CObjectBase::CObjectBase()
{
}

CObjectBase::CObjectBase(git_object* obj)
:	CObjectTempl(obj)
{
}

CObjectBase::CObjectBase(CRepo& repo, const COid& oid, git_otype type)
{
	repo.Read(*this, oid, type);
}

COid CObjectBase::Oid() const
{
	return *git_object_id(GetInternalObj());
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

//std::string CCommit::MessageShort() const
//{
//	return git_commit_message_short(m_obj);
//}

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
	return *git_commit_tree_id(m_obj);
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

git_filemode_t CTreeEntry::FileMode() const
{
	return git_tree_entry_filemode(GetInternalObj());
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

COid CTree::ID() const
{
	return *git_tree_id(GetInternalObj());
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

git_off_t CBlob::Size()const
{
	return git_blob_rawsize(GetInternalObj());
}



CTreeBuilder::CTreeBuilder(CRepo& repo)
:	m_repo(repo)
{
	Reset();
}

CTreeBuilder::CTreeBuilder(CRepo& repo, const CTree* source)
:	m_repo(repo)
{
	Reset(source);
}

CTreeBuilder::CTreeBuilder(CRepo& repo, const CTree& source)
:	m_repo(repo)
{
	Reset(&source);
}

void CTreeBuilder::Reset(const CTree* source)
{
	git_treebuilder* builder = NULL;
	ThrowIfError(git_treebuilder_new(&builder, m_repo.GetInternalObj(), source ? source->GetInternalObj() : NULL), "git_treebuilder_create");
	Attach(builder);
}

void CTreeBuilder::Clear()
{
	git_treebuilder_clear(GetInternalObj());
}

CTreeEntry CTreeBuilder::Insert(const char* filename, const COid& id, git_filemode_t attributes)
{
	const git_tree_entry* entry = NULL;
    ThrowIfError(git_treebuilder_insert(&entry, GetInternalObj(), filename, &id.GetInternalObj(), attributes), "git_treebuilder_insert()");
	return CTreeEntry(entry);
}

#ifdef WIN32

CTreeEntry CTreeBuilder::Insert(const wchar_t* filename, const COid& id, git_filemode_t attributes)
{
    return Insert(JStd::String::ToMult(filename, CP_UTF8).c_str(), id, attributes);
}

#endif

COid CTreeBuilder::Write()
{
	git_oid oid;
	ThrowIfError(git_treebuilder_write(&oid, GetInternalObj()), "git_treebuilder_write()");
	return oid;
}




CTreeNode::CTreeNode()
:	m_attributes((git_filemode_t)-1)
{
}

CTreeNode::CTreeNode(const std::string& name)
:	m_attributes((git_filemode_t)-1), m_name(name)
{
}

CTreeNode::~CTreeNode()
{
}

CTreeNode* CTreeNode::GetByPath(const char* name, bool createIfNotExist)
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
	{
		if(!createIfNotExist)
			return NULL;
		i = m_subTree.insert(m_subTree.end(), namePart);
	}
	return i->GetByPath(nameEnd);
}

void CTreeNode::Insert(const char* name, COid oid, git_filemode_t attributes)
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

git_filemode_t CTreeNode::GetAttributes()const
{
	if(m_attributes != (git_filemode_t)-1)
		return m_attributes;
	if(m_subTree.empty())
		return GIT_FILEMODE_BLOB;
	return GIT_FILEMODE_TREE;
}

bool CTreeNode::IsFile()const
{
	return !m_oid.isNull() && m_subTree.empty();
}


COid CTreeNode::Write(CRepo& repo)
{
	if(m_subTree.empty())
		return m_oid;
	CTreeBuilder treeB(repo);
	for(list_t::iterator i = m_subTree.begin(); i != m_subTree.end(); ++i)
	{
		COid oid = i->Write(repo);
		if(oid.isNull())
			continue;
        treeB.Insert(i->m_name.c_str(), oid, i->GetAttributes());
	}
	return m_oid = repo.Write(treeB);
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

CRemote::CRemote(CRepo& repo, const char* name)
{
	git_remote* remote = NULL;
	ThrowIfError(git_remote_lookup(&remote, repo.GetInternalObj(), name), "git_remote_lookup()");
	Attach(remote);
}

void CRemote::Connect(git_direction direction)
{
	ThrowIfError(git_remote_connect(GetInternalObj(), direction, NULL, NULL, NULL), "git_remote_connect()");
}

void CRemote::Disconnect()
{
	git_remote_disconnect(GetInternalObj());
}

void CRemote::UpdateTips(const char* refLogMsg)
{
	ThrowIfError(git_remote_update_tips(GetInternalObj(), NULL, 1, GIT_REMOTE_DOWNLOAD_TAGS_UNSPECIFIED, refLogMsg), "git_remote_update_tips()");
}

bool CRemote::IsConnected() const
{
	return git_remote_connected(GetInternalObj()) ? true : false;
}

void CRemote::Download()
{
	if(!IsConnected())
		throw std::runtime_error("Cannot fetch when not connected.");
	ThrowIfError(git_remote_download(GetInternalObj(), NULL, NULL), "git_remote_download()");
}


CRepo::CRepo()
{
}

CRepo::CRepo(const char* path)
{
	Open(path);
}

void CRepo::Open(const char* path)
{
    git_repository* repo = NULL;
    ThrowIfError(git_repository_open(&repo, path), "git_repository_open()");
    Attach(repo);
}

void CRepo::Create(const char* path, bool isBare)
{
    git_repository* repo = NULL;
    ThrowIfError(git_repository_init(&repo, path, isBare ? 1 : 0), "git_repository_init()");
    Attach(repo);
}

#ifdef WIN32
CRepo::CRepo(const wchar_t* path)
{
    Open(path);
}

void CRepo::Open(const wchar_t* path)
{
    Open(JStd::String::ToMult(path, CP_UTF8).c_str());
}

void CRepo::Create(const wchar_t* path, bool isBare)
{
    Create(JStd::String::ToMult(path, CP_UTF8).c_str(), isBare);
}
#endif

CRepo::~CRepo()
{
}


#ifdef WIN32
std::wstring CRepo::GetWPath() const
{
	return JStd::String::ToWide(GetPath(), CP_UTF8);
}
#endif

std::string CRepo::GetPath() const
{
	const char* pRet = git_repository_path(GetInternalObj());
	if(pRet == NULL)
		throw CGitException(0, "git_repository_path()");
	return pRet;
}

std::string CRepo::DiscoverPath(const char* startPath, bool acrossFs, const char* ceilingDirs)
{
    CBuf result;
    ThrowIfError(git_repository_discover(
                        result.H(),
                        startPath,
                        acrossFs ? 1 : 0,
                        ceilingDirs == NULL ? "" : ceilingDirs),
                 "git_repository_discover()");
    return result.AsString();
}

#ifdef WIN32
std::wstring CRepo::DiscoverPath(const wchar_t* startPath, bool acrossFs, const wchar_t* ceilingDirs)
{
    return JStd::String::ToWide(DiscoverPath(JStd::String::ToMult(startPath, CP_UTF8).c_str(),
                        acrossFs,
                        JStd::String::ToMult(ceilingDirs == NULL ? L"" : ceilingDirs, CP_UTF8).c_str())
                , CP_UTF8);
}
#endif

void CRepo::Read(CObjectBase& obj, const COid& oid, git_otype type)
{
	git_object* pobj = NULL;
	ThrowIfError(git_object_lookup(&pobj, GetInternalObj(), &oid.m_oid, type), "git_object_lookup()");
	obj.Attach(pobj);
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
	git_odb* podb = NULL;
	ThrowIfError(git_repository_odb(&podb, GetInternalObj()), "git_repository_odb()");
	return podb;
}

bool CRepo::IsBare()const
{
	CheckValid();
	return git_repository_is_bare(GetInternalObj()) ? true : false;
}

int addRef(git_reference* pref, void* refs)
{
	((RefVector*)refs)->emplace_back(pref);
	return 0;
}

void CRepo::GetReferences(RefVector& refs) const
{
	ThrowIfError(git_reference_foreach(GetInternalObj(), &addRef, &refs), "git_reference_foreach()");
}

int CRepo::ForEachRef_NoThrow(const T_forEachRefNothrowCallback& callback) const
{
	return git_reference_foreach(GetInternalObj(), [](git_reference* pref, void* vthis)
	{
		CRef ref(pref);
		return (*(const T_forEachRefNothrowCallback*)vthis)(ref);
	}, (void*)&callback);
}

void CRepo::ForEachRef(const T_forEachRefCallback& callback) const
{
	std::string					m_exception;
	bool						m_exceptionThrown = false;

	int error = ForEachRef_NoThrow([&](CRef& ref)
	{
		try
		{
			callback(ref);
		}
		catch(std::exception& e)
		{
			m_exception = e.what();
			m_exceptionThrown = true;
			//Cannot rethrow here... it needs to fall through the c stack.
			//Will be thrown later, but exception is always from type std::exception.
			//It is not possible to template the catch handler to later rethrow the same exception as what was catched here.
			return (int)GIT_ERROR;
		}
		return 0;
	});
	if(m_exceptionThrown)
        throw std::runtime_error(m_exception.c_str()); //rethrow exception caught from callback
	ThrowIfError(error, "git_reference_foreach()"); //Otherwise maybe throw some other error
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
	return tree.Write();
}


VectorCommit CRepo::ToCommits(const COids& oids)
{
	VectorCommit ret;
	ret.reserve(oids.m_oids.size());
	for(COids::VectorCOid::const_iterator i = oids.m_oids.begin(); i != oids.m_oids.end(); ++i)
        ret.push_back(std::shared_ptr<CCommit>(std::make_shared<CCommit>(*this, *i)));
	return ret;
}

CRef CRepo::GetRef(const char* name) const
{
	return CRef(*this, name);
}

CRef CRepo::MakeRef(const char* name, const COid& oid, bool force, const char* refLogMsg)
{
	git_reference* ref = NULL;
	ThrowIfError(git_reference_create(&ref, GetInternalObj(), name, &oid.GetInternalObj(), force ? 1 : 0, refLogMsg), "git_reference_create_oid()");
	return ref;
}

COid CRepo::CreateTag(const char* name, const CObjectBase& target, bool force)
{
	git_oid oid;
	ThrowIfError(git_tag_create_lightweight(&oid, GetInternalObj(), name, target.GetInternalObj(), force ? 1 : 0), "git_tag_create_lightweight()");
	return oid;
}

COid CRepo::CreateTag(const char* name, const COid& target, bool force)
{
	return CreateTag(name, CObjectBase(*this, target, GIT_OBJ_COMMIT), force);
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
									"UTF-8",
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
	while(*pathStart == '/')
		++pathStart;
	const char* pathEnd = strchr(pathStart, '/');
	if(pathEnd == NULL)
		pathEnd = pathStart + strlen(pathStart);
	if(pathEnd == pathStart)
		//Empty path. Don't use it like this
		throw std::runtime_error("TreeFind() done with invalid path name");

	std::string pathPart(pathStart, pathEnd);
	CTreeEntry entry = start.Entry(pathPart.c_str());

	while(*pathEnd == '/')
		++pathEnd;
	if(*pathEnd == '\0')
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
	node.m_attributes	= GIT_FILEMODE_TREE;
//	node.m_name			= "";
	node.m_oid			= tree.ID();

	size_t entryCount = tree.EntryCount();
	for(size_t i = 0; i < entryCount; ++i)
	{
		CTreeEntry entry = tree.Entry(i);
		if(entry.IsFile())
		{
			//node.Insert(entry.Name().c_str(), entry.Oid(), entry.Attributes());
			CTreeNode& newNode = *node.GetByPath(entry.Name().c_str(), true);
			newNode.m_subTree.clear();
			newNode.m_attributes	= entry.FileMode();
			newNode.m_oid			= entry.Oid();
			continue;
		}
		CTreeNode& newNode = *node.GetByPath(entry.Name().c_str(), true);
		//CTreeNode& newNode = *node.m_subTree.insert(node.m_subTree.end(), CTreeNode());
		BuildTreeNode(newNode, entry.Oid());
		newNode.m_name			= entry.Name();
		newNode.m_attributes	= entry.FileMode();
		if(newNode.m_oid != entry.Oid())
			throw std::logic_error("Tree OID mismatch.");
		newNode.m_oid			= entry.Oid();
	}
}

void CRepo::BuildTreeNode(CTreeNode& node, const COid& tree)
{
	BuildTreeNode(node, CTree(*this, tree));
}

void CRepo::DefaultSig(CSignature& sig)
{
	CConfig config;
	config.OpenDefault();
	sig.Create(config.Val<std::string>("user.name")->c_str(), config.Val<std::string>("user.email")->c_str());
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

void CCommitWalker::Hide(const COid& oid)
{
	ThrowIfError(git_revwalk_hide(GetInternalObj(), &oid.GetInternalObj()), "git_revwalk_hide()");
	m_nextCalled = false; //Documentation doesnt say whether or not git_revwalk_hide() resets the walker. So this is assumed it does
}



void CCommitWalker::Sort(unsigned int sort)
{
	git_revwalk_sorting(GetInternalObj(), sort);
	m_nextCalled = false; //git_revwalk_sorting() resets the walker
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
	case 0: m_end = false; break;
	case GIT_ITEROVER: m_end = true; break;
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

