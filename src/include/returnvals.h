/*   returnvals.h
 *   Created by:  Pirahna
 *
 *   Contains return values everything should use unless specified otherwise.
 */

#ifndef _RETURNVALS_H_
#define _RETURNVALS_H_

enum {
  eFAILURE          = 1,
  eSUCCESS          = 1<<1,
  eCH_DIED          = 1<<2,
  eVICT_DIED        = 1<<3,
  eINTERNAL_ERROR   = 1<<4,
  eEXTRA_VALUE 	    = 1<<5 // Added to act like a flag, setting if something
		           // Special happened in the function.. (verify_existing_components use at the moment)
};

#endif // _RETURNVALS_H_
