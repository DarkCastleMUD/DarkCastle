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
//   uint32 type;       // type of affect
//   int16  duration;   // how long it has left (-1 = infinate)
//   int32  modifier;   // how much is added/subtracted
//   int32  location;   // which ability to modify (APPLY_XXX)

//   obj_data * myobj;  // if this is given to me by an obj, which one?

CAffect::CAffect()
{
   type = 0;
   duration = 0;
   modifier = 0;
   location = 0;
   myobj = NULL;
}

CAffect::CAffect(CAffect & rhs)
{
   type = rhs.type;
   duration = rhs.duration;
   modifier = rhs.modifier;
   location = rhs.location;
   myobj = rhs.myobj;
}

void CAffect::setType(uint32 val)
{
   type = val;
}

void CAffect::setDuration(int16 val)
{
   duration = val;
}

void CAffect::setModifier(int16 val)
{
   modifier = val;
}

void CAffect::setLocation(int16 val)
{
   location = val;
}

void CAffect::setObj(obj_data * val)
{
   myobj = val;
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

int CAffectList::findAffect(uint32 target)
{
   int bottom = 0, middle, top = list.size( ) - 1;
   uint32 current;

   while( bottom <= top )
   {
      middle = ( top + bottom ) / 2;
      current = ((CAffect) list[middle]).getType( );
      if( target > current  )
         bottom = middle + 1 ;
      else if( target < current )
         top = middle - 1 ;
      else
         return middle;
   }
   return -1;
}

CAffect CAffectList::getAffect(uint32 affectnum)
{
   int i = findAffect(affectnum);
   return list[i];
}

int CAffectList::isAffectedBy(uint32 affect)
{
   int i = findAffect(affect);

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

void CAffectList::removeAffect(uint32 affect)
{
/* TODO
   int loc = findAffect( affect );

   if(loc != -1)
      list.erase( );
*/
}

void CAffectList::removeAffect(CAffect affect)
{
   removeAffect( affect.getType() );
}

