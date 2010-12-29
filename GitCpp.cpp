// GitCpp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string>
#include "jstd\JStd.h"
#include <iostream>
#include "Git.h"

#include "jstd\Sha1.h"

using namespace std;

int _tmain(int argc, _TCHAR* argv[])
{

	Git::CRepo repoTest(L"d:\\develop\\tortoisegit2");

//	Git::CRef ref = repoTest.GetRef(L"HEAD");
	Git::CRef ref = repoTest.GetRef(L"refs/heads/test");

	repoTest.EnsureNotSymbolic(ref);

	cout << ref << endl;

	Git::MapRef refs;
	repoTest.LoadRefs(refs);

	for(Git::MapRef::iterator i = refs.begin(); i != refs.end(); ++i)
	{
		cout << i->first << " " << i->second << endl;
	}

	Git::CObject obj(ref.Hash());
	repoTest.Open(obj);


	JStd::CSha1Hash hash = JStd::CSHA1::GenerateHash("testje");

	cout 
		<< hash.AsString() << endl
		<< hash.AsString(JStd::CSha1Hash::REPORT_HEX_DASHED) << endl
		<< JStd::CSHA1::FromString(hash.AsString(JStd::CSha1Hash::REPORT_HEX_DASHED).c_str()).AsString() << endl
		<< JStd::CSHA1::FromString(hash.AsString().c_str()).AsString() << endl;






	char c;
	cin >> c;
	return 0;
}

