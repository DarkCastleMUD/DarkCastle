// CAccount.cpp
//
// Includes:
//      CAccount
//
// Anything in the Account object belongs in here.  Login sequence and character
// management are all done though this.

#include "Account.h"


// TODO - see notes in Affect.h


////////////////////////////////////////////////////////////////////////
//
// CAccount
//
////////////////////////////////////////////////////////////////////////

/*
   string               login;
        
   string               firstname;
   string               lastname;
   string               address1;
   string               address2;
   string               address3;
   string               citystatezip;
   string               country;
   string               phone;

   vector<string>       characternames;
   int                  soulgems; 
   int                  bvOne;

*/

CAccount::CAccount(string loginid)
{
   login = loginid;
   soulgems = 0;
   bvOne = 0;
}

// Write account to file.
// 1 on success, 0 on failure
int CAccount::WriteToFile()
{
   return 0; // Failure
}

// Read account from file
// 1 on success, 0 on faiure
int CAccount::ReadFromFile()
{
   return 0; // Failure
}

