// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   Errors - Definition of errors. These can be or'd together to form
   a more complete description of the error, a 16 bit data word can
   also be or'd with the data to give a total description of the
   failure.

   ##################################################################### */
									/*}}}*/
#ifndef ERRORS_H
#define ERRORS_H

#define LOADPART_Header        0x00100000
#define LOADPART_Samples       0x00200000
#define LOADPART_Instruments   0x00300000
#define LOADPART_Patterns      0x00400000
#define LOADPART_Message       0x00500000
#define LOADPART               0x00F00000

#define LOADFAIL_Truncated     0x00010000
#define LOADFAIL_Sigcheck      0x00020000
#define LOADFAIL_Corrupt       0x00030000
#define LOADFAIL_Notsupported  0x00040000
#define LOADFAIL               0x000F0000

#define LOADNUM_None           0x0000FFFF
#define LOADNUM_Part1          0x00000000
#define LOADNUM_Part2          0x00008000
#define LOADNUM                0x0000FFFF
#define LOADNUM_Num            0x00007FFF

#define LOADOK                 0

#define PLAYFAIL_NotLoaded     0x00010000

#define PLAYOK                 0

#define FAIL_NotLoaded         0x00010000
#define FAIL_NotSupported      0x00020000
#define FAIL_None              0

#endif
