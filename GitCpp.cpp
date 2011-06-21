// GitCpp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string>
#include "jstd\JStd.h"
#include <iostream>
#include "Git.h"

#include "jstd\Sha1.h"

using namespace std;

void ShowRef(Git::CRepo& repo, const char* ref, bool doError)
{
	//Git::CRef(repoTest, i->c_str()).Oid(true);
	cout << "- " << ref << " --> " << Git::CRef(repo, ref).Oid(true) << endl;
	if(doError)
		throw std::runtime_error("An error. Lets stop.");
}

int _tmain(int argc, _TCHAR* argv[])
{
	try
	{
		//Git::COid someCommitOid = "4d18f66bae98ec2a06d7f3c575eb5e130f6b4759";
		Git::CRepo repoTest(Git::CRepo::DiscoverPath(L"d:/develop/wine").c_str());
		Git::COid someCommitOid = Git::CRef(repoTest, "HEAD").Oid(true);

	cout << repoTest.GetPath() << " is " << (repoTest.IsBare() ? "" : "not ") << "bare." << endl;

	cout << "References: " << endl;
	//Method 1
	Git::StringVector refs;
	repoTest.GetReferences(refs, GIT_REF_LISTALL);
	for(Git::StringVector::iterator i = refs.begin(); i != refs.end(); ++i)
		ShowRef(repoTest, i->c_str(), false);

	char c;
	cin >> c;

	//Method 2
	cout << "Method 2... " << endl;
	repoTest.ForEachRef(std::tr1::bind(&ShowRef, std::tr1::ref(repoTest), std::tr1::placeholders::_1, false), GIT_REF_LISTALL);

	cout << "Thats all..." << endl;
	cin >> c;

	cout << "Do the test" << endl;

	Git::CRawObject W_Obj;
	repoTest.Odb().Read(W_Obj, someCommitOid);
	cout << "Object is of type " << W_Obj.Type() << endl
		<< W_Obj << endl;

	Git::COid W_Id = repoTest.Odb().Write(GIT_OBJ_BLOB, "Hello World!", 12);
	cout << "Blob got ID " << W_Id << endl;

	repoTest.Odb().Read(W_Obj, W_Id);
	cout << "Content: " << W_Obj << endl;

	Git::CCommit W_Commit;
	repoTest.Read(W_Commit, someCommitOid);
	
	cout << "Msg: " << W_Commit.Message() << endl;
	cout << "ShortMsg: " << W_Commit.MessageShort() << endl;
	cout << "Author: " << W_Commit.Author()->name << endl;

	Git::CCommitWalker W_Walk(repoTest);
	W_Walk.AddRev(someCommitOid);
	//W_Walk.Sort(GIT_SORT_TOPOLOGICAL);// | GIT_SORT_REVERSE);
	int count = 0;
	for(;W_Walk;++W_Walk)
	{
		try
		{
			++count;Git::CCommit(repoTest, *W_Walk).MessageShort();
			if(count % 1000 == 1)
				cout << "Commit " << count << ": " << Git::CCommit(repoTest, *W_Walk).MessageShort() << endl;
		}
		catch(std::exception& e)
		{
			try
			{
				cout << "Commit " << count << " has a problem: " << e.what() << endl
					 << "Raw content: " << Git::CRawObject(repoTest, *W_Walk) << endl;
			}
			catch(std::exception& e)
			{
				cout << "*** Oops: " << e.what() << endl;
			}
		}
	}
	cout << "Done. " << count << " revisions." << endl;
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

	Git::CRawObject obj(JStd::CSHA1::FromString(ref.Hash().c_str()));
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

