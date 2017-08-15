/***************************************************************************
*    Copyright (c) 2008-2012 Broadcom Corporation                                        
*
*    This program is the proprietary software of Broadcom Corporation and/or
*    its licensors, and may only be used, duplicated, modified or
*    distributed pursuant to the terms and conditions of a separate, written
*    license agreement executed between you and Broadcom (an "Authorized
*    License").  Except as set forth in an Authorized License, Broadcom
*    grants no license (express or implied), right to use, or waiver of any
*    kind with respect to the Software, and Broadcom expressly reserves all
*    rights in and to the Software and all intellectual property rights
*    therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO
*    USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM
*    AND DISCONTINUE ALL USE OF THE SOFTWARE.
*
*
*    Except as expressly set forth in the Authorized License,
*
*    1.     This program, including its structure, sequence and
*    organization, constitutes the valuable trade secrets of Broadcom, and
*    you shall use all reasonable efforts to protect the confidentiality
*    thereof, and to use this information only in connection with your use
*    of Broadcom integrated circuit products.
*
*    2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
*    "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
*    REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR
*    OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
*    DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
*    NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
*    ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
*    CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT
*    OF USE OR PERFORMANCE OF THE SOFTWARE.
*
*    3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
*    BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL,
*    SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN
*    ANY WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
*    IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR
*    (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE
*    ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY
*    NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
*
****************************************************************************
*
*    Filename: slic6838Si32392.c
*
****************************************************************************
*    Description:
*
*      This file contains functions related to the control of the
*      SI32392 Subscriber Line Interface Circuits (SLIC).
*
****************************************************************************/

#include <xdrvTypes.h>
#include <kernelMainWrapper.h>
#include "mspiDef.h"
#include <xdrvLog.h>
#include <xdrvApm.h>
#include <string.h>
#include <bosSleep.h>
#include <xchgAssert.h>
#include "slic6838Si32392.h"
#include "slic6838Si32392Defs.h"

#include "hvg6838.h"
#include "apm6838.h"

#include "bcmSpiRes.h"

/* SI32392 SLIC states */
#define SI32392_STATE_DISCONNECT              0x00     /* Disconnect */
#define SI32392_STATE_STANDBY                 0x01     /* Standby */
#define SI32392_STATE_FWD_ACTIVE              0x02     /* Forward Active */
#define SI32392_STATE_REV_ACTIVE              0x03     /* Reverse Active */
#define SI32392_STATE_RING_OPEN               0x04     /* Ring open */
#define SI32392_STATE_TIP_OPEN                0x05     /* Tip open */
#define SI32392_STATE_AC_RING_TRIP            0x06     /* Ringing with AC ring trip */
#define SI32392_STATE_DC_RING_TRIP            0x07     /* Ringing with DC ring trip */
#define SI32392_STATE_NULL                    0x08     /* Invalid mode */

/* SLIC initial/shutdown states */
#define SI32392_STATE_INIT                    SI32392_STATE_FWD_ACTIVE
#define SI32392_STATE_DEINIT                  SI32392_STATE_FWD_ACTIVE

/* SLIC miscellaneous control parameters */
#define SI32392_RING_START_DELAY              2

/* SLIC "meta-driver" interface functions */
static void slic6838Si32392ModeControl( XDRV_SLIC *pDrv, XDRV_SLIC_MODE mode );
static void slic6838Si32392LedControl( XDRV_SLIC *pDrv, XDRV_UINT16 value );
static void slic6838Si32392PhaseReversalControl( XDRV_SLIC *pDrv, XDRV_UINT16 value );
static XDRV_BOOL slic6838Si32392IsOffhook( XDRV_SLIC *pDrv );
static int slic6838Si32392SetRingParms( XDRV_SLIC *pDrv, int ringFrequency, int ringWaveshape,
                                       int ringVoltage, int ringOffset, int ringOffsetCal );
static int slic6838Si32392GetRingParms( XDRV_SLIC *pDrv, int *ringFrequency, int *ringWaveshape,
                                       int *ringVoltage, int *ringOffset, int *ringOffsetCal );
static int slic6838Si32392SetBoostedLoopCurrent( XDRV_SLIC *pDrv, XDRV_UINT16 value );
static int slic6838Si32392SetPowerSource( XDRV_SLIC *pDrv, XDRV_UINT16 value );
static XDRV_BOOL slic6838Si32392GetOverCurrentStatus( XDRV_SLIC *pDrv );
static int slic6838Si32392GetSlicParms( XDRV_SLIC *pDrv, int *phaseReversal, int *loopCurrent, int *powerSource, int *slicMode );
static XDRV_SINT16 slic6838Si32392GetDlp( XDRV_SLIC *pDrv );
static void slic6838Si32392ProcessEvents( XDRV_SLIC *pDrv );
static XDRV_UINT32 slic6838Si32392Probe( XDRV_SLIC *pDrv, XDRV_UINT32 deviceId, XDRV_UINT32 chan, XDRV_UINT32 reg,
                                      XDRV_UINT32 regSize, void* value, XDRV_UINT32 valueSize, XDRV_UINT32 indirect, 
                                      XDRV_UINT8 set );

/* SLIC "meta-driver" interface */
static const XDRV_SLIC_FUNCS slic6838Si32392DrvFuncs =
{
   slic6838Si32392ModeControl,
   slic6838Si32392LedControl,
   slic6838Si32392PhaseReversalControl,
   slic6838Si32392IsOffhook,
   slic6838Si32392SetRingParms,
   slic6838Si32392GetRingParms,
   slic6838Si32392SetBoostedLoopCurrent,
   slic6838Si32392SetPowerSource,
   slic6838Si32392GetOverCurrentStatus,
   slic6838Si32392GetDlp,
   slic6838Si32392ProcessEvents,
   slic6838Si32392Probe,
   slic6838Si32392GetSlicParms
};

/* Private Function Prototypes */
static void OpenSlic( SLIC_6838_SI32392_DRV *pDev );
static void CloseSlic( SLIC_6838_SI32392_DRV *pDev );
static void InitSlicRegs( SLIC_6838_SI32392_DRV *pDev );
static void setSlicState( SLIC_6838_SI32392_DRV *pDev, int state, XDRV_SLIC_MODE mode );
static XDRV_UINT16 MSPI_Read( int spiDevId, int chan, XDRV_UINT8 addr );
static void MSPI_Write( int spiDevId, int chan, XDRV_UINT8 addr, XDRV_UINT16 data );
static int MSPI_Write_Verify( int spiDevId, int chan, XDRV_UINT8 addr, XDRV_UINT16 data );
static int MSPI_Mask_Write( int spiDevId, int chan, XDRV_UINT8 addr, XDRV_UINT16 data, XDRV_UINT16 mask, int verify );

/*
*****************************************************************************
** FUNCTION:   slic6838Si32392Init
**
** PURPOSE:    Initialize and open the Si32392 SLIC
**
** PARAMETERS: chan          - the line number ( 0 referenced )
**             pDev          - pointer to the Si32392 SLIC info structure
**             pSlicCfgInfo  - SLIC configuration structure
**             pApmDrv       - pointer to APM driver associate with this slic
**
** RETURNS:    none
**
*****************************************************************************
*/
void slic6838Si32392Init
(
   int                   chan,
   SLIC_6838_SI32392_DRV *pDev,
   const SLICCFG_6838_SI32392_INFO *pSlicCfgInfo,
   XDRV_APM             *pApmDrv
)
{
   int status;

   if ( pDev->bDrvEnabled )
   {
      XDRV_LOG_WARN(( XDRV_LOG_MOD_XDRV, "XDRV_SLIC: WARNING - chan %d driver already enabled !!!", chan));
   }

   /* Reset the slic driver */
   memset( pDev, 0, sizeof( SLIC_6838_SI32392_DRV ) );

   /* Initialize the SLIC information for each channel */
   pDev->chan = chan;
   pDev->pDrvFuncs = &slic6838Si32392DrvFuncs;

   /* Initialize the APM driver structure */
   pDev->pApmDrv = pApmDrv;

   /* Driver enabled */
   pDev->bDrvEnabled = XDRV_TRUE;
   pDev->bRingDCoffsetEnabled = XDRV_FALSE;

   /* Enhanced control features */
   pDev->bEnhancedControl = pSlicCfgInfo->enhancedControl;

   /* SPI device id */
   pDev->spiDevId = pSlicCfgInfo->spiDevId;

   status = BcmSpiReserveSlave(MSPI_BUS_ID, pDev->spiDevId, MSPI_CLK_FREQUENCY);
 
   if ( status != SPI_STATUS_OK )
   {
      XDRV_LOG_INFO(( XDRV_LOG_MOD_XDRV, "Error during BcmSpiReserveSlave (status %d) ", status ));
      // return ( XDRV_SLIC_STATUS_ERROR );
      return;
   }

   /* Open the SLIC */
   OpenSlic( pDev );
}


/*
*****************************************************************************
** FUNCTION:   slic6838Si32392Deinit
**
** PURPOSE:    Shutdown the Si32392 SLIC
**
** PARAMETERS: pDev  - pointer to the Si32392 SLIC info structure
**
** RETURNS:    none
**
*****************************************************************************
*/
void slic6838Si32392Deinit( SLIC_6838_SI32392_DRV *pDev )
{
   if ( pDev->bDrvEnabled == XDRV_FALSE )
   {
      XDRV_LOG_ERROR(( XDRV_LOG_MOD_XDRV, "XDRV_SLIC: ERROR - driver already disabled !!!"));
   }

   /* Close the SLIC */
   CloseSlic( pDev );

   memset( pDev, 0, sizeof( SLIC_6838_SI32392_DRV ) );
}


/*
*****************************************************************************
** FUNCTION:   slic6838Si32392IsOffhook
**
** PURPOSE:    Determine if a channel is on or off hook
**
** PARAMETERS: pDrv - pointer to the SLIC driver device structure
**
** RETURNS:    TRUE if offhook, FALSE if onhook
**
*****************************************************************************
*/
static XDRV_BOOL slic6838Si32392IsOffhook( XDRV_SLIC *pDrv )
{
   SLIC_6838_SI32392_DRV *pDev = (SLIC_6838_SI32392_DRV *)pDrv;
   int                   chan = pDev->chan;
   XDRV_UINT16           hookStatus;

   if( pDev->slicMode == XDRV_SLIC_MODE_LCFO )
   {
      return( XDRV_FALSE );
   }

      /* Check hookstate status */
      hookStatus = MSPI_Read( pDev->spiDevId,  chan, SI32392_REG_DEVICE_STATUS );

      if ( hookStatus & (( chan == 0 ) ? SI32392_CHAN0_HOOKSWITCH_STAT_OFF:SI32392_CHAN1_HOOKSWITCH_STAT_OFF ) )
      {
         pDev->bOffhook = XDRV_TRUE;
      }
      else
      {
         pDev->bOffhook = XDRV_FALSE;
      }

   return ( pDev->bOffhook );
}


/*
*****************************************************************************
** FUNCTION:   slic6838Si32392ModeControl
**
** PURPOSE:    Sets the SLIC into one of the defined modes
**
** PARAMETERS: pDrv  - pointer to the SLIC driver device structure
**             mode  - the mode to set the SLIC into.
**
** RETURNS:    none
**
*****************************************************************************
*/
static void slic6838Si32392ModeControl( XDRV_SLIC *pDrv, XDRV_SLIC_MODE mode )
{
   SLIC_6838_SI32392_DRV *pDev = (SLIC_6838_SI32392_DRV *)pDrv;
   int chan = pDev->chan;
   int slicState;

   switch( mode )
   {
      case XDRV_SLIC_MODE_LCFO:
      {
         slicState = SI32392_STATE_DISCONNECT;
      }
      break;

      /* Standby mode */
      case XDRV_SLIC_MODE_STANDBY:
      {
         if( pDev->phaseReversalActive )
         {
            slicState = SI32392_STATE_REV_ACTIVE;
         }
         else
         {
            slicState = SI32392_STATE_STANDBY;
         }
      }
      break;

      /* On-hook transmission */
      /* Loop current feed */
      case XDRV_SLIC_MODE_OHT:
      case XDRV_SLIC_MODE_LCF:
      {
         if( pDev->phaseReversalActive )
         {
            slicState = SI32392_STATE_REV_ACTIVE;
         }
         else
         {
            slicState = SI32392_STATE_FWD_ACTIVE;
         }
      }
      break;

      /* On-hook transmission reverse */
      /* Reverse loop current feed */
      case XDRV_SLIC_MODE_OHTR:
      case XDRV_SLIC_MODE_RLCF:
      {
         if( pDev->phaseReversalActive )
         {
            slicState = SI32392_STATE_FWD_ACTIVE;
         }
         else
         {
            slicState = SI32392_STATE_REV_ACTIVE;
         }
      }
      break;

      /* Tip Open */
      case XDRV_SLIC_MODE_TIPOPEN:
      {
         slicState = SI32392_STATE_TIP_OPEN;
      }
      break;

      /* Ring Open */
      case XDRV_SLIC_MODE_RINGOPEN:
      {
         slicState = SI32392_STATE_RING_OPEN;
      }
      break;

      /* Ringing */
      case XDRV_SLIC_MODE_RING:
      {
         slicState = SI32392_STATE_AC_RING_TRIP;
      }
      break;

      default:
      {
         XDRV_LOG_ERROR(( XDRV_LOG_MOD_XDRV, " XDRV_SLIC: ERROR - unrecognized SLIC mode 0x%02x", mode));
         return;
      }
      break;
   }

   if(( pDev->slicMode == XDRV_SLIC_MODE_RING ) &&
      ( mode == XDRV_SLIC_MODE_RING ) &&
      ( pDev->pendingSlicTime > 0 ))
   {
      /* Ringing stopping */
      pDev->pendingSlicState = slicState;
      pDev->pendingSlicMode = mode;
   }
   else if(( pDev->slicMode == XDRV_SLIC_MODE_RING ) &&
           ( pDev->disableRingMode == XDRV_FALSE ) &&
           (( mode == XDRV_SLIC_MODE_STANDBY ) ||
            ( mode == XDRV_SLIC_MODE_OHT ) ||
            ( mode == XDRV_SLIC_MODE_OHTR )))
   {
      if( pDev->pendingSlicTime > 0 )
      {
         /* Ringing already stopping */
         pDev->pendingSlicState = slicState;
         pDev->pendingSlicMode = mode;
      }
      else
      {
         /* Initialize delayed state change */
         pDev->pendingSlicState = slicState;
         pDev->pendingSlicMode = mode;
         pDev->pendingSlicTime = 100;

         /* Update the HVG parameters based on the current SLIC state */
         xdrvApmHvgUpdateSlicStatus( pDev->pApmDrv, chan, XDRV_SLIC_MODE_RING,
                                     pDev->bRingDCoffsetEnabled, pDev->bOnBattery,
                                     pDev->bBoostedLoopCurrent, XDRV_TRUE );
      }
   }
   else
   {
      /* Clear pending state changes */
      pDev->pendingSlicState = 0;
      pDev->pendingSlicMode = 0;
      pDev->pendingSlicTime = 0;

      /* Start the appropriate mode change */
      setSlicState( pDev, slicState, mode );

      /* Save the current requested SLIC mode */
      pDev->slicMode = mode;
   }   
}


/*
*****************************************************************************
** FUNCTION:   slic6838Si32392LedControl
**
** PURPOSE:    This function controls nothing - legacy compiling support
**
** PARAMETERS: pDrv  - pointer to the SLIC driver device structure
**             value - 1 (on) or 0 (off)
**
** RETURNS:    None
**
*****************************************************************************
*/
static void slic6838Si32392LedControl( XDRV_SLIC *pDrv, XDRV_UINT16 value )
{
   /* SLICs do not control LEDs on the 6838 */
}

/*
*****************************************************************************
** FUNCTION:   slic6838Si32392PhaseReversalControl
**
** PURPOSE:    This function controls the phase of the SLICs
**
** PARAMETERS: pDrv  - pointer to the SLIC driver device structure
**             value - 1 (on) or 0 (off)
**
** RETURNS:    none
**
*****************************************************************************
*/
static void slic6838Si32392PhaseReversalControl( XDRV_SLIC *pDrv, XDRV_UINT16 value )
{
   SLIC_6838_SI32392_DRV *pDev = (SLIC_6838_SI32392_DRV *)pDrv;

   /* Set the phase reversal control flag */
   pDev->phaseReversalActive = value;

   if(( pDev->slicMode == XDRV_SLIC_MODE_STANDBY ) ||
      ( pDev->slicMode == XDRV_SLIC_MODE_OHT ) ||
      ( pDev->slicMode == XDRV_SLIC_MODE_OHTR ) ||
      ( pDev->slicMode == XDRV_SLIC_MODE_LCF ) ||
      ( pDev->slicMode == XDRV_SLIC_MODE_RLCF ))
   {
      slic6838Si32392ModeControl( pDrv, pDev->slicMode );
   }
}

/*
*****************************************************************************
** FUNCTION:   slic6838Si32392SetRingParms
**
** PURPOSE:    This function controls the ring waveform
**
** PARAMETERS: pDrv  - pointer to the SLIC driver device structure
**             ringFrequency  - ringing frequency
**             ringWaveshape  - ringing waveshape
**             ringVoltage    - ringing voltage
**             ringOffset     - ringing DC offset
**             ringOffsetCal  - ringing DC offset calibration
**
** RETURNS:    0 on success
**
*****************************************************************************
*/
static int slic6838Si32392SetRingParms( XDRV_SLIC *pDrv, int ringFrequency, int ringWaveshape,
                                      int ringVoltage, int ringOffset, int ringOffsetCal )
{
   SLIC_6838_SI32392_DRV *pDev = (SLIC_6838_SI32392_DRV *)pDrv;
   XDRV_UINT16 slicControlReg;

   if ( ringOffset )
   {
      pDev->bRingDCoffsetEnabled = XDRV_TRUE;
   }
   else
   {
      pDev->bRingDCoffsetEnabled = XDRV_FALSE;
   }

   /* Update RSPOL setting */
   slicControlReg = MSPI_Read( pDev->spiDevId,  pDev->chan, SI32392_REG_LINEFEED_CONFIG_2 );
   slicControlReg &= ~SI32392_RSPOL_MASK;
   if ( ringOffset < 0 )
   {
      slicControlReg |= SI32392_RSPOL_REVERSED;
   }
   MSPI_Write( pDev->spiDevId,  pDev->chan, SI32392_REG_LINEFEED_CONFIG_2, slicControlReg );

   /* Save ring settings */
   pDev->ringVoltage = ringVoltage;
   pDev->ringOffset = ringOffset;
   pDev->ringOffsetCal = ringOffsetCal;
   pDev->ringFrequency = ringFrequency;
   pDev->ringWaveshape = ringWaveshape;

   return ( 0 );
}

/*
*****************************************************************************
** FUNCTION:   slic6838Si32392GetRingParms
**
** PURPOSE:    This function gets the current ring waveform parameters
**
** PARAMETERS: pDrv           - pointer to the SLIC driver device structure
**             ringFrequency  - ringing frequency
**             ringWaveshape  - ringing waveshape
**             ringVoltage    - ringing voltage
**             ringOffset     - ringing DC offset
**             ringOffsetCal  - ringing DC offset calibration
**
** RETURNS:    0 on success
**
*****************************************************************************
*/
static int slic6838Si32392GetRingParms( XDRV_SLIC *pDrv, int *ringFrequency, int *ringWaveshape,
                                       int *ringVoltage, int *ringOffset, int *ringOffsetCal )
{
   SLIC_6838_SI32392_DRV *pDev = (SLIC_6838_SI32392_DRV *)pDrv;

   /* Retrieve ring settings */
   *ringVoltage = pDev->ringVoltage;
   *ringOffset = pDev->ringOffset;
   *ringOffsetCal = pDev->ringOffsetCal;
   *ringFrequency = pDev->ringFrequency;
   *ringWaveshape = pDev->ringWaveshape;

   return ( 0 );
}

/*
*****************************************************************************
** FUNCTION:   slic6838Si32392GetSlicParms
**
** PURPOSE:    This function gets the current slic configuration parameters
**
** PARAMETERS: pDrv  - pointer to the SLIC driver device structure
**             phaseReversal  - current phase reversal setting
**             loopCurrent    - current loop current setting
**             powerSource    - current power source config setting
**             slicMode       - current slic mode
**
** RETURNS:    0 on success
**
*****************************************************************************
*/
static int slic6838Si32392GetSlicParms( XDRV_SLIC *pDrv, int *phaseReversal, int *loopCurrent, int *powerSource, int *slicMode )
{
   SLIC_6838_SI32392_DRV *pDev = (SLIC_6838_SI32392_DRV *)pDrv;

   /* Retrieve ring settings */
   *phaseReversal = pDev->phaseReversalActive;
   *loopCurrent = pDev->bBoostedLoopCurrent;
   *powerSource = pDev->bOnBattery;
   *slicMode = pDev->slicMode;

   return ( 0 );
}

/*
*****************************************************************************
** FUNCTION:   slic6838Si32392SetBoostedLoopCurrent
**
** PURPOSE:    This function controls boosted loop current
**
** PARAMETERS: pDrv  - pointer to the SLIC driver device structure
**             value - 1 (on) or 0 (off)
**
** RETURNS:    0 on success
**
*****************************************************************************
*/
static int slic6838Si32392SetBoostedLoopCurrent( XDRV_SLIC *pDrv, XDRV_UINT16 value )
{
   SLIC_6838_SI32392_DRV *pDev = (SLIC_6838_SI32392_DRV *)pDrv;
   int chan = pDev->chan;

   /* Record boosted loop current state */
   pDev->bBoostedLoopCurrent = value;

   /* 
   ** Program the SLIC for appropriate current limit: 
   ** 0 - 25mA, 1 - 40mA 
   */
   if ( value )
   {
      MSPI_Mask_Write( pDev->spiDevId,  chan, SI32392_REG_LINEFEED_CONFIG_2,
                       ( chan == 0 ) ? SI32392_CHAN0_LOOP_CUR_THRESH_40:SI32392_CHAN1_LOOP_CUR_THRESH_40,
                       ( chan == 0 ) ? SI32392_CHAN0_LOOP_CUR_THRESH_MASK:SI32392_CHAN1_LOOP_CUR_THRESH_MASK, 0 );
      XDRV_LOG_INFO(( XDRV_LOG_MOD_XDRV, "XDRV_SLIC %d: Loop current set to 40 mA", pDev->chan ));
   }
   else
   {
      MSPI_Mask_Write( pDev->spiDevId,  chan, SI32392_REG_LINEFEED_CONFIG_2,
                       ( chan == 0 ) ? SI32392_CHAN0_LOOP_CUR_THRESH_25:SI32392_CHAN1_LOOP_CUR_THRESH_25,
                       ( chan == 0 ) ? SI32392_CHAN0_LOOP_CUR_THRESH_MASK:SI32392_CHAN1_LOOP_CUR_THRESH_MASK, 0 );
      XDRV_LOG_INFO(( XDRV_LOG_MOD_XDRV, "XDRV_SLIC %d: Loop current set to 25 mA", pDev->chan ));
   }

   /* Update the SLIC and HVG states */
   slic6838Si32392ModeControl( pDrv, pDev->slicMode );

   XDRV_LOG_INFO(( XDRV_LOG_MOD_XDRV, "XDRV_SLIC %d: Boosted loop current mode change (%d).", pDev->chan, value));

   return ( 0 );
}


/*
*****************************************************************************
** FUNCTION:   OpenSlic
**
** PURPOSE:    Open the SLIC
**
** PARAMETERS: pDev - pointer to the Si32392 SLIC info structure
**
** RETURNS:    none
**
*****************************************************************************
*/
static void OpenSlic( SLIC_6838_SI32392_DRV *pDev )
{
   int chan = pDev->chan;

   /* Initialize the SLIC registers for this channel */
   InitSlicRegs( pDev );

   /* Set initial SLIC state */
   MSPI_Mask_Write( pDev->spiDevId,  chan, SI32392_REG_LINEFEED_CONFIG_1,
                    SI32392_STATE_INIT << ( ( chan == 0 ) ? SI32392_CHAN0_MODE_SHIFT : SI32392_CHAN1_MODE_SHIFT ),
                    ( ( chan == 0 ) ? SI32392_CHAN0_MODE_MASK : SI32392_CHAN1_MODE_MASK ), 0 );

   /* Save the current requested SLIC mode */
   pDev->slicMode = XDRV_SLIC_MODE_LCF;

   /* Set hook state change flag to force initial update */
   pDev->bOffhookStateChange = XDRV_TRUE;
}


/*
*****************************************************************************
** FUNCTION:   CloseSlic
**
** PURPOSE:    Close the SLIC
**
** PARAMETERS: pDev  - pointer to the Si32392 SLIC info structure
**
** RETURNS:    none
**
*****************************************************************************
*/
static void CloseSlic( SLIC_6838_SI32392_DRV *pDev )
{
   int chan = pDev->chan;

   /* Set the SLIC to the appropriate disabled state */
   MSPI_Mask_Write( pDev->spiDevId,  chan, SI32392_REG_LINEFEED_CONFIG_1,
                    SI32392_STATE_DEINIT << ( ( chan == 0 ) ? SI32392_CHAN0_MODE_SHIFT : SI32392_CHAN1_MODE_SHIFT ),
                    ( ( chan == 0 ) ? SI32392_CHAN0_MODE_MASK : SI32392_CHAN1_MODE_MASK ), 0 );
}


/*
*****************************************************************************
** FUNCTION:   InitSlicRegs
**
** PURPOSE:    Initialize the SLIC registers and RAM
**
** PARAMETERS: pDev - pointer to the Si32392 SLIC info structure
**
** RETURNS:    none
**
*****************************************************************************
*/
static void InitSlicRegs( SLIC_6838_SI32392_DRV *pDev )
{
   int chan = pDev->chan;
   XDRV_UINT16 regData;

   /* Clear interrupts */
   MSPI_Write( pDev->spiDevId,  chan, SI32392_REG_DEVICE_STATUS, 0x0000 );

   /*
   ** At this point we do not need to initialize register 1, as when line 2
   ** gets initialized, we will crash line 1.  Both lines share
   ** the same register.
   */
   if(( pDev->chan % 2 ) == 0 )
   {
      MSPI_Write( pDev->spiDevId,  chan, SI32392_REG_LINEFEED_CONFIG_1, 0x0000 );
   }

   /* Control registers */
   MSPI_Write( pDev->spiDevId, chan, SI32392_REG_LINEFEED_CONFIG_2, 
               SI32392_RINGTRIP_DETECT_THRESH_80 | SI32392_INTERRUPT_ENABLED |
               SI32392_CHAN0_TEST_LOAD_26K | SI32392_CHAN1_TEST_LOAD_26K );
   MSPI_Write( pDev->spiDevId, chan, SI32392_REG_LINEFEED_CONFIG_3, SI32392_TEST_NORMAL_OPERATION );

   /* Enable extended operation */
   regData = MSPI_Read( pDev->spiDevId, chan, SI32392_REG_LINEFEED_CONFIG_3 );
   regData |= SI32392_EXT_REG;
   MSPI_Write( pDev->spiDevId, chan, SI32392_REG_LINEFEED_CONFIG_3, regData );

   /* Enable independent ringing */
   regData = MSPI_Read( pDev->spiDevId, chan, SI32392_REG_LINEFEED_CONFIG_7 );
   regData |= SI32392_IND_RING_ENABLED;
   MSPI_Write( pDev->spiDevId, chan, SI32392_REG_LINEFEED_CONFIG_7, regData );

   /* Update VABTRIM values */
   regData = MSPI_Read( pDev->spiDevId, chan, SI32392_REG_LINEFEED_CONFIG_6 );
   regData &= ~( SI32392_VABTRIM1_MASK | SI32392_VABTRIM2_MASK );
   regData |= (( SI32392_VABTRIM_13V << SI32392_VABTRIM1_SHIFT ) |
               ( SI32392_VABTRIM_13V << SI32392_VABTRIM2_SHIFT ));
   MSPI_Write( pDev->spiDevId, chan, SI32392_REG_LINEFEED_CONFIG_6, regData );

   /* Disable extended operation */
   regData = MSPI_Read( pDev->spiDevId, chan, SI32392_REG_LINEFEED_CONFIG_3 );
   regData &= ~SI32392_EXT_REG;
   MSPI_Write( pDev->spiDevId, chan, SI32392_REG_LINEFEED_CONFIG_3, regData );
}


/*
*****************************************************************************
** FUNCTION:   setSlicState
**
** PURPOSE:    Set SLIC state and related registers
**
** PARAMETERS: pDev - pointer to the Si32392 SLIC info structure
**             state - new slic state
**             mode - new slic mode
**
** RETURNS:    none
**
*****************************************************************************
*/
static void setSlicState( SLIC_6838_SI32392_DRV *pDev, int state, XDRV_SLIC_MODE mode )
{
   XDRV_UINT16 slicStateReg;
   XDRV_UINT16 slicControlReg;
   XDRV_UINT16 currentTemp;
   XDRV_UINT16 requiredTemp;
   XDRV_UINT16 mask;
   XDRV_UINT16 shift;
   int chan = pDev->chan;

   /* Retrieve old data */
   slicStateReg = MSPI_Read( pDev->spiDevId,  chan, SI32392_REG_LINEFEED_CONFIG_1 );
   slicControlReg = MSPI_Read( pDev->spiDevId,  chan, SI32392_REG_LINEFEED_CONFIG_3 );

   /* Update the register value */
   mask = ( ( chan % 2 ) == 0 ) ? SI32392_CHAN0_MODE_MASK : SI32392_CHAN1_MODE_MASK;
   shift = ( ( chan % 2 ) == 0 ) ? SI32392_CHAN0_MODE_SHIFT : SI32392_CHAN1_MODE_SHIFT;
   slicStateReg &= ( ~mask );
   slicStateReg |= ( ( state << shift ) & mask );

   if( slicControlReg & SI32392_TEST_TSDT )
   {
      currentTemp = 1;
   }
   else
   {
      currentTemp = 0;
   }

   if((( slicStateReg & SI32392_CHAN0_MODE_MASK ) == SI32392_CHAN0_MODE_AC_RING_TRIP ) ||
      (( slicStateReg & SI32392_CHAN0_MODE_MASK ) == SI32392_CHAN0_MODE_DC_RING_TRIP ) ||
      (( slicStateReg & SI32392_CHAN1_MODE_MASK ) == SI32392_CHAN1_MODE_AC_RING_TRIP ) ||
      (( slicStateReg & SI32392_CHAN1_MODE_MASK ) == SI32392_CHAN1_MODE_DC_RING_TRIP ))
   {
      requiredTemp = 1;
      slicControlReg |= SI32392_TEST_TSDT;
   }
   else
   {
      requiredTemp = 0;
      slicControlReg &= ~SI32392_TEST_TSDT;
   }

   /* Update HVG first if new SLIC mode is ringing */
   if( mode == XDRV_SLIC_MODE_RING )
   {
      /* Update the HVG parameters based on the current SLIC state */
      xdrvApmHvgUpdateSlicStatus( pDev->pApmDrv, chan, mode,
                                  pDev->bRingDCoffsetEnabled, pDev->bOnBattery,
                                  pDev->bBoostedLoopCurrent, XDRV_FALSE );

#if XDRV_CFG_BALANCED_RINGING
      /* Sleep to allow HVG to adapt due to changes in Si32392 SLIC performance */
      bosSleep( SI32392_RING_START_DELAY );
#endif

      /* Write the modified state data to the SLIC */
      MSPI_Write( pDev->spiDevId,  chan, SI32392_REG_LINEFEED_CONFIG_1, slicStateReg );

      /* If necessary update the temperature control data to the SLIC */
      if( requiredTemp != currentTemp )
      {
         MSPI_Write( pDev->spiDevId,  chan, SI32392_REG_LINEFEED_CONFIG_3, slicControlReg );
      }
   }
   else
   {
      /* Write the modified state data to the SLIC */
      MSPI_Write( pDev->spiDevId, chan, SI32392_REG_LINEFEED_CONFIG_1, slicStateReg );

      /* If necessary update the temperature control data to the SLIC */
      if( requiredTemp != currentTemp )
      {
         MSPI_Write( pDev->spiDevId, chan, SI32392_REG_LINEFEED_CONFIG_3, slicControlReg );
      }

      /* Update the HVG parameters based on the current SLIC state */
      xdrvApmHvgUpdateSlicStatus( pDev->pApmDrv, chan, mode,
                                  pDev->bRingDCoffsetEnabled, pDev->bOnBattery,
                                  pDev->bBoostedLoopCurrent, XDRV_FALSE );
   }
}


/*
******************************************************************************
** FUNCTION:  MSPI_Read
**
** PURPOSE:   Read from a memory address via MSPI
**
** PARAMETERS:
**            chan  - the line number ( 0 referenced )
**            addr  - address of the control register
**
** RETURNS:
**            data  - 8-bit data for the control register
**
*****************************************************************************
*/
static XDRV_UINT16 MSPI_Read( int spiDevId,  int chan, XDRV_UINT8 addr )
{
   XDRV_UINT16    rxData;
   XDRV_UINT8     txBuffer[4] = { 0, 0, 0, 0 };
   XDRV_UINT8     rxBuffer[4] = { 0, 0, 0, 0 };

   /* Place the command/address and data into the lower byte of each txBuffer */
   txBuffer[0] = ( addr << 5 ) | SI32392_READ_CMD;
   txBuffer[1] = 0x00;
   txBuffer[2] = 0x00;
   txBuffer[3] = 0x00;

   /* Write to the MSPI */
   if ( BcmSpiSyncTrans(txBuffer, rxBuffer, 0, 4, MSPI_BUS_ID, spiDevId) )
   {
      XDRV_LOG_ERROR(( XDRV_LOG_MOD_XDRV, "MSPI_Read Error"));
   }

   /* Process the returned values */
   rxData = (((( XDRV_UINT16 )rxBuffer[2] << 8 ) | (( XDRV_UINT16 )rxBuffer[3] )) & 0x0FFF );
   return( rxData );
}

#if 0
/*
******************************************************************************
** FUNCTION:  MSPI_Read_Status
**
** PURPOSE:   Read status bits from message preamble via MSPI
**
** PARAMETERS:
**            chan  - the line number ( 0 referenced )
**            addr  - address of the control register
**
** RETURNS:
**            data  - 8-bit data for the control register
**
*****************************************************************************
*/
static XDRV_UINT16 MSPI_Read_Status( int spiDevId, int chan, XDRV_UINT8 addr )
{
   /* unsigned long  numTxFrame; */
   
   XDRV_UINT16    rxData;
   XDRV_UINT8     txBuffer[4] = { 0, 0, 0, 0 };
   XDRV_UINT8     rxBuffer[4] = { 0, 0, 0, 0 };

   /* Place the command/address and data into the lower byte of each txBuffer */
   txBuffer[0] = ( addr << 5 ) | SI32392_READ_CMD;
   txBuffer[1] = 0x00;
   txBuffer[2] = 0x00;
   txBuffer[3] = 0x00;

   if ( BcmSpiSyncTrans(txBuffer, rxBuffer, 0, 4, MSPI_BUS_ID, spiDevId) )
   {
      XDRV_LOG_ERROR(( XDRV_LOG_MOD_XDRV, "MSPI_Read Error"));
   }

   /* Process the returned values */ 
   rxData = (((( XDRV_UINT16 )rxBuffer[0] << 4 ) | (( XDRV_UINT16 )rxBuffer[1] >> 4 )) & 0x0FFF );
   return( rxData );
}
#endif

/*
*****************************************************************************
** FUNCTION:  MSPI_Write
**
** PURPOSE:   Write to a memory address via MSPI
**
** PARAMETERS:
**            chan  - the line number ( 0 referenced )
**            addr  - memory address to be written to
**            data  - 16-bit data to be written
**
** RETURNS:   nothing
**
*****************************************************************************
*/
static void MSPI_Write( int spiDevId,  int chan, XDRV_UINT8 addr, XDRV_UINT16 data )
{
   XDRV_UINT8     txBuffer[2];
   XDRV_UINT8     dummy[2];

   /* Place the command/address and data into the lower byte of each txBuffer */
   txBuffer[0] = ( (( XDRV_UINT8 )( data >> 8 )) & 0x0F ) | ( addr << 5 ) | SI32392_WRITE_CMD;
   txBuffer[1] = ( XDRV_UINT8 )data;

   /* Write to the MSPI */
   if ( BcmSpiSyncTrans(txBuffer, dummy, 0, 2, MSPI_BUS_ID, spiDevId) )
   {
      XDRV_LOG_ERROR(( XDRV_LOG_MOD_XDRV, "MSPI_Write Error"));
   }
}


/*
*****************************************************************************
** FUNCTION:  MSPI_Write_Verify
**
** PURPOSE:   Write to a memory address via MSPI and verify the write
**
** PARAMETERS:
**            chan - the line number ( 0 referenced )
**            addr - memory address to be written to
**            data - 8-bit data to be written
**
** RETURNS:   0 on success, 1 otherwise
**
*****************************************************************************
*/
static int MSPI_Write_Verify( int spiDevId, int chan, XDRV_UINT8 addr, XDRV_UINT16 data )
{
   XDRV_UINT8     txBuffer[2];
   XDRV_UINT8     dummy[2];
   XDRV_UINT16    readData;


   /* Place the command/address and data into the lower byte of each txBuffer */
   txBuffer[0] = (((XDRV_UINT8)(data >> 8)) & 0x1f) | ((addr << 5) | SI32392_WRITE_CMD);
   txBuffer[1] = (XDRV_UINT8)data;

   /* Write to the MSPI */
   if ( BcmSpiSyncTrans(txBuffer, dummy, 0, 2, MSPI_BUS_ID, spiDevId) )
   {
      XDRV_LOG_ERROR(( XDRV_LOG_MOD_XDRV, "ERROR: MSPI_Write_Verify - Addr %d - Data 0x%02X ", addr, data));
   }

   /* Read the register that was just written to */
   readData = MSPI_Read( spiDevId,  chan, addr );

   /* Verify that the write operation was successful */
   if( readData != data )
   {
      XDRV_LOG_ERROR(( XDRV_LOG_MOD_XDRV, "ERROR: MSPI_Write_Verify - Addr %d - Data 0x%04X ", addr, data));
      return( 1 );
   }
   else
   {
      return( 0 );
   }
}

/*
*****************************************************************************
** FUNCTION:  MSPI_Mask_Write
**
** PURPOSE:   Write to the masked location of a memory address via MSPI
**            and verify the write if requested to
**
** PARAMETERS:
**            chan - the line number ( 0 referenced )
**            addr - memory address to be written to
**            data - 8-bit data to be written
**            mask - mask to the data to be written
**            verify - bool flag to verify the write
**
** RETURNS:   0 on success, 1 otherwise
**
*****************************************************************************
*/
static int MSPI_Mask_Write( int spiDevId, int chan, XDRV_UINT8 addr, XDRV_UINT16 data, XDRV_UINT16 mask, int verify )
{
   XDRV_UINT16 localData;

   /* Retrieve Old Data */
   localData = MSPI_Read( spiDevId,  chan, addr );
   /* Clear the masked location */
   localData &= ( ~mask );

   /* Add the new data */
   localData |= ( data & mask );

   /* Write the modified data to the SLIC */
   if ( verify )
   {
      return MSPI_Write_Verify( spiDevId, chan, addr, localData );
   }
   else
   {
      MSPI_Write( spiDevId, chan, addr, localData );
   }

   return 0;
}

/*
******************************************************************************
** FUNCTION:  slic6838Si32392RegisterRead
**
** PURPOSE:   Read from a memory address via MSPI
**
** PARAMETERS:
**            chan     - the line number ( 0 referenced )
**            addr     - address of the control register
**            data     - data read from the control register
**
** RETURNS:   none
**
*****************************************************************************
*/
void slic6838Si32392RegisterRead( SLIC_6838_SI32392_DRV *pDev, XDRV_UINT8 addr, XDRV_UINT16 *data )
{
   int chan = pDev->chan;;   
   *data = MSPI_Read( pDev->spiDevId,  chan, addr );
}

/*
******************************************************************************
** FUNCTION:  slic6838Si32392RegisterWrite
**
** PURPOSE:   Write to a memory address via MSPI
**
** PARAMETERS:
**            chan     - the line number ( 0 referenced )
**            addr     - address of the control register
**            data     - 8 or 16 bit data for the control register
**
** RETURNS:   none
**
*****************************************************************************
*/
void slic6838Si32392RegisterWrite( SLIC_6838_SI32392_DRV *pDev, XDRV_UINT8 addr, XDRV_UINT16 data )
{
   int chan = pDev->chan;   
   MSPI_Write( pDev->spiDevId,  chan, addr, data );
}

/*
******************************************************************************
** FUNCTION:  slic6838Si32392RegisterMaskWrite
**
** PURPOSE:   Write to the masked location of a memory address via MSPI
**            and verify the write if requested to
**
** PARAMETERS:
**            chan - the line number ( 0 referenced )
**            addr - memory address to be written to
**            data - 8-bit data to be written
**            mask - mask to the data to be written
**            verify - bool flag to verify the write
**
** RETURNS:   0 on success, 1 otherwise
**
*****************************************************************************
*/
void slic6838Si32392RegisterMaskWrite( SLIC_6838_SI32392_DRV *pDev, XDRV_UINT8 addr, XDRV_UINT16 data, XDRV_UINT16 mask, int verify )
{
   int chan = pDev->chan;   
   MSPI_Mask_Write(  pDev->spiDevId,  chan, addr, data, mask, verify );
}

/*
*****************************************************************************
** FUNCTION:   slic6838Si32392SetPowerSource
**
** PURPOSE:    This function notifies the SLIC driver of any changes in power
**             conditions (on battery or AC power)
**
** PARAMETERS: pDrv  - pointer to the SLIC driver device structure
**             value - 1 (battery power on)  = device operating on battery power
**                     0 (battery power off) = device operating on AC power
**
** RETURNS:    0 on success
**
*****************************************************************************
*/
static int slic6838Si32392SetPowerSource( XDRV_SLIC *pDrv, XDRV_UINT16 value )
{
   SLIC_6838_SI32392_DRV *pDev = (SLIC_6838_SI32392_DRV *)pDrv;
   int                   chan = pDev->chan;
   XDRV_UINT16           configRegister;

   /* Record AC power status */
   if( pDev->bEnhancedControl )
   {
      pDev->bOnBattery = value;
   }
   else if( value )
   {
      pDev->bOnBattery = XDRV_TRUE;
   }
   else
   {
      pDev->bOnBattery = XDRV_FALSE;
   }

   /* Enable enhance control */
   if ( pDev->bEnhancedControl )
   {
      if( value & XDRV_APM_ENHANCED_CONTROL_ENABLED )
      {
         configRegister = MSPI_Read( pDev->spiDevId,  chan, SI32392_REG_LINEFEED_CONFIG_2 );
         configRegister |= ( SI32392_CHAN0_ENHANCED_CONTROL_ON | SI32392_CHAN1_ENHANCED_CONTROL_ON );
         MSPI_Write( pDev->spiDevId,  chan, SI32392_REG_LINEFEED_CONFIG_2, configRegister );
      }
      else if ( value == 0 )
      {
         configRegister = MSPI_Read( pDev->spiDevId,  chan, SI32392_REG_LINEFEED_CONFIG_2 );
         configRegister &= ~( SI32392_CHAN0_ENHANCED_CONTROL_ON | SI32392_CHAN1_ENHANCED_CONTROL_ON );
         MSPI_Write( pDev->spiDevId,  chan, SI32392_REG_LINEFEED_CONFIG_2, configRegister );
      }
   }

   /* Reset SLIC mode to current state to force configuration updates */
   slic6838Si32392ModeControl( pDrv, pDev->slicMode );

   XDRV_LOG_INFO(( XDRV_LOG_MOD_XDRV, "XDRV_SLIC %d: Power Source mode change (%d).", pDev->chan, pDev->bOnBattery ));

   return ( 0 );
}

/*
*****************************************************************************
** FUNCTION:   slic6838Si32392SetSpecialMode
**
** PURPOSE:    This function notifies the SLIC to enable or disable
**             various special modes
**
** PARAMETERS: pDrv   - pointer to the SLIC driver device structure
**             mode   - special mode to enable or disable
**             enable - enable or disable
**
** RETURNS:    0 on success
**
*****************************************************************************
*/
int slic6838Si32392SetSpecialMode( SLIC_6838_SI32392_DRV *pDev, SLIC_6838_SI32392_SPECIAL_MODE mode, XDRV_BOOL enable )
{
   int chan = pDev->chan;
   XDRV_UINT16 regData;

   switch( mode )
   {
      case SLIC_6838_SI32392_SPECIAL_MODE_SHORT:
      {
         /* Enable extended operation */
         regData = MSPI_Read( pDev->spiDevId, chan, SI32392_REG_LINEFEED_CONFIG_3 );
         regData |= SI32392_EXT_REG;
         MSPI_Write( pDev->spiDevId, chan, SI32392_REG_LINEFEED_CONFIG_3, regData );

         /* Update appropriate flag */
         regData = MSPI_Read( pDev->spiDevId, chan, SI32392_REG_LINEFEED_CONFIG_4 );
         if( enable )
         {
            /* Enable short to ground */
            if( chan == 0 )
            {
               regData |= ( SI32392_TEST_SHORTCTRL1 );
            }
            else
            {
               regData |= ( SI32392_TEST_SHORTCTRL2 ) ;
            }
         }
         else
         {
            /* Disable short to ground */
            if( chan == 0 )
            {
               regData &= ~( SI32392_TEST_SHORTCTRL1 ) ;
            }
            else
            {
               regData &= ~( SI32392_TEST_SHORTCTRL2 ) ;
            }
         }
         MSPI_Write( pDev->spiDevId, chan, SI32392_REG_LINEFEED_CONFIG_4, regData );

         /* Disable extended operation */
         regData = MSPI_Read( pDev->spiDevId, chan, SI32392_REG_LINEFEED_CONFIG_3 );
         regData &= ~SI32392_EXT_REG;
         MSPI_Write( pDev->spiDevId, chan, SI32392_REG_LINEFEED_CONFIG_3, regData );
      }
      break;
      
      case SLIC_6838_SI32392_SPECIAL_MODE_RING_SHORT:
      {
         /* Enable extended operation */
         regData = MSPI_Read( pDev->spiDevId, chan, SI32392_REG_LINEFEED_CONFIG_3 );
         regData |= SI32392_EXT_REG;
         MSPI_Write( pDev->spiDevId, chan, SI32392_REG_LINEFEED_CONFIG_3, regData );

         /* Update appropriate flag */
         regData = MSPI_Read( pDev->spiDevId, chan, SI32392_REG_LINEFEED_CONFIG_4 );
         if( enable )
         {
            /* Enable short to ground */
            if( chan == 0 )
            {
               regData |= ( SI32392_TEST_SHORTCTRL1 | SI32392_TEST_SHORTR1 );
            }
            else
            {
               regData |= ( SI32392_TEST_SHORTCTRL2 | SI32392_TEST_SHORTR2 ) ;
            }
         }
         else
         {
            /* Disable short to ground */
            if( chan == 0 )
            {
               regData &= ~( SI32392_TEST_SHORTCTRL1 | SI32392_TEST_SHORTR1 ) ;
            }
            else
            {
               regData &= ~( SI32392_TEST_SHORTCTRL2 | SI32392_TEST_SHORTR2 ) ;
            }
         }
         MSPI_Write( pDev->spiDevId, chan, SI32392_REG_LINEFEED_CONFIG_4, regData );

         /* Disable extended operation */
         regData = MSPI_Read( pDev->spiDevId, chan, SI32392_REG_LINEFEED_CONFIG_3 );
         regData &= ~SI32392_EXT_REG;
         MSPI_Write( pDev->spiDevId, chan, SI32392_REG_LINEFEED_CONFIG_3, regData );
      }
      break;
      
      case SLIC_6838_SI32392_SPECIAL_MODE_TIP_SHORT:
      {
         /* Enable extended operation */
         regData = MSPI_Read( pDev->spiDevId, chan, SI32392_REG_LINEFEED_CONFIG_3 );
         regData |= SI32392_EXT_REG;
         MSPI_Write( pDev->spiDevId, chan, SI32392_REG_LINEFEED_CONFIG_3, regData );

         /* Update appropriate flag */
         regData = MSPI_Read( pDev->spiDevId, chan, SI32392_REG_LINEFEED_CONFIG_4 );
         if( enable )
         {
            /* Enable short to ground */
            if( chan == 0 )
            {
               regData |= ( SI32392_TEST_SHORTCTRL1 | SI32392_TEST_SHORTT1 ) ;
            }
            else
            {
               regData |= ( SI32392_TEST_SHORTCTRL2 | SI32392_TEST_SHORTT2 ) ;
            }
         }
         else
         {
            /* Disable short to ground */
            if( chan == 0 )
            {
               regData &= ~( SI32392_TEST_SHORTCTRL1 | SI32392_TEST_SHORTT1 ) ;
            }
            else
            {
               regData &= ~( SI32392_TEST_SHORTCTRL2 | SI32392_TEST_SHORTT2 ) ;
            }
         }
         MSPI_Write( pDev->spiDevId, chan, SI32392_REG_LINEFEED_CONFIG_4, regData );

         /* Disable extended operation */
         regData = MSPI_Read( pDev->spiDevId, chan, SI32392_REG_LINEFEED_CONFIG_3 );
         regData &= ~SI32392_EXT_REG;
         MSPI_Write( pDev->spiDevId, chan, SI32392_REG_LINEFEED_CONFIG_3, regData );
      }
      break;
      
      case SLIC_6838_SI32392_SPECIAL_DISABLE_RING_TRANISTION_MODE:
      {
         /* Set flag appropriately */
         pDev->disableRingMode = enable;
      }
      break;

      default:
      {
      }
      break;
   }

   return ( 0 );
}

/*
*****************************************************************************
** FUNCTION:   slic6838Si32392GetOverCurrentStatus
**
** PURPOSE:    Check to see if the over current protection has kicked in
**
** PARAMETERS: pDrv  - pointer to the SLIC driver device structure
**
** RETURNS:    XDRV_TRUE  - If the over current protection has kicked in
**             XDRV_FALSE - Otherwise
**
*****************************************************************************
*/
static XDRV_BOOL slic6838Si32392GetOverCurrentStatus( XDRV_SLIC *pDrv )
{
   SLIC_6838_SI32392_DRV *pDev = (SLIC_6838_SI32392_DRV *)pDrv;
   XDRV_APM *pApmDrv = pDev->pApmDrv;
   int chan = pDev->chan;

   return xdrvApmGetOverCurrentStatus( pApmDrv, chan );
}


/*
*****************************************************************************
** FUNCTION:   slic6838Si32392UpdateState
**
** PURPOSE:    Update Si32392 SLIC state
**
** PARAMETERS: pDev - pointer to the Si32392 SLIC info structure
**
** RETURNS:    none
**
*****************************************************************************
*/
void slic6838Si32392UpdateState( SLIC_6838_SI32392_DRV *pDev )
{
   if( pDev->pendingSlicTime > 0 )
   {
      if( pDev->pendingSlicTime > 10 )
      {
         /* Decrement counter by task rate */
         pDev->pendingSlicTime -= 10;
         return;
      }
      else
      {
      
         /* Timer expiring */
         pDev->pendingSlicTime = 0;

         /* Update SLIC state */
         setSlicState( pDev, pDev->pendingSlicState, pDev->pendingSlicMode );

         /* Save the current requested SLIC mode */
         pDev->slicMode = pDev->pendingSlicMode;

         /* Clear pending state information */
         pDev->pendingSlicState = 0;
         pDev->pendingSlicMode = 0;
      }
   }
}


/*
*****************************************************************************
** FUNCTION:   slic6838Si32392UpdateOffhookStatus
**
** PURPOSE:    Determine if a channel is on or off hook
**
** PARAMETERS: pDev - pointer to the SLIC driver device structure
**
** RETURNS:    none
**
*****************************************************************************
*/
void slic6838Si32392UpdateOffhookStatus( SLIC_6838_SI32392_DRV *pDev )
{
   pDev->bOffhookStateChange = XDRV_TRUE;
}

static XDRV_SINT16 slic6838Si32392GetDlp( XDRV_SLIC *pDrv )
{
   (void) pDrv;

   return( 0 );
}

static void slic6838Si32392ProcessEvents( XDRV_SLIC *pDrv )
{
   (void) pDrv;

   return;
}

/*
*****************************************************************************
** FUNCTION:   slic6838Si32392Probe
**
** PURPOSE:    To probe registers. (DEBUG ONLY)
**
** PARAMETERS: pDrv  - pointer to the SLIC driver device structure
**
** RETURNS:    none
**
*****************************************************************************
*/
static XDRV_UINT32 slic6838Si32392Probe( XDRV_SLIC *pDrv, XDRV_UINT32 deviceId, XDRV_UINT32 chan, XDRV_UINT32 reg,
                              XDRV_UINT32 regSize, void* value, XDRV_UINT32 valueSize, XDRV_UINT32 indirect, 
                              XDRV_UINT8 set )
{
   (void) pDrv;

   return (0);
}
