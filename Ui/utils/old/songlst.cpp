/* ########################################################################

   Tunez Classes

   ########################################################################
*/
#include <Muse.h>

#include "SongLst.h"
#include "SngTypes.h"
#include <ctype.h>
#include <iomanip.h>

/* ########################################################################

   Class - SongTree (Manages the song list)
   Member - Constructor (Init the class)

   ########################################################################
*/
SongTree::SongTree()
{
   Songs.construct();
   StringTable.construct();
   Types.construct();

   SongBlockSize = 500;
   StrBlockSize = 16*1024;
   FreeList = 0;
   FreeStrings = 0;

   Head = GetNewSong();
   Head->TypeId = 0;

   Types[0] = new DisksType(this);
   Types[0]->TypeId = 0;

   Types[1] = new DiskType(this);
   Types[1]->TypeId = 1;

   Types[2] = new DirectoryType(this);
   Types[2]->TypeId = 2;

   Types[Head->TypeId]->Refresh(Head);
}

SongTree::~SongTree()
{
   for (Song **Sng = Songs.begin(); Sng != Songs.end(); Sng++)
      delete [] *Sng;
   Songs.free();

   for (char **S = StringTable.begin(); S != StringTable.end(); S++)
      delete [] *S;
   StringTable.free();

   for (SongType **Ty = Types.begin(); Ty != Types.end(); Ty++)
      delete *Ty;
   Types.free();
}

/* ########################################################################

   Class - SongTree (Manages the song list)
   Member - MakeFreeSongs (Ensures that there are some free songs)

   Allocates a new song block and links it into the free list

   ########################################################################
*/
void SongTree::MakeFreeSongs()
{
   if (FreeList != 0)
      return;
   Songs.push_backv(new Song[SongBlockSize]);

   memset(Songs.back(),0,sizeof(Songs)*SongBlockSize);
   for (Song *S = Songs.back();S != Songs.back() + SongBlockSize - 1; S++)
      S->Next = S + 1;
   FreeList = Songs.back();
}

/* ########################################################################

   Class - SongTree (Manages the song list)
   Member - GetNewSong (Allocates a new song node)

   Pops a song off the free list

   ########################################################################
*/
Song *SongTree::GetNewSong()
{
   if (FreeList == 0)
      MakeFreeSongs();
   Song *S = FreeList;
   FreeList = FreeList->Next;
   S->Next = 0;
   return S;
}

/* ########################################################################

   Class - SongTree (Manages the song list)
   Member - FreeSong (Erases a song node)

   Unlinks and pushes the node onto the free list, then recursively unlinks
   all children.

   ########################################################################
*/
void SongTree::FreeSong(Song *Node)
{
   Song *Child = Node->Child;
   cout << "Free " << Node->Name << endl;

   if (Node->Parent != 0)
   {
      // Unlink from the parent
      if (Node->Parent->Child == Node)
         Node->Parent->Child = Node->Next;
      else
      {
         // Unlink from the last
         Song *T = FindPrev(Node);
         if (T != 0)
            T->Next = Node->Next;
      }
   }

   // Zero and add to free list
   KillNode(Node);
   Node->Next = FreeList;
   FreeList = Node;

   if (Child == 0)
      return;

   // Kill all children
   for (Song *S = Child; S != 0; S = S->Next)
   {
      if (S->Child != 0)
      {
         S->Parent = 0;
         FreeSong(S);
      }
      Song *Temp = S->Next;
      KillNode(S);
      S->Next = Temp;
      Node = S;
   }

   Node->Next = FreeList;
   FreeList = Child;
}

/* ########################################################################

   Class - SongTree (Manages the song list)
   Member - KillNode (Deallocates strings held in the node)

   Frees allocated strings and 0's the node.

   ########################################################################
*/
void SongTree::KillNode(Song *Node)
{
   FreeString(Node->Name);
   memset(Node,0,sizeof(*Node));
}

/* ########################################################################

   Class - SongTree (Manages the song list)
   Member - Insert (Inserts a node below a give parent)

   Should add in a sorted fashion, but doesn't.

   ########################################################################
*/
void SongTree::Insert(Song *Parent,Song *Node)
{
   if (Node->Parent == Parent)
      return;

   Node->Next = Parent->Child;
   Node->Parent = Parent;
   Parent->Child = Node;
}

/* ########################################################################

   Class - SongTree (Manages the song list)
   Member - FindPrev (Finds the previous node to the given node)

   Searches from the start of the list to the current node to get the
   previose node, is slow.

   ########################################################################
*/
Song *SongTree::FindPrev(Song *Node)
{
   if (Node->Parent == 0 || Node->Parent->Child == Node || Node->Parent->Child == 0)
      return 0;
   for (Song *S = Node->Parent->Child; S->Next != Node && S->Next != 0; S = S->Next);
   if (S->Next != Node)
      return 0;
   return S;
}

/* ########################################################################

   Class - SongTree (Manages the song list)
   Member - MakePath (Internal recursive helper)

   Generates the path name for a node, with trailing '\' if it's a directory.

   ########################################################################
*/
void SongTree::MakePathI(Song *Base,char *&Name)
{
   if (Base->Parent != 0)
      MakePathI(Base->Parent,Name);

   for (char *C = Base->Name; C != 0 && *C != 0; C++, Name++)
      *Name = *C;

   if (Types[Base->TypeId]->Append != 0)
   {
      *Name = Types[Base->TypeId]->Append;
      Name++;
   }
}

void SongTree::MakePath(Song *Base,char *Name)
{
   Name[0] = 0;
   char *C = Name;
   MakePathI(Base,C);
   *C = 0;
}

/* ########################################################################

   Class - SongTree (Manages the song list)
   Member - MakeFreeString (Allocates a new string heap)

   Allocates a new heap for the strings

   ########################################################################
*/
void SongTree::MakeFreeString()
{
   StringTable.push_backv(new char[StrBlockSize]);
   memset(StringTable.back(),0,StrBlockSize);

   StringTabItem *S = (StringTabItem *)StringTable.back();
   S->Next = FreeStrings;
   S->Length = StrBlockSize;
   S->End = EndL + EndR;
   FreeStrings = S;
}

/* ########################################################################

   Class - SongTree (Manages the song list)
   Member - GetString (Allocates a new string by size)

   Do not include the byte for the null, 1 is a 1 char string.
   Hash should be called after the string is written.

   ########################################################################
*/
char *SongTree::GetString(unsigned long Size)
{
   Size++;  //  Need Null
   Size += sizeof(StringTabItem) - 1;
   if (Size > StrBlockSize)
      return 0;

   StringTabItem *I = 0;
   StringTabItem *P = 0;
   while (1)
   {
      P = 0;
      // Search free string list for one that is larger
      for (I = FreeStrings; I != 0 && I->Length < Size; I = I->Next)
         P = I;
      if (I != 0)
         break;
      MakeFreeString();
   }

   // Make a hole
   StringTabItem *Hole = 0;
   if (I->Length - Size > sizeof(StringTabItem))
   {
      Hole = (StringTabItem *)(((char *)I) + Size);
      Hole->Next = I->Next;

      Hole->PrevLen = Size;
      Hole->Length = I->Length - Size;
      Hole->End = I->End  & EndR;
      I->End = I->End & EndL;
      I->Length = Size;
      if (Hole->End != EndR)
         ((StringTabItem *)(((char *)Hole) + Hole->Length))->PrevLen = Hole->Length;
   }
   else
      Hole = I->Next;

   // Update the prev pointer
   if (P == 0)
      FreeStrings = Hole;
   else
      P->Next = Hole;

   I->Hash = 0;
   I->References = 1;
   return I->Str;
}

/* ########################################################################

   Class - SongTree (Manages the song list)
   Member - UnlinkString (Unlinks a block from the free chain)

   Searches for and removes a string from the free chain.

   ########################################################################
*/
void SongTree::UnlinkString(StringTabItem *S)
{
   // Find and unlink from free chain
   StringTabItem *P = 0;
   for (StringTabItem *I = FreeStrings; I != 0 && I != S; I = I->Next)
      P = I;

   // Shouldn't happen
   if (I == 0)
      printf("Oh Joy\n");

   if (P == 0)
      FreeStrings = I->Next;
   else
      P->Next = I->Next;
}

/* ########################################################################

   Class - SongTree (Manages the song list)
   Member - FreeString (Frees the block)

   Frees the string (only when references == 1) and globbes the empty block
   with it's neighbors.

   ########################################################################
*/
void SongTree::FreeString(char *String)
{
   if (String == 0)
      return;

   StringTabItem *S = ((StringTabItem *)(String - sizeof(StringTabItem) + 1));
   S->References--;
   if (S->References != 0)
      return;

   // Zero the string
   memset(String,0,S->Length - sizeof(StringTabItem) + 1);
   S->References = 0;

   // Combine with the next block
   if ((S->End & EndR) == 0)
   {
      StringTabItem *Next = ((StringTabItem *)(((char *)S) + S->Length));

      // It's free
      if (Next->References == 0)
      {
         UnlinkString(Next);
         S->Length += Next->Length;
         S->End |= Next->End;
         memset(Next,0,sizeof(*Next) - 1);
      }
   }

   // Combine with the last block
   if ((S->End & EndL) == 0)
   {
      StringTabItem *Last = ((StringTabItem *)(((char *)S) - S->PrevLen));

      // It's free
      if (Last->References == 0)
      {
         UnlinkString(Last);
         Last->Length += S->Length;
         Last->End |= S->End;
         memset(S,0,sizeof(*S) - 1);
         S = Last;
      }
   }

   // Update links
   if ((S->End & EndR) == 0)
      ((StringTabItem *)(((char *)S) + S->Length))->PrevLen = S->Length;

   S->Next = FreeStrings;
   FreeStrings = S;
}

/* ########################################################################

   Class - SongTree (Manages the song list)
   Member - GetString (Gets a string by content)

   Searches the heap for a string with the same content and returns a pointer
   to that string or a newly allocated string containing the given string,
   Hash need not be called.

   ########################################################################
*/
char *SongTree::GetString(char *String)
{
   unsigned long H = Hash(String);

   for (int I = 0; I != StringTable.size(); I++)
   {
      StringTabItem *Next = (StringTabItem *)(StringTable[I]);
      while ((Next->End & EndR) == 0)
      {
         if (Next->References != 0 && Next->Hash == H  && strcmp(Next->Str,String) == 0)
         {
            Next->References++;
            return Next->Str;
         }
         Next = ((StringTabItem *)(((char *)Next) + Next->Length));
      }
   }
   char *S = GetString(strlen(String));
   strcpy(S,String);
   Hash(S);
   return S;
}

/* ########################################################################

   Class - SongTree (Manages the song list)
   Member - Hash (Computes a hash value for a string)

   If the string exists in the heap the hash is updated in the heap as well

   ########################################################################
*/
unsigned long SongTree::Hash(char *String)
{
   const char *d = String;
   unsigned long h = 0;

   for (; d != 0 && *d != 0; d++)
       h = (h << 2) + *d;

   // Store new hash
   for (int I = 0; I != StringTable.size(); I++)
      if (StringTable[I] < String && (StringTable[I] + StrBlockSize) > String)
      {
         StringTabItem *S = ((StringTabItem *)(String - sizeof(StringTabItem) + 1));

         // Neat
         if (S->References == 0)
         {
            printf("Dead String\n");
            return h;
         }

         S->Hash = h;
         return h;
      }
   return h;
}

SongType::SongType(SongTree *Owner) : Owner(Owner), Append(0)
{
};

void Print(SongTree &Tree,Song *Node,int L = 0)
{
   if (Node == 0)
      return;

   char C[300];
   Tree.MakePath(Node,C);
   cout << C << endl;
   Print(Tree,Node->Child,L + 1);
   Print(Tree,Node->Next,L);
}
#include <conio.h>

void main()
{
   SongTree Tree;
//   Print(Tree,Tree.Head);
   while (1)
   {
      getch();
      Tree.Types[Tree.Head->TypeId]->Refresh(Tree.Head);
      cout << endl;
   }
   cout << Tree.StringTable.size()*16*1024 << endl;
   cout << Tree.Songs.size()*500*sizeof(Song) << endl;
}
