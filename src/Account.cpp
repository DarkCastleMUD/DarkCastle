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

bool CAccount::hasCharacter( string name )
{
   for(unsigned int i = 0; i < characternames.size(); i++)
      if( 0 == characternames[i].compare( name ) )
         return true;
   return false;
}

void CAccount::charListToBuf( char buf[] )
{
   unsigned int size = sizeof(buf);
   char temp[50];
   string nextname;
   unsigned int i;

   assert( size > 40 );

   *buf = '\0';  // clear buf

   if( characternames.size() < 1 ) {
      strcpy(buf, "NONE!");
      return;
   }

   for(i = 0; i < characternames.size(); i++) {
      nextname = characternames[i];
      if( ( strlen(buf) + 34) >= size ) {
         strcpy(buf, "Error:Acct:charListToBuf - too long");
         return;
      }
      sprintf(temp, "%30s%s", nextname.c_str(), ((i+1) % 3 == 0) ? "\r\n" : "" );
      strcat(buf, temp);
   }
   if( i % 3 != 0 )
     strcat(buf, "\r\n");
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

