// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   MinMax - These are defines for min/max/bound. Usefull range checking
            macros.
   
   They expand to compiler intrinsics whenever possible.
   
   This source is placed in the Public Domain, do with it what you will
   It was originally written by Jason Gunthorpe <jgg@gpu.srv.ualberta.ca>   
   
   ##################################################################### */
									/*}}}*/
#ifndef MAXMIN_H
#define MAXMIN_H

// Use High C++'s intrinsic min/max function
#if defined(__HIGHC__)
#define min(x,y) _min(x,y)
#define max(x,y) _max(x,y)
#endif

// GNU C++ has a min/max operator <coolio>
#if defined(__GNUG__)
#define min(A,B) ((A) <? (B))
#define max(A,B) ((A) >? (B))
#endif

/* We have to have one.. Borland has problems with this, their RTL 
   tends to #define minmax with a template */
#if !defined(min) && !defined(__BORLANDC__)
#define min(A,B) ((A) < (B)?(A):(B))
#define max(A,B) ((A) > (B)?(A):(B))
#endif

/* Bound functions, bound will return the value b within the limits a-c
   bounv will change b so that it is within the limits of a-c. 
*/
#define bound(a,b,c) min(c,max(b,a))
#define boundv(a,b,c) b = bound(a,b,c)

#endif
