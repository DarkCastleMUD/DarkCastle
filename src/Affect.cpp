// CAffect.h
//
// Includes:
//      CAffect
//      CAffectList
//
// ANYTHING that affects a character should be put into this.  The list
// affects is sorted and very quick to access.  Pretty much all bitvector's
// and anything else can be put into this list.
//

#include "Affect.h"
// TODO - see notes in Affect.h


////////////////////////////////////////////////////////////////////////
//
// CAffect
//
////////////////////////////////////////////////////////////////////////

//private:
//   uint32 type;           // type of affect  

//   int16  spellid;        // if this is given to me by a spell/skill, which one?
//   obj_data * myobj;      // if this is given to me by an obj, which one?
//   int16  otherid;        // if this is given to me by something else

//   int16  spellduration;  // how long it has left (-1 = infinate)
//   int16  combatduration; // how long it has left (-1 = infinate) 
   
//   int32  modifier;       // how much is added/subtracted


CAffect::CAffect()
{
   type = 0;
   spellid = -1;
   myobj = NULL;
   otherid = -1;
   spellduration = -1;
   combatduration = -1;
   modifier = 0;

   myownerlist = NULL;
}

CAffect::CAffect(CAffect & rhs)
{
   type = rhs.type;
   spellid = rhs.spellid;
   myobj = rhs.myobj;
   otherid = rhs.spellid;
   spellduration = rhs.spellduration;
   combatduration = rhs.combatduration;
   modifier = rhs.modifier;

   myownerlist = NULL;
}

void CAffect::updateCombatTimer()
{
   if(-1 == combatduration)  // infinite
      return;

   combatduration--;   

   assert(myownerlist);

   if(combatduration < 1)
      myownerlist->removeAffect(*this);
}

void CAffect::updateTickTimer()
{
   if(-1 == spellduration) // infinite
      return;

   spellduration--;

   assert(myownerlist);

   if(spellduration < 1)
      myownerlist->removeAffect(*this);
}

void CAffect::setType(uint32 val)
{
   type = val;
}

void CAffect::setSpellId(int16 val)
{
   spellid = val;
}

void CAffect::setSpellDuration(int16 val)
{
   spellduration = val;
}

void CAffect::setCombatDuration(int16 val)
{
   combatduration = val;
}

void CAffect::setModifier(int16 val)
{
   modifier = val;
}

void CAffect::setObj(obj_data * val)
{
   myobj = val;
}

bool operator< (CAffect& left, CAffect& right)
{
   if( left.getAffect() != right.getAffect() )
      return ( left.getAffect() < right.getAffect() );
   if( left.getSpellId() != right.getSpellId() )
      return ( left.getSpellId() < right.getSpellId() );
   if( left.getOtherId() != right.getOtherId() )
      return ( left.getOtherId() < right.getOtherId() );

   // TODO
//   if(!left.getObj() || !right.getObj())
//      logf( ERROR );

   return ( left.getObj() < right.getObj() );
}

bool operator> (CAffect& left, CAffect& right)
{
   if( left.getAffect() != right.getAffect() )
      return ( left.getAffect() > right.getAffect() );
   if( left.getSpellId() != right.getSpellId() )
      return ( left.getSpellId() > right.getSpellId() );
   if( left.getOtherId() != right.getOtherId() )
      return ( left.getOtherId() > right.getOtherId() );

   // TODO
//   if(!left.getObj() || !right.getObj())
//      logf( ERROR );

   return ( left.getObj() > right.getObj() );
}

////////////////////////////////////////////////////////////////////////
//
// CAffectList
//
////////////////////////////////////////////////////////////////////////

//private:
//   vector<CAffect> list;

CAffectList::CAffectList()
{
}

void CAffectList::insertSorted(CAffect affect)
{
}

int CAffectList::findAffect(CAffect target)
{
   int bottom = 0, middle, top = list.size( ) - 1;
   CAffect current;

   while( bottom <= top )
   {
      middle = ( top + bottom ) / 2;
      current = ((CAffect) list[middle]);
      if( target > current  )
         bottom = middle + 1 ;
      else if( target < current )
         top = middle - 1 ;
      else
         return middle;
   }
   return -1;
}

CAffect CAffectList::getAffect(CAffect affect)
{
   int i = findAffect(affect);
   return list[i];
}

int CAffectList::isAffectedBy(uint32 affect)
{
   CAffect findme;
   findme.setType(affect);

   int i = findAffect(findme);

   if(-1 == i)     return 0;

   return 1;
}

// Affect with (allows multiples)
// copies object
void CAffectList::affectWith(CAffect affect)
{
   insertSorted(affect);
}

// Affect with (replaces existing if there)
// copies object
void CAffectList::affectWithReplace(CAffect affect)
{
   removeAffect(affect);
   insertSorted(affect);
}

// Affect with (if existing, add durations and merge)
// copies object
void CAffectList::affectWithJoin(CAffect affect)
{
/*
   TODO

   CAffect old = isAffectedBy(affect);
   int duration;

   if(ptr)
      duration = ptr->getDuration();
   else duration = 0;

   removeAffect(affect);

   affect.setDuration( duration + affect.getDuration() );

   ptr = new CAffect(affect);

   insertSorted(ptr);
*/
}

void CAffectList::removeAffect(CAffect affect)
{
   removeAffect( affect );
}

