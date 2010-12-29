#pragma once
#include <map>
#include <string>
#include <iostream>

namespace Git
{

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
	CObject(const char* refHash);
	CObject(const std::string& refHash);



	std::string m_Hash;
};

class CRepo
{
public:
	CRepo(const wchar_t* P_szPathPtr);

	CRef	GetRef(const wchar_t* refName);
	void	LoadPackedRefs(MapRef& refMap);
	void	LoadFileRefs(MapRef& refMap, const wchar_t* subPath = NULL);
	void	LoadRefs(MapRef& refMap);
	void	EnsureNotSymbolic(CRef& ref);
	bool	Open(CObject& obj, const wchar_t* basePath = NULL);

private:

	std::wstring m_Path;
};


}

