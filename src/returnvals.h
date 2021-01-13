/*   returnvals.h
 *   Created by:  Pirahna
 *
 *   Contains return values everything should use unless specified otherwise.
 */

#ifndef _RETURNVALS_H_
#define _RETURNVALS_H_

enum {
  eFAILURE          = 1U,
  eSUCCESS          = 1U<<1,
  eCH_DIED          = 1U<<2,
  eVICT_DIED        = 1U<<3,
  eINTERNAL_ERROR   = 1U<<4,
  eEXTRA_VALUE 	    = 1U<<5, // Added to act like a flag, setting if something
		           // Special happened in the function.. (verify_existing_components use at the moment)
  eEXTRA_VAL2       = 1U<<6, // damage() needs two

  eDELAYED_EXEC     = 1U<<7, // Mobprogs, MPPAUSE
  eIMMUNE_VICTIM    = 1U<<8  // returned by damage() when attacking somethat's immune to attack
};


#define SOCIAL_FALSE            0
#define SOCIAL_TRUE             1
#define SOCIAL_TRUE_WITH_NOISE  2

#endif // _RETURNVALS_H_
