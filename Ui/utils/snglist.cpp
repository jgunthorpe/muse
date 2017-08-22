/* ########################################################################

   Song List

   ########################################################################
*/
#include <Muse.h>

#include "SngList.h"
#include "SngTypes.h"
#include "Zip.h"

#include <ctype.h>
#include <iostream.h>

/* ########################################################################

   Class - SongList (Manages the song list)
   Member - Constructor (Inits)

   ########################################################################
*/
SongList::SongList()
{
   Types.construct();
   Types[DisksType::ID] = new DisksType(this);
   Types[DiskType::ID] = new DiskType(this);
   Types[DirectoryType::ID] = new DirectoryType(this);

   Types[ZipFileType::ID] = new ZipFileType(this);
   Types[ZipDirType::ID] = new ZipDirType(this);
//   Types[ZipModuleType::ID] = new ZipModuleType(this);

   Head = New();
   Head->TypeId = DisksType::ID;
   Disks.construct();
}

SongList::~SongList()
{
   for (NodeType **I = Types.begin(); I != Types.end(); I++)
      delete *I;
   Types.free();
   Disks.free();
}

NodeType::NodeType(SongList *Owner) : Owner(Owner), Append(0)
{
}

void SongList::iterator::operator --(int)
{
   if (Prev != 0 && Prev->Next == Cur)
   {
      Cur = Prev;
      Prev = 0;
      return;
   }

   if (Cur == 0 || Cur->Parent == 0 || Cur->Parent->Child == Cur)
   {
      Cur = 0;
      return;
   }

   for (iterator I = Cur->Parent->Child; I != 0 && I.TrueNode()->Next != Cur; I++);
   *this = I;
}

/* ########################################################################

   Class - SongList (Manages the song list)
   Member - Free (Erases a song node)

   Unlinks and pushes the node onto the free list, then recursively unlinks
   all children.

   ########################################################################
*/
void SongList::Free(SongList::iterator Node)
{
   cout << "Free " << Node->Name << endl;
   if (Node.IsLink() != 0)
   {
      // Unlink it from the link chain
      Node.TrueNode()->IsLink = 0;
      Node->PrevLink->NextLink = Node->NextLink;
      if (Node->NextLink != 0)
         Node->NextLink->PrevLink = Node->PrevLink;
      Node->Child = 0;
   }

   iterator Child = Node->Child;

   if (Node->Parent != 0)
   {
      // Unlink from the parent
      if (Node->Parent->Child == Node)
         Node->Parent->Child = Node->Next;
      else
      {
         // Unlink from the last
         iterator T = Node;
         T--;
         if (T != 0)
            T.TrueNode()->Next = Node->Next;
      }
   }

   // Kill all links
   for (iterator S = Node->NextLink; S != 0; S = S.TrueNode()->NextLink)
      Free(S);

   // Zero and add to free list
   KillNode(Node);
   Nodes.Free(Node);

   if (Child == 0)
      return;

   // Kill all children
   for (S = Child; S != 0;)
   {
      if (S.IsLink() != 0)
      {
         // Unlink it from the link chain
         S.TrueNode()->IsLink = 0;
         S->PrevLink->NextLink = S->NextLink;
         if (S->NextLink != 0)
            S->NextLink->PrevLink = S->PrevLink;
         S->Child = 0;
      }

      if (S->Child != 0)
      {
         iterator Temp = S;
         S++;
         Temp->Parent = 0;
         Free(Temp);
         continue;
      }

      iterator Temp = S;
      S++;
      KillNode(Temp);
      Nodes.Free(Temp);
   }
}

/* ########################################################################

   Class - SongList (Manages the song list)
   Member - KillNode (Deallocates strings held in the node)

   Frees allocated strings and 0's the node.

   ########################################################################
*/
void SongList::KillNode(iterator Node)
{
   Strings.Free(Node->Name);
   if (Node->Info != 0)
   {
      Strings.Free(Node->Info->Title);
      Strings.Free(Node->Info->TypeName);
      Strings.Free(Node->Info->ClassName);
      Strings.Free(Node->Info->Author);
      Strings.Free(Node->Info->AuthComment);
      Strings.Free(Node->Info->ModComment);
      Strings.Free(Node->Info->Style);
      Songs.Free(Node->Info);
   }
}

/* ########################################################################

   Class - SongList (Manages the song list)
   Member - Compare (Compares nodes)

   0 is equal, 1 is > -1 is <
   ########################################################################
*/
int SongList::Compare(Node *A,Node *B)
{
   int I;

   // Compare Names
   if (A->Name != B->Name)
   {
      I = nstrcmp(A->Name,B->Name);
      if (I != 0) return I;
   }

   // Compare Sizes
   if (A->Size > B->Size)
      return 1;

   if (A->Size < B->Size)
      return -1;
   return 0;
}

/* ########################################################################

   Class - SongList (Manages the song list)
   Member - New (Creates a new node)

   Gets a node from the free nodes.

   ########################################################################
*/
SongList::iterator SongList::New(Node *Parent,char *Name,unsigned long Size,int TypeId,char Archived, iterator Insert)
{
   // Means it's an untyped file
   if (TypeId == 0)
   {
      // Search for owning type
      for (NodeType **I = Types.begin(); I != Types.end(); I++)
         if (*I != 0 && (*I)->IsType(Name) == 1)
            TypeId = I - Types.begin();
   }

   iterator T = Nodes.New();
   T->Parent = Parent;
   T->Name = Strings.New(Name);
   T->Size = Size;
   T->TypeId = TypeId;
   T->Archived = Archived;
   if (Parent == 0)
      return T;

   // Insert it
   if (Insert == 0)
   {
      T->Next = Parent->Child;
      Parent->Child = T;
   }
   else
   {
      T->Next = Insert.TrueNode()->Next;
      Insert.TrueNode()->Next = T;
   }

   ReLocate(T);
   if (Types[T->TypeId] != 0)
      Types[T->TypeId]->ConfigNode(T);
   return T;
}

/* ########################################################################

   Class - SongList (Manages the song list)
   Member - ReLocate (Sorts a node)

   Resorts a node in a subtree

   ########################################################################
*/
void SongList::ReLocate(iterator &T)
{
   Node *TrueT = T.TrueNode();          // Real T, deal with links
   if (TrueT->Parent == 0)
      return;

   iterator I = T;
   iterator I2 = T;
   iterator S = TrueT->Parent->Child;
   I--;
   I2++;

   // Starting point is T
   if (I == 0 || Compare(I,T) <= 0)
   {
      // It's properly sorted
      if (I2 == 0 || Compare(I2,T) >= 0)
         return;

      // Unlink from parent
      if (I == 0)
      {
         S++;
         TrueT->Parent->Child = S.TrueNode();
      }
      else
      {
         S = I.TrueNode();
         I.TrueNode()->Next = TrueT->Next;
      }
   }

   for (;S != 0 && Compare(S,T) < 0; S++);

   S--;
   if (S == 0)
   {
      TrueT->Next = TrueT->Parent->Child;
      TrueT->Parent->Child = TrueT;
      T.Prev = 0;
   }
   else
   {
      TrueT->Next = S.TrueNode()->Next;
      S.TrueNode()->Next = TrueT;
      T.Prev = S.TrueNode();
   }
}

/* ########################################################################

   Class - SongList (Manages the song list)
   Member - Link (Links a node to another node)

   Resorts a node in a subtree

   ########################################################################
*/
SongList::iterator SongList::Link(Node *Parent,Node *Target,iterator Insert)
{
   // Link to the true owner
   if (Target->IsLink == 1)
      Target = Target->Link;

   Node *T = Nodes.New();
   T->Parent = Parent;
   T->IsLink = 1;
   T->Link = Target;

   // Do link linking
   T->NextLink = Target->NextLink;
   T->PrevLink = Target;
   if (Target->NextLink != 0)
      Target->NextLink->PrevLink = T;
   Target->NextLink = T;

   // Insert it
   if (Insert == 0)
   {
      T->Next = Parent->Child;
      Parent->Child = T;
   }
   else
   {
      T->Next = Insert.TrueNode()->Next;
      Insert.TrueNode()->Next = T;
   }

   iterator I = T;
   ReLocate(I);
   return I;
}

/* ########################################################################

   Class - SongList (Manages the song list)
   Member - MakePath (Internal recursive helper)

   Generates the path name for a node, with trailing '\' if it's a directory.

   ########################################################################
*/
void SongList::MakePathI(Node *Base,char *&Name)
{
   if (Base->Parent != 0)
      MakePathI(Base->Parent,Name);

   for (char *C = Base->Name; C != 0 && *C != 0; C++, Name++)
      *Name = *C;

   if (Base->TypeId != 0 && Types[Base->TypeId]->Append != 0)
   {
      *Name = Types[Base->TypeId]->Append;
      Name++;
   }
}

void SongList::MakePath(Node *Base,char *Name)
{
   if (Base == 0)
   {
      Name[0] = 0;
      return;
   }

   Name[0] = 0;
   char *C = Name;
   MakePathI(Base,C);
   *C = 0;
}

/* ########################################################################

   Class - SongList (Manages the song list)
   Member - Search (Locates a child node by name)

   Searches, using Start as a division point.

   ########################################################################
*/
SongList::iterator SongList::Search(Node *Parent,char *Name,SongList::iterator Start,int Refresh)
{
   if (Parent == 0)
      return 0;

   if (Parent->Refreshed == 0)
      if (this->Refresh(Parent) != 0)
         return 0;

   iterator S = Start;
   S--;

   // I is above Name, so reset
   if (S == 0 || nstrcmp(S->Name,Name) > 0)
      S = Parent->Child;

   int Last = 0;
   for (;S != 0 && (Last = nstrcmp(S->Name,Name)) < 0; S++);

   if (Last == 0 && S != 0)
      return S;

   if (Refresh == 1)
   {
      if (this->Refresh(Parent) != 0)
         return 0;
      return Search(Parent,Name,Start);
   }
   return 0;
}

/* ########################################################################

   Class - SongList (Manages the song list)
   Member - GetNode (Reverse of GetPath)

   Locates the node that matches the given path name, if components do
   not exist and Exists = 1 then Refresh is called to generate them.

   Start is the current directory for relative offsets.

   ########################################################################
*/
SongList::iterator SongList::GetNode(char *Name,iterator Start)
{
   char S[300];
   strcpy(S,Name);
   char *E = S + strlen(S);

   // Tokanize the string
   for (char *I = S; I != E; I++)
   {
      if (*I == '\\' || *I == '/')
      {
         for (;I != E && (*I == '\\' || *I == '/'); I++);
         I[-1] = 0;
      }
   }
   I = S;

   // Check for a drive spec
   if (Name[1] == ':')
   {
      S[1] = 0;
      // Absolute path
      if (S[2] == 0)
      {
         Start = Search(Head,S,0,1);
         Start = Start->Child;       // Goto Root directory
         I += 3;
      }
      else
      {
         if (toupper(Name[0]) >= 'A' && toupper(Name[0]) <= 'Z')
         {
            I += 2;
            Start = Disks[toupper(Name[0]) - 'A'];
            if (Start == 0)
            {
               Start = Search(Head,S,0,1);
               Start = Start->Child;       // Goto Root directory
               Disks[toupper(Name[0]) - 'A'] = Start;
            }
         }
      }
   }

   // No disk is given
   if (Start == 0)
      return 0;

   // Analyze it
   for (; I < E;I += strlen(I) + 1)
   {
      // Empty \\?
      if (*I == 0)
         return 0;

      // Check for dots
      for (char *C = I; *C != 0 && *C == '.'; C++);
      if (*C == 0)
      {
         int Back = strlen(I) - 1;
         for (; Start != 0 && I != 0 && Back != 0; Start = Start->Parent, Back--);
         if (Start == 0 || Start->Parent == 0 || Start->Parent == Head)
            return 0;
         continue;
      }

      // Go down
      Start = Search(Start,I,0,1);
      if (Start == 0)
         return 0;
   }

   return Start;
}

/* ########################################################################

   Class - SongList (Manages the song list)
   Member - WildGetNode (Reverse of GetPath)

   Uses a wildcard search to determine all matching nodes, they are linked
   to a sub-node.

   Start is the current directory for relative offsets.

   ########################################################################
*/
SongList::iterator SongList::WildGetNode(char *Name,iterator Parent,iterator Start)
{
   char S[300];
   strcpy(S,Name);
   char *E = S + strlen(S);
   char Newed = 0;
   if (Parent == 0)
   {
      Parent = New();
      Newed = 1;
   }

   // Tokanize the string
   for (char *I = S; I < E; I++)
   {
      if (*I == '\\' || *I == '/')
      {
         for (;I != E && (*I == '\\' || *I == '/'); I++);
         I[-1] = 0;
      }
   }

   I = S;

   // Check for a drive spec
   if (Name[1] == ':')
   {
      S[1] = 0;
      // Absolute path
      if (S[2] == 0)
      {
         Start = Search(Head,S,0,1);
         Start = Start->Child;       // Goto Root directory
         I += 3;
      }
      else
      {
         if (toupper(Name[0]) >= 'A' && toupper(Name[0]) <= 'Z')
         {
            I += 2;
            Start = Disks[toupper(Name[0]) - 'A'];
            if (Start == 0)
            {
               Start = Search(Head,S,0,1);
               Start = Start->Child;       // Goto Root directory
               Disks[toupper(Name[0]) - 'A'] = Start;
            }
         }
      }
   }

   // No disk is given
   if (Start == 0)
      return 0;

   WildHelper(I,E,Parent,Start);
   if (Parent->Child == 0 && Newed == 1)
   {
      Free(Parent);
      return 0;
   }
   return Parent;
}

void SongList::WildHelper(char *I,char *E,iterator Parent,iterator Start)
{
   if (Start == 0)
      return;

   // Analyze it
   for (; I < E;I += strlen(I) + 1)
   {
      // Empty \\?
      if (*I == 0)
         return;

      // Check for dots
      for (char *C = I; *C != 0 && *C == '.'; C++);
      if (*C == 0)
      {
         int Back = strlen(I) - 1;
         for (; Start != 0 && I != 0 && Back != 0; Start = Start->Parent, Back--);
         if (Start == 0 || Start->Parent == 0 || Start->Parent == Head)
            return;
         continue;
      }

      // Check for wilds
      int Wilds = 0;
      for (char *I2 = I; *I2 != 0 && Wilds == 0; I2++)
         if (*I2 == '*' || *I2 == '?')
            Wilds = 1;

      // Fast Non Wild matcher
      if (Wilds == 0)
      {
         Start = Search(Start,I,0,1);
         if (Start == 0)
            return;
         continue;
      }

      // Search with wildcards
      if (Start->Refreshed != 0)
         if (Refresh(Start) != 0)
            return;

      // Last directory element.
      int Last = 0;
      if (I + strlen(I) + 1 >= E)
         Last = 1;

      for (iterator Cur = Start->Child; Cur != 0;)
      {
         // See if this item matches
         if (MatchWild(Cur->Name,I) == 1)
         {
            // It's a directory, descend
            if (Cur->Directory == 1)
            {
               iterator OCur = Cur;
               Cur++;       // Incase Refresh fails, Cur will still be good

               // Don't bother looking at sub dirs
               if (Last == 1)
                  continue;

               WildHelper(I + strlen(I) + 1,E,Parent,OCur);
               continue;
            }
            else
               if (Last == 1)
                  Link(Parent,Cur);
         }
         Cur++;
      }
      break;
   }
}

char C[300];
void Print(SongList::iterator Node,SongList &List,int L = 0)
{
   if (Node == 0)
      return;

   for (;Node != 0; Node++)
   {
      for (int I = 0; I != L; I++)
         cout << ' ';

      List.MakePath(Node,C);
      cout << C;

      if (Node->Info != 0)
         cout << " '" << Node->Info->Title << '\'';
      cout << endl;

      if (Node.IsLink() == 0 && Node->Child != 0)
         Print(Node->Child,List,L + 1);
   }
}

void main(char argc,char *argv[])
{
   SongList A;
   A.Refresh(A.Head);
   A.Refresh(A.GetNode(argv[1]));
   cout << "---" << endl;
   Print(A.Head,A);
}
