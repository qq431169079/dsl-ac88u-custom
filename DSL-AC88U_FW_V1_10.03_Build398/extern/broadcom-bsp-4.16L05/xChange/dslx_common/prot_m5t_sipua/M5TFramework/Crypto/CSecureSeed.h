//==SDOC========================================================================
//==============================================================================
//
//     Copyright(c) 2005 M5T Centre d'Excellence en Telecom Inc. ("M5T")
//
//  NOTICE:
//   This document contains information that is confidential and proprietary
//   to M5T.
//
//   M5T reserves all rights to this document as well as to the Intellectual
//   Property of the document and the technology and know-how that it includes
//   and represents.
//
//   This publication cannot be reproduced, neither in whole nor in part, in
//   any form whatsoever without prior written approval by M5T.
//
//   M5T reserves the right to revise this publication and make changes at any
//   time and without the obligation to notify any person and/or entity of such
//   revisions and/or changes.
//
//==============================================================================
//==EDOC========================================================================
#ifndef MXG_CSECURESEED_H
//M5T_INTERNAL_USE_BEGIN
#define MXG_CSECURESEED_H
//M5T_INTERNAL_USE_END

//==============================================================================
//====  INCLUDES + FORWARD DECLARATIONS  =======================================
//==============================================================================


//-- M5T Global definitions
//---------------------------
#ifndef MXG_MXCONFIG_H
#include "Config/MxConfig.h"
#endif

//-- M5T Framework Configuration
//-------------------------------
#ifndef MXG_FRAMEWORKCFG_H
#include "Config/FrameworkCfg.h"
#endif

#if !defined(MXD_CRYPTO_SECURESEED_NONE)

//-- Data Members
//-----------------

//-- Interface Realized and/or Parent
//-------------------------------------
#if defined(MXD_CRYPTO_SECURESEED_INCLUDE)
    #include MXD_CRYPTO_SECURESEED_INCLUDE
#else
    #error "No implementation defined for CSecureSeed!!!"
#endif

MX_NAMESPACE_START(MXD_GNS)

//==============================================================================
//====  CONSTANTS + DEFINES  ===================================================
//==============================================================================

//==SDOC========================================================================
//== Class: CSecureSeed
//========================================
//<GROUP CRYPTO_CLASSES>
//
// Summary:
//   Class used to generate random bytes to seed a pseudo-random generator.
//
// Description:
//   Class used to generate random bytes to seed a pseudo-random generator. Once
//   generated, these byres are hashed by using SHA-1 to give a 20 byte seed.
//
//==EDOC========================================================================
class CSecureSeed : public MXD_CRYPTO_SECURESEED_CLASSNAME
{
//-- Published Interface
//------------------------
public:

    //==SDOC====================================================================
    //==
    //==   GenerateSeed
    //==
    //==========================================================================
    //
    //  Summary:
    //      Generates a seed.
    //
    //  Parameters:
    //      puSeedValue:
    //          Value of the generated seed.
    //
    //  Returns:
    //      - resS_OK
    //      - resFE_INVALID_ARGUMENT
    //      - resFE_FAIL
    //
    //  Description:
    //      Generates a seed 20 bytes long. The seed is generated by using
    //      OS-specific data.
    //
    //==EDOC====================================================================
    static mxt_result GenerateSeed(OUT uint8_t* puSeedValue);

// Hidden Methods
//-----------------
private:
#if defined(MXD_COMPILER_GNU_GCC)
    // Must be enclosed in the #if defined(MXD_COMPILER_GNU_GCC) to avoid
    // waring or error with other compiler where MXD_COMPILER_GNU_GCC has no
    // value while it is required for the comparison.
    #if (MXD_COMPILER_GNU_GCC == MXD_COMPILER_GNU_GCC_2_7)
        // GCC 2.7.2 complains about private destructor with no friends.
        friend class Foo;
    #endif
#endif

    //-- << Deactivated Constructors / Destructors / Operators >>
    //------------------------------------------------------------

    // Summary:
    //  Constructor.
    //-----------------------
    CSecureSeed();

    // Summary:
    //  Destructor.
    //----------------------
    virtual ~CSecureSeed();
};

inline
CSecureSeed::CSecureSeed()
 : MXD_CRYPTO_SECURESEED_CLASSNAME()
{
}

inline
CSecureSeed::~CSecureSeed()
{
}

inline
mxt_result CSecureSeed::GenerateSeed(OUT uint8_t* puSeedValue)
{
    return MXD_CRYPTO_SECURESEED_CLASSNAME::GenerateSeed(puSeedValue);
}

MX_NAMESPACE_END(MXD_GNS)

#endif // #if !defined(MXD_CRYPTO_SECURESEED_NONE)

#endif //-- #ifndef MXG_CSECURESEED_H

