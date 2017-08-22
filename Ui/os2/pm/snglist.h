/* ########################################################################

   Song List

   ########################################################################
*/
#ifndef SNGLIST_H
#define SNGLIST_H

#include "LinHeap.h"
#include "StrTable.h"

class NodeType;
class SongList
{
   public:

   // Node structure for the tree
   struct Node
   {
      Node *Parent;
      Node *Next;

      /* Pointer to the next link in the link chain, this is traversed on
         an erase to free all the linked nodes */
      Node *NextLink;

      /* Links can't have chilren so the child link is actually a pointer to
         the link'ed node */
      union
      {
         Node *Child;
         Node *Link;
      };

      /* Again, links can't have size so this is the compainion pointer to
         NextLink, producing a doubly linked link list */
      union
      {
         unsigned long Size;
         Node *PrevLink;
      };

      // General info
      char *Name;
      museSongInfo *Info;
      unsigned char TypeId;
      unsigned IsLink:1;         // 1 if this is a link
      unsigned TempKill:1;       /* Used during refershing to determine
                                    what has changed */
      unsigned Directory:1;      /* 1 if it's a directory, this is needed
                                    because dirs might be empty */
      unsigned Refreshed:1;      /* 1 if this node has ever been refreshed.
                                    if Directory == 1 and Refreshed == 0
                                    then the node should be refreshed before
                                    scanned down */
      unsigned Archived:1;       // 1 if this is an archived file
      unsigned Archive:1;        // 1 if this is an archive
      unsigned UnSorted:1;       // 1 if the sorter should be turned off
   };

   class iterator
   {
      public:
      Node *Cur;
      Node *Prev;

      void operator --(int);
      void operator ++(int) {Prev = Cur; Cur = Cur->Next;};
      operator Node *() {if (Cur == 0) return 0; if (Cur->IsLink == 0) return Cur; else return Cur->Link;};
      Node *operator ->() {return *this;};
      Node &operator *() {return **this;};
      Node *operator =(Node *Cur) {this->Cur = Cur;Prev = 0;return Cur;};
      iterator operator =(iterator I) {Cur = I.Cur;Prev = I.Prev;return I;};
      int operator ==(const Node *C) {return C == Cur;};
      int operator !=(const Node *C) {return C != Cur;};
      int operator !=(const iterator &C) {return C.Cur != Cur;};
      int operator ==(const iterator &C) {return C.Cur == Cur;};
      int IsLink() {return Cur->IsLink;};
      Node *TrueNode() {return Cur;};

      iterator(Node *Cur) : Cur(Cur), Prev(0) {};
      iterator() : Cur(0), Prev(0) {};
   };

   LinearHeap<Node> Nodes;
   LinearHeap<museSongInfo> Songs;
   StringTable Strings;
   IDLSequence<NodeType *> Types;
   IDLSequence<Node *> Disks;

   iterator Head;

   void Free(iterator Node);
   void KillNode(iterator Node);
   iterator New() {return Nodes.New();};
   museSongInfo *NewInfo() {return Songs.New();};
   iterator New(Node *Parent,char *Name,unsigned long Size,int TypeId,char Archived = 0,iterator Insert = 0);
   iterator Link(Node *Parent,Node *Target,iterator Insert = 0);
   iterator Search(Node *Parent,char *Name,iterator Start = 0,int Refresh = 0);
   iterator GetNode(char *Name,iterator Start = 0);
   iterator WildGetNode(char *Name,iterator Parent = 0,iterator Start = 0);
   void ReLocate(iterator &T);
   int Compare(Node *A,Node *B);
   void MakePath(Node *Base,char *Name);

   int Refresh(iterator A);
   SongList();
   ~SongList();

   private:
   void MakePathI(Node *Base,char *&Name);
   void WildHelper(char *S,char *E,iterator Parent,iterator Start);
};

class NodeType
{
   protected:
   SongList *Owner;

   public:
   char Append;
   typedef SongList::iterator iterator;
   typedef SongList::Node Node;

   // Redirectors
   inline char *New(char *String) {return Owner->Strings.New(String);};
   inline void Free(char *String) {Owner->Strings.Free(String);};
   inline unsigned long Hash(char *String) {return Owner->Strings.Hash(String);};
   inline unsigned long GetHash(char *String) {return Owner->Strings.GetHash(String);};
   inline int IsEqual(char *TableString,char *String,unsigned long Hsh)
   {
      if (TableString == 0 || String == 0)
         return 0;
      if (GetHash(TableString) == Hsh && strcmp(TableString,String) == 0)
         return 1;
      return 0;
   }

   inline void MakePath(Node *Base,char *Name) {Owner->MakePath(Base,Name);};
   inline iterator New(Node *Parent,char *Name,unsigned long Size,int TypeId,char Archived = 0,iterator Insert = 0) {return Owner->New(Parent,Name,Size,TypeId,Archived,Insert);};
   inline iterator Search(Node *Parent,char *Name,iterator Start = 0,int Refresh = 0) {return Owner->Search(Parent,Name,Start,Refresh);};
   inline iterator GetNode(char *Name,iterator Start = 0) {return Owner->GetNode(Name,Start);};
   inline iterator WildGetNode(char *Name,iterator Parent = 0,iterator Start = 0) {return Owner->WildGetNode(Name,Parent,Start);};
   inline museSongInfo *NewInfo() {return Owner->NewInfo();};
   inline iterator Link(Node *Parent,Node *Target,iterator Insert = 0) {return Owner->Link(Parent,Target,Insert);};
   inline void ReLocate(iterator &T) {Owner->ReLocate(T);};
   inline void Free(iterator Node) {Owner->Free(Node);};

   virtual int Refresh(iterator Head) {return 0;};

   // Archiver support
   virtual int IsType(const char *Name) {return 0;};
   virtual void ConfigNode(iterator Node) {return;};

   NodeType(SongList *Owner);
};

inline int SongList::Refresh(iterator A)
{
   if (A->TypeId == 0)
      return 1;
   return Types[A->TypeId]->Refresh(A);
};

#endif
