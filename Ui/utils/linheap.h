/* ########################################################################

   Linear Heap

   Special object heap that can be written to disk at high speed

   ########################################################################
*/
#ifndef LINHEAP_H
#define LINHEAP_H

template <class T> class LinearHeap
{
   union T_Type
   {
      T Value;
      T_Type *NextFree;

      operator T *() {return &Value;};
   };

   IDLSequence<T_Type *> Records;
   unsigned long BlockSize;
   T_Type *FreeList;

   void AddEmpty();

   public:

   // Allocate a new node
   T *New()
   {
      if (FreeList == 0)
         AddEmpty();

      T_Type *Temp = FreeList;
      FreeList = FreeList->NextFree;
      Temp->NextFree = 0;

      return *Temp;
   };

   /// Free a node
   void Free(T *Node)
   {
      T_Type *Item = (T_Type *)Node;
      memset(Item,0,sizeof(*Item));
      Item->NextFree = FreeList;
      FreeList = Item;
   };

   LinearHeap()
   {
      BlockSize = 500;
      Records.construct();
      FreeList = 0;
   };
   ~LinearHeap();
};

template <class T> LinearHeap<T>::~LinearHeap()
{
   for (IDLSequence<T_Type *>::iterator I = Records.begin(); I != Records.end(); I++)
      delete [] *I;
   Records.free();
};

template <class T> void LinearHeap<T>::AddEmpty()
{
   T_Type *Item;
   Records.push_backv(Item = new T_Type[BlockSize]);
   memset(Item,0,sizeof(*Item)*BlockSize);
   for (int I = 0; I != BlockSize - 1; I++)
      Item[I].NextFree = Item + I + 1;
   Item[I].NextFree = FreeList;
   FreeList = Item;
};

#endif
