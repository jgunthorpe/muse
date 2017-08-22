// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   MetaClass - Metaclass framework for C++.
   
   This provides a basic metaclass framework for C++ which provides:
      - Simple RTTI
      - Class registration
      - Class Catoragization
      - Versioning
      - By name instantiation
   
   It is used to keep track of what classes are registered in the system.
   When the program loads all of the metaclasses are created by the 
   compiler and are instantiated. They are then added to a global meta
   class list. If a list of all specific subtypes (say FileFormats) is 
   desired then the master list can be queries for all derived types.
   
   The system depends on a 'meta class' which is a single instance class
   that each class has. Sometimes referred to as the class of the class.
   This single instance class contains various bits of important information
   about it's twin class. It is instantiated at runtime and can be accessed
   by the syntax:
      MyClass::Meta            For static references
      MyClass->GetMetaClass()  For instantiated classes.
   
   To aid in this all classes are derived from the master object MetaObject
   which provides the necessary virtual functions to access the metaclass 
   object once a class has been instantiated.
   
   Also a metaclass member GetDerived is provided, this generates a list
   of derived metaclasses. The list is internally cached so it is safe to 
   call this function many times. Note that it isn't terribly thread safe
   iff you are adding metaclasses during the normal course of the program
   which is rare.
  
   This source is placed in the Public Domain, do with it what you will
   It was originally written by Jason Gunthorpe <jgg@gpu.srv.ualberta.ca>   
   
   ##################################################################### */
									/*}}}*/
#ifndef METACLSS_H
#define METACLSS_H

#include <ptypes.h>

class MetaObject;
class MetaClass;

#define SequenceMetaClass Sequence<MetaClass *>

// Metaclass base class
class MetaClass
{
   private:
   // List of all Classes in the system
   static SequenceMetaClass *ClassList;
   
   protected:
   long MajorVersion;
   long MinorVersion;

   // List of derived children
   SequenceMetaClass Derived;
   
   public:

   inline void GetVersion(long *Major,long *Minor)
   {
      *Major = MajorVersion;
      *Minor = MinorVersion;
   };

   virtual const char *GetName(); 
   virtual bool DescendedFrom(MetaClass *Base);
   virtual MetaObject *New();
   inline void GetClassList(SequenceMetaClass &Classes) {Classes.Duplicate(*ClassList);};
   MetaClass **GetDerived();
   
   MetaClass();
   virtual ~MetaClass() {};
};

// Object base class
class MetaObject
{
   public:
   static MetaClass *Meta;

   virtual MetaClass *GetMetaClass() {return Meta;};
   
   virtual ~MetaObject();
};

#endif
