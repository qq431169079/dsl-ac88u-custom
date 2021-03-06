/***************************************************************************
*    Copyright (c) 2000-2011 Broadcom                             
*                                                                           
*    This program is the proprietary software of Broadcom and/or
*    its licensors, and may only be used, duplicated, modified or           
*    distributed pursuant to the terms and conditions of a separate, written
*    license agreement executed between you and Broadcom (an Authorized     
*    License).  Except as set forth in an Authorized License, Broadcom      
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
*    AS IS AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,              
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
*    Filename: FlexiCalc.h
*
****************************************************************************
*    Description:
*
*    FlexiCalc output constants for the following inputs:
*
*       BCM6816                          
*       SLIC = Silicon Labs Si3239       
*       R1 = 300 ohms
*       R2 = 1000 ohms
*       C1 = 220 nano Farads 
*       DLP = -9dB
*       ELP = -4dB
*       HWDACgain = 0dB
*       HW_impedance = 680 ohms
*       Protection resistor = 10 ohms
*       Ringing frequency = 16.66 hertz
*       Ringing amplitude = 45Vrms
*
*    Flexicalc Version: 3.3
*
****************************************************************************/

#ifndef FLEXICALC_UK_3239_H
#define FLEXICALC_UK_3239_H

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
** Flexicalc Values Structure                                              **
****************************************************************************/

#if !VRG_COUNTRY_CFG_UK
#define flexicalcUKArchive3239 ((const APM6816_FLEXICALC_CFG*) NULL)
#else
const APM6816_FLEXICALC_CFG flexicalcUKArchive3239[] =
{
{
   0x3239,           /* Slic type: Si3239 */
   45,               /* Ring Voltage (RMS) */
   -9,               /* DLP - Decode Level Point (receive loss) in dB */
   -4,               /* ELP - Encode Level Point (transmitt loss) in dB */
   0x0007,           /* eq_rx_shft */
   0x000B,               /* eq_tx_shft */
   0,               /* eq_imp_resp */

   /*
   ** Y-filter Coefficients
   */
   1,               /* yfltr_en */
   {  /* IIR 2 Filter Coefficients */
      0x04966,   /* Y IIR2 filter b0 */
      0x061FA,   /* Y IIR2 filter b1 */
      0x04966,   /* Y IIR2 filter b2 */
      0x4E1DC,   /* Y IIR2 filter a1 */
      0xE28AB    /* Y IIR2 filter a2 */
   },
   0x77,               /* y_iir2_filter_shift */
   {  /* Fir Filter Coefficients */
      0x2EEB9,   /* YFIR1_VAL */
      0xA69AC,   /* YFIR2_VAL */
      0x1AE64,   /* YFIR3_VAL */
      0x1FAD8,   /* YFIR4_VAL */
      0xFB492,   /* YFIR5_VAL */
      0xE9F52,   /* YFIR6_VAL */
      0xF7E30,   /* YFIR7_VAL */
      0x0B67D,   /* YFIR8_VAL */
      0x0C5E8,   /* YFIR9_VAL */
      0xFE1AC,   /* YFIR10_VAL */
      0xF53D2,   /* YFIR11_VAL */
      0xFC436,   /* YFIR12_VAL */
      0x0821B,   /* YFIR13_VAL */
      0x07DBA,   /* YFIR14_VAL */
      0xFBBF4,   /* YFIR15_VAL */
      0xF7336,   /* YFIR16_VAL */
      0x0961A,   /* YFIR17_VAL */
      0xFBC8A    /* YFIR18_VAL */
   },
   0x06,               /* y_fir_filter_shift */
   0x7FFFF,               /* yfltr_gain */
   {0x52830},          /* y_iir1_filter[1] */
   0xA6,               /* y_iir1_filter_shift */

   /* Hybrid Balance Coefficients */
   7,               /* hybal_shft */
   {0x40A1, 0xB65C, 0x3373, 0x831A, 0x6025},    /* hybal_audio_fir[5] */
   {0x0000, 0x0000, 0x0000, 0x0000, 0x0000},    /* hybal_pm_fir[5] */
   1,               /* hybal_en */

   {  /* Rx Equalization Coefficents */
      0x58D6, 0xC748, 0x047B, 0xFAED, 0x01BE, 0xFE85, 0x0077, 0xFF32,
      0x002E, 0xFF70, 0x000D, 0xFF99, 0x000A, 0xFFBA, 0x0009, 0xFFC8,
      0x0004, 0xFFD8, 0x000F, 0xFFE2, 0x0004, 0xFFE7, 0x0009, 0xFFEF,
      0x0009, 0xFFF2, 0x0004, 0xFFF5, 0x0006, 0xFFF5, 0x0003, 0xFFF9,
      0x0003, 0xFFF3, 0x0003, 0xFFF6, 0x0001, 0xFFFA, 0x0004, 0xFFF9,
      0x0004, 0x0000, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
   },

   {  /* Tx Equalization Coefficents */
      0x421F, 0xF945, 0x0008, 0xFD8D, 0xFF8B, 0xFF35, 0xFFC9, 0xFF9F,
      0xFFE3, 0xFFC7, 0xFFEC, 0xFFDC, 0xFFF4, 0xFFE8, 0xFFFA, 0xFFEF,
      0xFFFE, 0xFFF3, 0xFFFF, 0xFFF5, 0x0000, 0xFFFB, 0x0003, 0xFFFD,
      0x0003, 0xFFFD, 0xFFFE, 0xFFFC, 0xFFFF, 0xFFFC, 0x0000, 0xFFFD,
      0x0000, 0xFFFD, 0x0001, 0xFFFE, 0x0001, 0xFFFE, 0x0001, 0xFFFF,
      0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
   },

   0x0004,               /* cic_int_shft */
   0x0004,               /* cic_dec_shft */
   0x7BA5,           /* asrc_int_scale */
   0x0A8F,           /* asrc_dec_scale */
   1,               /* vtx_pg */
   1,               /* vrx_pg */
   0,               /* hpf_en */
   6,               /* hybal_smpl_offset */

}
};
#endif

#ifdef __cplusplus
}
#endif

#endif  /* FLEXICALC_UK_3239_H */
