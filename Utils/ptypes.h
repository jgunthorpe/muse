// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   PTypes - Portable Types
   
   Here we define some usefull portable types, most noteably the C++ bool
   type for platforms that do not have it.
   
   If ever it becomes necessary to use types like u32 (unsigned 32 bit 
   type) and their friends this is were they should go. I do not have
   them here because I haven't needed them yet.
   
   This source is placed in the Public Domain, do with it what you will
   It was originally written by Jason Gunthorpe <jgg@gpu.srv.ualberta.ca>   
   
   ##################################################################### */
									/*}}}*/
#ifndef PTYPES_H
#define PTYPES_H

#if !defined(__GNUG__)
#define bool int
#define true 1
#define false 0
#endif

// This is something I picked up from SOM, 8bit unsigned data type
typedef unsigned char octet;

#endif
