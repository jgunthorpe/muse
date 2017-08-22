/* ########################################################################

   String Table

   ########################################################################
*/
#ifndef STRTABLE_H
#define STRTABLE_H

#define EndL (1 << 0)
#define EndR (1 << 1)
class StringTable
{
   #pragma pack(1)
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
   #pragma pack()

   unsigned long BlockSize;
   IDLSequence<char *> StringTable;
   StringTabItem *FreeStrings;

   char *New(int Size);
   void AddEmpty();
   void UnlinkString(StringTabItem *S);

   public:

   char *New(char *Str);
   void Free(char *Str);
   unsigned long Hash(char *Str);

   inline unsigned long GetHash(char *String)
   {
      return ((StringTabItem *)(String - sizeof(StringTabItem) + 1))->Hash;
   }

   StringTable();
   ~StringTable();
};

int nstrcmp(char *A,char *B);
int MatchWild(const char *String,const char *Wild);

#endif
