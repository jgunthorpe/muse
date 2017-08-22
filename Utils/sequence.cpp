// -*- mode: cpp; mode: fold -*-
// Description   							/*{{{*/
/* ######################################################################

   These are the global template instantiations for the macros defined in 
   sequence.h. For compatibility (and speed of compiling) this lib is built 
   to be compiled with automatic template instantiations disabled. Each 
   module that defines a class that has an associated sequence* will 
   include the proper template line to foce instantiation in it's base file.
   
   Note that template instantiation doesn't have to be disabled, it just
   can be.

   This source is placed in the Public Domain, do with it what you will
   It was originally written by Jason Gunthorpe <jgg@gpu.srv.ualberta.ca>   
   
   ##################################################################### */
									/*}}}*/
#include <sequence.h>
#include <metaclss.h>

template class Sequence<long>;
template class Sequence<short>;
template class Sequence<float>;
template class Sequence<bool>;
template class Sequence<char *>;
