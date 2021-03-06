/****************************************************************************
*
*     Copyright (c) 2001 Broadcom
*           All Rights Reserved
*
*     No portions of this material may be reproduced in any form without the
*     written permission of:
*
*           Broadcom
*           16251 Laguna Canyon Road
*           Irvine, California  92618
*
*     All information contained in this document is Broadcom
*     company private, proprietary, and trade secret.
*
*****************************************************************************/
/**
*
*  @file    bosEventLinuxUser.h
*
*  @brief   LinuxUser specific definitions for the BOS Event module.
*
****************************************************************************/

#if !defined( BOSEVENTLINUXUSER_H )
#define BOSEVENTLINUXUSER_H        /**< Include Guard                           */

/* ---- Include Files ---------------------------------------------------- */

#if !defined( BOSLINUXUSER_H )
#  include "bosLinuxUser.h"
#endif

/**
 * @addtogroup bosEvent
 * @{
 */

/* ---- Constants and Types ---------------------------------------------- */

typedef struct BOS_EVENT
{
   /* Bit used in synchronization primitive. */
   BOS_UINT32        eventBit;

   /* Index into internal TCB table. */
   unsigned int      taskIndex;

} BOS_EVENT;


typedef struct BOS_EVENT_SET
{
   /* The set of bits that correspond to the events that have been added. */
   BOS_UINT32        eventBits;

   /* The set of events that occurred. */
   BOS_UINT32        eventsReceived;

   /* Index into internal TCB table. */
   unsigned int      taskIndex;

} BOS_EVENT_SET;


/* ---- Variable Externs ------------------------------------------------- */
/* ---- Function Prototypes ---------------------------------------------- */

/** @} */

#endif /* BOSEVENTLINUXUSER_H */

