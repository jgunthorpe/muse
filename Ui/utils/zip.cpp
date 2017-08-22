/* ########################################################################

   Node Type classes

   ########################################################################
*/

#include <Muse.h>

#include "SngList.h"
#include "Zip.h"

#include <ctype.h>
#include <iomanip.h>
#include <io.h>
#include <sys\stat.h>
#include <fcntl.h>

int ZipFileType::ID = 1;
int ZipDirType::ID = 8;
//int ZipModuleType::ID = 9;

/* ########################################################################

   Class - ZipFileType (Parent of a ZipFile)
   Member - IsType (Checks for .zip extension)

   Extension check.

   ########################################################################
*/
int ZipFileType::IsType(const char *FName)
{
   const char *End = FName + strlen(FName);
   for(;*End != '.' && End != FName;End --);

   if (End != FName)
      End++;

   if (stricmp(End,"ZIP") != 0)
      return 0;
   return 1;
}

void ZipFileType::ConfigNode(iterator Node)
{
   Node->Directory = 1;
   Node->Archive = 1;
}

/* ########################################################################

   Class - ZipFileType (Parent of a ZipFile)
   Member - Refresh (Scans the zip)

   Perform Zip Scan

   ########################################################################
*/

#pragma pack(1)
typedef struct
{
   unsigned long Signature;
   unsigned short DiskNum;
   unsigned short DiskWithStart;
   unsigned short NumEntriesOnDisk;
   unsigned short NumEntries;
   unsigned long Size;
   unsigned long Offset;
   unsigned short CommentLength;
} CentralDir;

typedef struct
{
   unsigned long Signature;
	unsigned short VersionBy;
	unsigned short VersionNeeded;
	unsigned short GenFlags;
	unsigned short CompressionMethood;
	unsigned short LastModTime;
	unsigned short LastModDate;
	unsigned long crc32;
	unsigned long CompressedSize;
	unsigned long UnCompressedSize;
	unsigned short FileNameLength;
	unsigned short ExtraLength;
	unsigned short FileComment;
	unsigned short DiskStart;
	unsigned short InternalAttr;
	unsigned long ExternalAttr;
	unsigned long Offset;
} FileRecord;

#pragma pack()
void ZipFileType::SetTempKill(iterator Head)
{
   for (iterator I = Head->Child; I != 0; I++)
   {
      I->TempKill = 1;
      if (I->Child != 0)
         SetTempKill(I);
   }
}
void ZipFileType::KillTempKill(iterator Head)
{
   for (iterator I = Head->Child; I != 0; I++)
   {
      if (I->TempKill == 1)
      {
         iterator OldI = I;
         I++;
         Free(OldI);
         if (I == 0)
            break;
         continue;
      }

      if (I->Child != 0)
         SetTempKill(I);
   }
}

int ZipFileType::Refresh(iterator Head)
{
   char S[300];
   MakePath(Head,S);
   S[strlen(S) - 1] = 0; // Remove ending slash
   int FD = open(S,O_RDONLY|O_BINARY,S_IREAD);
   if (FD == -1)
   {
      Free(Head);
      return 1;
   }

   // Set TempKill flags.
   Head->Refreshed = 1;
   SetTempKill(Head);

   // Seek and read the central dir at it's non-comment location
   CentralDir Dir;
   if (lseek(FD,-1*sizeof(CentralDir),SEEK_END) == -1 ||
       read(FD,(char *)&Dir,sizeof(CentralDir)) != sizeof(CentralDir))
   {
      close(FD);
      Free(Head);
      return 1;
   }

   // Must have a comment, start searching
   if (Dir.Signature != 0x06054b50)
   {
      // Two tries, 2k block and 64k block
      for (char T = 0; T != 1; T++)
      {
         unsigned char *Buf = new unsigned char [0xFFFF+sizeof(CentralDir)+18];
         long Size;
         if (T == 0)
         {
            lseek(FD,-1*(2000+sizeof(CentralDir)+18),SEEK_END);
            Size = read(FD,(char *)Buf,2000+sizeof(CentralDir)+18);
         }
         else
         {
            lseek(FD,-1*(0xFFFF+sizeof(CentralDir)+18),SEEK_END);
            Size = read(FD,(char *)Buf,0xFFFF+sizeof(CentralDir)+18);
         }

         if (Size <= 0)
         {
            delete [] Buf;
            close(FD);
            Free(Head);
            return 1;
         }

         // Search from the back to find the signature
         for (unsigned char *I = Buf + Size; *((unsigned long *)I) != 0x06054b50 && I != Buf; I--);
         if (I == Buf && *((unsigned long *)I) != 0x06054b50)
            continue;

         Dir = *((CentralDir *)I);
         delete [] Buf;
         break;
      }
   }

   if (Dir.Signature != 0x06054b50 || lseek(FD,Dir.Offset,SEEK_SET) == -1)
   {
      close(FD);
      Free(Head);
      return 1;
   }

   // Now at First File Record, Read away
   char LastDir[300];
   iterator LastHead = Head;
   iterator LastNode = 0;
   LastDir[0] = 0;
   for (unsigned int I = 0; I != Dir.NumEntries; I++)
   {
      FileRecord Fil;
      if (read(FD,(char *)&Fil,sizeof(Fil)) != sizeof(Fil) ||
          Fil.Signature != 0x02014b50)
      {
         close(FD);
         Free(Head);
         return 1;
      }

      // Quick Reality check
      if (!(Fil.UnCompressedSize == 0 || Fil.UnCompressedSize > 20*1024*1024 ||
           (Fil.ExternalAttr & 16) != 0))
      {
         // Read the file name
         char *Name = new char[Fil.FileNameLength + 1];
         if (read(FD,Name,Fil.FileNameLength) != Fil.FileNameLength)
         {
            close(FD);
            delete [] Name;
            Free(Head);
            return 1;
         }
         Name[Fil.FileNameLength] = 0;
//         cout << Name << endl;

         // Find the directory Node.
         char *LastSlash = 0;
         for (char *I = Name; *I != 0; I++)
         {
            if (*I == '\\' || *I == '/')
               LastSlash = I;
         }

         // Has a directory attached
         iterator Node;
         if (LastSlash != 0)
         {
            *LastSlash = 0;
            LastSlash++;

            // Check if it was the last hit
            if (stricmp(LastDir,Name) != 0)
            {
               // Nope, Find it and store it
               strcpy(LastDir,Name);
               for (I = Name; *I != 0; I++)
               {
                  if (*I == '\\' || *I == '/')
                     *I = 0;
               }

               LastHead = Head;
               for (I = Name; I < LastSlash; I += strlen(I) + 1)
               {
                  iterator NewHead = Search(LastHead,I);
                  if (NewHead == 0)
                  {
                     NewHead = New(LastHead,I,0,ZipDirType::ID,1);
                     NewHead->Directory = 1;
                     NewHead->Refreshed = 1;
                  }
                  else
                     NewHead->TempKill = 0;
                  LastHead = NewHead;
               }
            }
             Node = Search(LastHead,LastSlash,LastNode);
            if (Node == 0)
               Node = New(LastHead,LastSlash,Fil.UnCompressedSize,0,1);
            else
               Node->TempKill = 0;
         }
         else
         {
            Node = Search(LastHead,LastSlash,LastNode);
            if (Node == 0)
               Node = New(Head,Name,Fil.UnCompressedSize,0,1);
            else
               Node->TempKill = 0;
         }

         if (Node != 0)
         {
            Node->Refreshed = 1;
            LastNode = Node;
         }
         if (lseek(FD,Fil.ExtraLength + Fil.FileComment, SEEK_CUR) == -1)
         {
            close(FD);
            Free(Head);
            return 1;
         }
      }
      else
      {
         if (lseek(FD,Fil.FileNameLength + Fil.ExtraLength + Fil.FileComment, SEEK_CUR) == -1)
         {
            break;
         }
      }
   }

   KillTempKill(Head);
   close(FD);
   return 0;
}
