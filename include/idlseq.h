// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   Sequence - Header to assist in the use of IDL sequences

   Originaly this header was designed to be used with SOM's IDL files to
   provide a better way to make use of the standard CORBA sequence type.
   Since then it has grown to become a usefull low overhead vector class.
   This is very handy if it is not possible to bring in the rather large
   STL, it also has fewer problems with non pointer types and the like.
   In alot of cases this type will work better than the stl-vector type,
   especially when the storage type is a structure.
   
   It is in very many ways a super enhanced new ?[] function.

   Instances should be created using the syntax
      template class Sequence<object>;
   With 1 of those lines per used object ONCE. The usuall custom I have 
   followed is to have the compilation unit of the object for which the
   sequence is being instantiated do this. ie FormatBs.cpp has a line
   for museFormatClass. This allows compilers like GCC that do not support
   comdef based templates to work properly.
   
   ##################################################################### */
									/*}}}*/
#ifndef IDLSEQ_H
#define IDLSEQ_HH

#include <string.h>              // memcpy
#include <stdlib.h>

/* IDL Sequence Template

   _length is the # of items stored in the sequence.
   _maximum is the # of bytes that are allocated for use by the sequence.
*/
template <class T> struct Sequence
{
   public:

   // STL Types
   typedef T value_type;
   typedef T *pointer;
   typedef T *iterator;
   typedef T const *const_iterator;
   typedef T &reference;
   typedef T const &const_reference;
   typedef unsigned long size_type;

   size_type _maximum;
   size_type _length;
   T *_buffer;

   /* Takes the place of the constructor. Zeros the members. It is assumed
      they are garbage. If they have meaningfull values then use Free(). */
   void construct()
   {
      _maximum = 0;
      _length = 0;
      _buffer = 0;
   };

   // Free the memory held by the sequence. This also calls Construct
   void free()
   {
      if ((_buffer != 0) && (_maximum != 0))
         ::free(_buffer);
      construct();
   };

   // We will empty the sequence during construction.
   Sequence()
   {
      construct();
   };
   
   // Assignment operator.
   Sequence<T> &operator =(Sequence<T> &From)
   {
      _buffer = From._buffer;
      _length = From._length;
      _maximum = From._maximum;
      return *this;
   };

   /* STL Style access, iterator looks just like a pointer. T* should be an
      iterator type. */
   iterator begin() {return _buffer;};
   iterator end() {return _buffer + _length;};
   const_iterator begin() const {return _buffer;};
   const_iterator end() const {return _buffer + _length;};
   size_type size() const {return _length;};
   size_type capacity() const {return _maximum/sizeof(T);};
   int empty() const       // Should return bool, but I have no bool type!
   {
      if (_length == 0)
         return 1;
      return 0;
   };
   void erasefrom(const_iterator Pos) {_length = Pos - begin();};
   void erase(iterator Item);
   void insert(reference New,iterator Item);

   reference front() {return *_buffer;};
   const_reference front() const {return *_buffer;};
   reference back() {return *(_buffer + _length - 1);};
   const_reference back() const {return *(_buffer + _length - 1);};

   // Add an item to the sequence (at the end)
   void push_back(const_reference Item) {operator [](_length) = Item;};
   void push_backv(value_type Item) {operator [](_length) = Item;};

   // Remove the first item from the sequence
   int pop_front(reference Item)
   {
      if (size() > 0)
      {
         Item = front();
         erase(begin());
         return 1;
      }
      return 0;
   };

   /* Allocate an amount of memory for XX items (includes existing items)
      _length remains unchanged by this */
   void reserve(size_type Size);

   // Copy all of the items in From into a new memory block in *this
   void Duplicate(Sequence<T> &From);
   void DuplicateNoSize(Sequence<T> &From);   // For inout parameter

   // This can resize (no index is invalid, so be careful) 0 is the lowest
   reference operator [](size_type Index)
   {
      // Check for resize
      if (Index >= _length)
      {
         if (Index*sizeof(T) >= _maximum)
         {
            if (Index + 1 > _length*2)
               reserve(Index + 1);
            else
               reserve(_length*2);
         }
         _length = Index + 1;
      }
      return _buffer[Index];
   };

   // This will never resize
   reference operator [](size_type Index) const
   {
      return _buffer[Index];
   };
};

template <class T>
void Sequence<T>::insert(/*reference*/ T &New,/*iterator*/ T *Item)
{
   if (Item > end())
      return;

   if (Item == end())
   {
      push_back(New);
      return;
   }

   int Pos = (Item - begin());
   operator [](_length);

   iterator I;
   for (I = end(); I != begin() + Pos; I--)
      *I = I[-1];
   *I = New;
}

template <class T>
void Sequence<T>::erase(/*iterator*/ T *Item)
{
   if (Item >= end())
      return;

   if (Item == end() - 1)
   {
      erasefrom(Item);
      return;
   }

   memcpy(Item,Item+1,(end() - Item - 1)*sizeof(T));
   _length--;
}

template <class T>
void Sequence<T>::reserve(/*size_type*/ unsigned long Size)
{
   // Check if a resize is needed.
   if ((Size > _length) || (Size > capacity()))
   {
      pointer Temp = (pointer)calloc(Size,sizeof(T));

      if (Temp == 0)
         return;

      memcpy(Temp,_buffer,_length*sizeof(T));

      ::free(_buffer);

      /* Reset the variables, _length does not change because no items
         were added. Only operator [] can change _length. */
      _buffer = Temp;
      _maximum = Size*sizeof(T);
   }
};

template <class T>
void Sequence<T>::Duplicate(Sequence<T> &From)
{
   // Check for an empty source sequence
   if (From._length == 0)
   {
      _length = 0;
      return;
   }

   // See if we need an alloc
   if (From._length > _length)
   {
      pointer Temp = (pointer)calloc(From._length,sizeof(T));

      if (Temp == 0)
         return;

      ::free(_buffer);
      _buffer = Temp;
      _maximum = From._length*sizeof(T);
   };
   _length = From._length;

   memcpy(_buffer,From._buffer,_length*sizeof(T));
};

template <class T>
void Sequence<T>::DuplicateNoSize(Sequence<T> &From)
{
   // Check for an empty source sequence
   if (From._length == 0)
   {
      _length = 0;
      return;
   }

   // See if we need an alloc
   if (From._length > _length)
   {
      // Can't alloc, so copy as much as possible.
      _length = capacity();
   }
   else
      _length = From._length;

   memcpy(_buffer,From._buffer,_length*sizeof(T));
};

// These are common sequence types
#define SequenceLong Sequence<long>
#define SequenceShort Sequence<short>
#define SequenceFloat Sequence<float>
#define Sequencebool Sequence<bool>
#define SequenceString Sequence<char *>
       	 
#endif
