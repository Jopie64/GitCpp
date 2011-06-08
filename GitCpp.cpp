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
	try
	{
		Git::CRepo repoTest(L"d:/develop/tortoisegit2/.git");

	cout << repoTest.GetPath() << endl;

	Git::COdb W_Odb1 = repoTest.Odb();
	Git::COdb W_Odb2 = repoTest.Odb();

	cout << "Doe de test" << endl;

	Git::CObject W_Obj;
	repoTest.Odb().Read(W_Obj, "4d18f66bae98ec2a06d7f3c575eb5e130f6b4759");
	cout << "Object is of type " << W_Obj.Type() << endl
		<< W_Obj << endl;
	/*
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

	Git::CObject obj(JStd::CSHA1::FromString(ref.Hash().c_str()));
	repoTest.Open(obj);


	JStd::CSha1Hash hash = JStd::CSHA1::GenerateHash("testje");

	cout 
		<< hash.AsString() << endl
		<< hash.AsString(JStd::CSha1Hash::REPORT_HEX_DASHED) << endl
		<< JStd::CSHA1::FromString(hash.AsString(JStd::CSha1Hash::REPORT_HEX_DASHED).c_str()).AsString() << endl
		<< JStd::CSHA1::FromString(hash.AsString().c_str()).AsString() << endl;

*/



	}
	catch(std::exception& e)
	{
		cout << "An exception occurred: " << e.what() << endl;
	}


	char c;
	cin >> c;
	return 0;
}

