/* ########################################################################

   String Table

   ########################################################################
*/
#include <Muse.h>

#include <ctype.h>
#include "StrTable.h"

/* ########################################################################

   Class - StringTable (Manages the song list)
   Member - Constructor (Inits)

   ########################################################################
*/
StringTable::StringTable()
{
   FreeStrings = 0;
   StringTable.construct();
   BlockSize = 64*1024;
}
StringTable::~StringTable()
{
   for (IDLSequence<char *>::iterator I = StringTable.begin(); I != StringTable.end(); I++)
      delete [] *I;
   StringTable.free();
}

/* ########################################################################

   Class - StringTable (Manages the song list)
   Member - AddEmpty (Allocates a new string heap)

   Allocates a new heap for the strings

   ########################################################################
*/
void StringTable::AddEmpty()
{
   if (FreeStrings != 0)
      return;

   StringTable.push_backv(new char[BlockSize]);
   memset(StringTable.back(),0,BlockSize);

   StringTabItem *S = (StringTabItem *)StringTable.back();
   S->Next = FreeStrings;
   S->Length = BlockSize;
   S->End = EndL + EndR;
   FreeStrings = S;
}

/* ########################################################################

   Class - StringTable (Manages the song list)
   Member - New (Allocates a new string by size)

   Do not include the byte for the null, 1 is a 1 char string.
   Hash should be called after the string is written.

   ########################################################################
*/
char *StringTable::New(int Size)
{
   Size++;  //  Need Null
   Size += sizeof(StringTabItem) - 1;
   if (Size > BlockSize)
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
      AddEmpty();
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

   Class - StringTable (Manages the song list)
   Member - UnlinkString (Unlinks a block from the free chain)

   Searches for and removes a string from the free chain.

   ########################################################################
*/
void StringTable::UnlinkString(StringTabItem *S)
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

   Class - StringTable (Manages the song list)
   Member - Free (Frees the block)

   Frees the string (only when references == 1) and globbes the empty block
   with it's neighbors.

   ########################################################################
*/
void StringTable::Free(char *String)
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

   Class - StringTable (Manages the song list)
   Member - New (Gets a string by content)

   Searches the heap for a string with the same content and returns a pointer
   to that string or a newly allocated string containing the given string,
   Hash need not be called.

   ########################################################################
*/
char *StringTable::New(char *String)
{
   if (String == 0)
      return 0;

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
   char *S = New(strlen(String));
   strcpy(S,String);

   ((StringTabItem *)(S - sizeof(StringTabItem) + 1))->Hash = H;
   return S;
}

/* ########################################################################

   Class - StringTable (Manages the song list)
   Member - Hash (Computes a hash value for a string)

   If the string exists in the heap the hash is updated in the heap as well

   ########################################################################
*/
unsigned long StringTable::Hash(char *String)
{
   const char *d = String;
   unsigned long h = 0;

   for (; d != 0 && *d != 0; d++)
       h = (h << 2) + *d;

   return h;
}

int nstrcmp(char *A,char *B)
{
   if (A == 0 && B == 0)   // ==
      return 0;
   if (A == 0)   // 0 < 'A'
      return -1;
   if (B == 0)   // 'A' > 0
      return 1;

   for (;toupper(*A) == toupper(*B) && *A != 0;A++, B++);
   if (toupper(*A) == toupper(*B))      // ==
      return 0;
   if (*A == 0)       // 'Ala' < 'Alababa'
      return -1;
   if (*B == 0)
      return 1;        // 'Alababa' > 'Ala'

   if (toupper(*A) > toupper(*B))
      return 1;
   return -1;
}

/* ########################################################################

   Function - MatchWild (Matches a wildcard spec to a path name)

   Returns 1 if the wild card descibes the given string.

   ########################################################################
*/
int MatchWild(const char *String,const char *Wild)
{
   const char *I = String;
   const char *I2 = Wild;

   if (Wild == "")
      return 1;

   // Match all non * chars
   for (;*I != 0 && *I2 != 0;I++,I2++)
   {
      if (*I2 == '?')
         continue;

      if (*I2 == '*')
      {
         // At the end of the match
         if (I2[1] == 0)
         {
            // Check for Dir Slashes
            for (;*I != 0 && *I != '\\' && *I != '/';I++);
            if (*I == 0)
               return 1;
            return 0;
         }
         /* Recusivly spawn ourself to match the bit after the * to the
            text */
         int R = 0;
         for (;*I != 0 && *I != '\\' && *I != '/' && R == 0;I++)
            if (toupper(*I) == toupper(I2[1]) || I2[1] == '?' || I2[1] == '*')
               R = MatchWild(I,I2+1);
         if (R == 0)
            return 0;
         return 1;
      }

      // Failed
      if (*I == '\\' && *I2 == '/')
         continue;
      if (*I == '/' && *I2 == '\\')
         continue;
      if (toupper(*I) != toupper(*I2))
         return 0;
   }

   if (*I ==0 && (*I2 == 0 || *I2 == '*'))
      return 1;
   return 0;
}

