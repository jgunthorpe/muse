/* ########################################################################

   SongList

   Implements a brother-father-son tree with a heap based reference counted
   string pool.

   The class has been designed for rapid writing to disk and reading from
   disk while still using effecient use of memory. Each string is allocated
   from the string table, and each node from the node table. On a disk
   write all pointers are translated to file relative offsets, in memory
   and on read the relative offsets are converted back to pointers. This
   allows the entire tree to be written in two writes.

   Type values allow the nodes to reference code to deal with them in
   helper objects. This allows ZIP files and other neat tricks.

   25000 items uses approximately 2M of storage.

   ########################################################################
*/
#ifndef SONGLST_H
#define SONGLST_H

#pragma pack(1)
struct Song
{
   Song *Parent;
   Song *Child;
   Song *Next;

   string Name;
   unsigned long Size;
   museSongInfo Info;
   unsigned char TypeId;
};

struct StringTabItem
{
   union
   {
      unsigned long Hash;
      StringTabItem *Next;
   };
   unsigned char End;
   unsigned char References;
   unsigned long Length;
   unsigned long PrevLen;
   char Str[1];                    // Actually Length Bytes long, 1 to help the compiler
};
#define EndL (1 << 0)
#define EndR (1 << 1)
#pragma pack()

class SongTree;
class SongType;

class SongTree
{
   unsigned long SongBlockSize;
   unsigned long StrBlockSize;

   Song *FreeList;
   StringTabItem *FreeStrings;

   void MakeFreeSongs();
   void MakeFreeString();
   void UnlinkString(StringTabItem *S);
   void KillNode(Song *S);
   void MakePathI(Song *Base,char *&Name);

   public:

   Song *Head;
   IDLSequence<Song *> Songs;
   IDLSequence<char *> StringTable;
   IDLSequence<SongType *> Types;

   char *GetString(unsigned long Size);
   void FreeString(char *String);
   char *GetString(char *String);
   unsigned long Hash(char *String);
   inline unsigned long GetHash(char *String)
   {
      return ((StringTabItem *)(String - sizeof(StringTabItem) + 1))->Hash;
   }

   Song *GetNewSong();
   void FreeSong(Song *Node);
   Song *FindPrev(Song *Node);

   void MakePath(Song *Base,char *Name);

   void Insert(Song *Parent,Song *Node);

   SongTree();
   ~SongTree();
};

class SongType
{
   protected:
   SongTree *Owner;

   public:
   unsigned long TypeId;
   char Append;

   // Redirectors
   inline char *GetString(unsigned long Size) {return Owner->GetString(Size);};
   inline void FreeString(char *String) {Owner->FreeString(String);};
   inline char *GetString(char *String) {return Owner->GetString(String);};
   inline unsigned long Hash(char *String) {return Owner->Hash(String);};
   inline Song *GetNewSong() {return Owner->GetNewSong();};
   inline void FreeSong(Song *Node) {Owner->FreeSong(Node);};
   inline Song *FindPrev(Song *Node) {return Owner->FindPrev(Node);};
   inline void MakePath(Song *Base,char *Name) {Owner->MakePath(Base,Name);};
   inline void Insert(Song *Parent,Song *Node) {Owner->Insert(Parent,Node);};
   inline unsigned long GetHash(char *String) {return Owner->GetHash(String);};

   virtual void Refresh(Song *Head) = 0;
   SongType(SongTree *Owner);
};

#endif
