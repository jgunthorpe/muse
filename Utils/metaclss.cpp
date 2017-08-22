// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   MetaClass - Metaclass framework for C++.
   
   This source is placed in the Public Domain, do with it what you will
   It was originally written by Jason Gunthorpe <jgg@gpu.srv.ualberta.ca>   
   
   ##################################################################### */
									/*}}}*/
// Includes         							/*{{{*/
#include <idlseq.h>
#include <metaclss.h>
   									/*}}}*/
// Instantiate the templates
template class Sequence<MetaClass *>;

// MetaObject::Meta instantiation					/*{{{*/
// ---------------------------------------------------------------------
/* The C++ Standard dictates that the staticly inilialized constant object
   must be set before construction of any global constructors. We can
   use this fact to advoid library initilization hacks. The MetaNewer
   class is a dummy class to force the construction of the metaclass for
   MetaObject incase no-one else gets called first. The constructor
   for MetaClass will construct this object any time a base is created.
   All this allows the registration system to work.
 */
MetaClass *MetaObject::Meta = 0;
SequenceMetaClass *MetaClass::ClassList = 0;
class MetaNewer
{
   public:
   MetaNewer()
   {
      if (MetaObject::Meta != 0)
      {
	 MetaObject::Meta = (MetaClass *)1;   // The constructor will build it otherwise.
	 MetaObject::Meta = new MetaClass;
      }
   }
};
static MetaNewer TheNewer;
   									/*}}}*/
// MetaClass::MetaClass - Performs metaclass registration               /*{{{*/
// ---------------------------------------------------------------------
/* As each metaclass is constructed it is registered here in the base
   classes constructor. We flush the derived class list of every metaclass
   in the system when this is called. */
MetaClass::MetaClass()
{
   // Construct the metaclass now.
   if (MetaObject::Meta == 0)
   {
      MetaObject::Meta = (MetaClass *)1; // The constructor will build it otherwise
      MetaObject::Meta = new MetaClass;
   }

   // Woo woo, we are the base base base class ;>
   if (MetaObject::Meta == (MetaClass *)1)
   {
      ClassList = new SequenceMetaClass;
      ClassList->reserve(20);
   }

   ClassList->push_back(this);

   // Flush the derived list
   for (MetaClass **I = ClassList->begin(); I != ClassList->end(); I++)
      (*I)->Derived.free();
}
									/*}}}*/
// MetaClass::DescendedFrom - true if the passed class is a baseclass   /*{{{*/
// ---------------------------------------------------------------------
/* Derived classes should implement a version of this that compares with
   their metaclass and calls thier bases class recusively */
bool MetaClass::DescendedFrom(MetaClass *Base)
{
   if (Base == MetaObject::Meta)
      return true;
   return false;
}
									/*}}}*/
// MetaClass::GetName - Returns the name of the classs			/*{{{*/
// ---------------------------------------------------------------------
/* Derived classes should implement a version of this that returns their
   name */
const char *MetaClass::GetName()
{
   return "MetaClass";
};
									/*}}}*/
// MetaClass::New - Constructs a new instance of the class      	/*{{{*/
// ---------------------------------------------------------------------
/* Returns a new instance of the associated class. Derived classes should
   override to new their own objects. */
MetaObject *MetaClass::New()
{
   return new MetaObject;
}
									/*}}}*/
// MetaClass::GetDerived - Get a list of derived classes		/*{{{*/
// ---------------------------------------------------------------------
/* The return format is a pointer to an array of meta classes, the end
   is marked with a null. This makes it easy to cast the list into 
   whatever this object is.. We could also redefine GetDerived in child
   classes to use a different return type. */
MetaClass **MetaClass::GetDerived()
{
   if (Derived.size() == 0)
   {
      for (MetaClass **I = ClassList->begin(); I != ClassList->end(); I++)
	 if (*I != this && (*I)->DescendedFrom(this) == true)
	    Derived.push_backv(*I);
      Derived.push_backv(0);
   }

   return Derived.begin();
}
									/*}}}*/
// MetaObject::~MetaObject() - Virtual destructor        		/*{{{*/
// ---------------------------------------------------------------------
/* This is not inlined so that the MetaObject class can have a base 
   modeule. Classes without any non inlined functions have their
   virtual tables and other things emitted for each compilation unit
   that references them (BAD) */
MetaObject::~MetaObject()
{
}
									/*}}}*/
