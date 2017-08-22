// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   Linked List - A very lightweight linked list class.

   This class was created because STL's linked list class is too 'deep'
   to compile with GCC and auto template instantiation. Also this class
   implements a singly linked list while STL uses a doubly linked list.
   Why? Because it was ment to store gobs of small items where the 
   primary operation is a search or a push_front. It was also ment to
   have a tiny inplace foot print (4 bytes). So it is cheap to instatiate
   in structures. It's main reason to exist is to replace Sequence for 
   cases when it is undesirable to have empty memory.
   
   It looks very much like it's STL counterpart - list, so either can
   be dropped in at will.
   
   The iterator class is clever and keeps track of the previous item so
   a single -- is very fast, this simplifies alot of operatoins.
   
   This source is placed in the Public Domain, do with it what you will
   It was originally written by Jason Gunthorpe <jgg@gpu.srv.ualberta.ca>   
   
   ##################################################################### */
									/*}}}*/
#ifndef LLIST_H
#define LLIST_H

template <class T> class LList
{
   // This is the individual list node 
   struct Node
   {
      Node *Next;
      T Data;
      T &operator *() {return Data;};

      Node(T const &Data) : Next(0), Data(Data) {};
      Node(T const &Data,Node *Next) : Next(Next), Data(Data) {};
   };

   Node *Head;
   
   public:

   /* This is the iterator class for the linked list.
      It has ++,--,*,==,!= for operators. Beware, postfix and prefix
      ++ operators are the same!
   */
   class iterator
   {
      friend LList<T>;
      
      LList<T> const *List;
      Node *Cur;
      Node *Last;
    
      public:

      iterator &operator ++() {Cur = Cur->Next; return *this;};
      inline iterator &operator ++(int) {return operator ++();};

      // GCC cant handle this outside the class
      iterator &iterator::operator --()
      {
	 // Use the cached value if possible.
	 if (Last != 0)
	 {
	    Cur = Last;
	    Last = 0;
	    return *this;
	 }
	 
	 // Cur is at the front, maybe this should set cur to 0?
	 if (Cur == List->Head)
	    return *this;
	 
	 
	 Node *Find = Cur;
	 Cur = List->Head;
	 Last = 0;
	 for (; Cur != 0 && Cur->Next != Find; Cur = Cur->Next)
	    Last = Cur;
	 
	 // Hm, we couldnt find it.. This should throw an exception
	 if (Cur == 0)
	    abort();
	 
	 return *this;
      };
      inline iterator &operator --(int) {return operator --();};
      
      // These compare the position of the iterator, not the contents
      int operator ==(iterator const &I) const {return I.Cur == Cur;};
      int operator !=(iterator const &I) const {return I.Cur != Cur;};
	 
      // Dereference operators
      T &operator *() const {return **Cur;};
      T *operator ->() const {return &Cur->Data;}; 
      
      iterator() : Cur(0), Last(0) {};
      iterator(LList<T> const *List,Node *Cur) : List(List), Cur(Cur), Last(0) {};
   };   
   friend iterator;
   
   typedef iterator const_iterator;
   
   T &front() {return **Head;};
   
   // Insert an item at the front
   void push_front(T &Item) {Head = new Node(Item,Head);};

   // Discard the front item
   void pop_front() {Node *Tmp = Head; Head = Head->Next; delete Tmp;};

   // Return true if the list is empty
   bool empty() const {return Head == 0?true:false;};
      
   // Usual STL iterator notation
   iterator begin() {return iterator(this,Head);};
   iterator end() {return iterator(this,0);};

   // Insert so that *Spot = Item and *Spot++ = OldItem.
   bool insert(T const &Item,iterator &Spot)
   {
      if (Spot.Cur == 0 && Head == 0)
      {
	 Head = new Node(Item);
	 Spot.Cur = Head;
	 Spot.Last = 0;
	 return true;
      }
      
      if (Spot.Cur == 0)
	 return false;
      Spot.Cur->Next = new Node(Item,Spot.Cur->Next);
      Spot++;

      return true;
   };

   // Dump all the items.
   void free();
   
   // Construct/destruct
   LList() : Head(0) {};
   ~LList() {free();};
};

// The destructor will erase all of the items.
template <class T> void LList<T>::free()
{
   if (Head == 0)
      return;
   
   // Go through and free each item.
   for (Node *Next; Head != 0;)
   {
      Next = Head->Next;
      delete Head;
      Head = Next;
   }   
};

#endif
