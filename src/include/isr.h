#ifndef ISR_H_
#define ISR_H_
/************************************************************************
| $Id: isr.h,v 1.1 2002/06/13 04:32:22 dcastle Exp $
| isr.h
| Description:  This file defines the isr (IMMUNE/SUSCEPTIBLE/RESIST)
|   vectors for everything.
*/

#define ISR_PIERCE         1
#define ISR_SLASH          1<<1
#define ISR_MAGIC          1<<2
#define ISR_CHARM          1<<3
#define ISR_FIRE           1<<4
#define ISR_ENERGY         1<<5
#define ISR_ACID           1<<6
#define ISR_POISON         1<<7
#define ISR_SLEEP          1<<8
#define ISR_COLD           1<<9
#define ISR_PARA           1<<10
#define ISR_BLUDGEON       1<<11
#define ISR_WHIP           1<<12
#define ISR_CRUSH          1<<13
#define ISR_HIT            1<<14
#define ISR_BITE           1<<15
#define ISR_STING          1<<16
#define ISR_CLAW           1<<17
#define ISR_PHYSICAL       1<<18
#define ISR_NON_MAGIC      1<<19
#define ISR_KI             1<<20
#define ISR_SONG           1<<21
#define ISR_WATER          1<<22


#define SAVE_TYPE_FIRE     0
#define SAVE_TYPE_COLD     1
#define SAVE_TYPE_ENERGY   2
#define SAVE_TYPE_ACID     3
#define SAVE_TYPE_MAGIC    4
#define SAVE_TYPE_POISON   5
#define SAVE_TYPE_MAX      5  // Do not change this value.  Used in pfile writing
                              // for size of the array.  

                              // If you decide to add a new saving throw type you 
                              // will have to be a little tricky:) -pir 12/13/01 3:32am

#endif
