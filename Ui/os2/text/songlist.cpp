#pragma off(behaved)
/* ########################################################################

   SongList - Handles Creating the song list

   ########################################################################
*/
#ifndef CPPCOMPILE
#ifndef DTS
#include <IDLSequence.xh>
#pragma pack(4)
#include <HandlersList.xh>
#pragma pack()
#else
#include <IDLSequence.hh>
#include <HandLst.hh>
#endif
#else
#include <IDLSeq.hc>
#include <HandLst.hc>
#endif

#include "SongList.h"

#include <iostream.h>
#include <iomanip.h>
#include <fstream.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <io.h>
#include <sys\stat.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>

#define INCL_DOS
#define INCL_ERRORS
#include <os2.h>

#define PIPE_SIZE 65535

// Ripped strstr from the borland RTL and made it into stristr
char *stristr(const char *str1, const char *str2)
{
   int len1 = strlen(str1);
   int len2 = strlen(str2);
   int i,j,k;

   if (!len2)
      return (char *)str1;    /* return str1 if str2 empty */
   if (!len1)
      return 0;               /* return NULL if str1 empty */
   i = 0;
   for(;;)
   {
      while(i < len1 && toupper(str1[i]) != toupper(str2[0]))
         ++i;
      if (i == len1)
         return 0;
      j = 0;
      k = i;
      while (i < len1 && j < len2 && toupper(str1[i]) == toupper(str2[j]))
      {
         ++i;
         ++j;
      }
      if (j == len2)
         return (char *)str1+k;
      if (i == len1)
         return 0;
   }
}

/* ########################################################################

   Class - SongList (Handle Creating the song list)
   Member - Constructor (Init the class)

   Just zero data members

   ########################################################################
*/
SongList::SongList()
{
   List.construct();
   List.reserve(500);
   Handlers = 0;
}

SongList::~SongList()
{
   List.free();
};

/* ########################################################################

   Class - SongList (Handle Creating the song list)
   Member - Add (Adds to the list, can handle zips, dirs and wildcards)

   Will add the specified search path to the list.

   ########################################################################
*/
void SongList::Add(const char *Path)
{
   // Check to see if it's a zip path
   const char *ZI = stristr(Path,".ZIP");
   if (ZI != 0 && (ZI[4] == '\\' || ZI[4] == '/'))
   {
      char S[300];
      memset(S,0,sizeof(S));
      strncpy(S,Path,ZI - Path + 4);
      ProcessZip(S,ZI + 5);
      return;
   }

   // Check to see if it's an rar path
   ZI = stristr(Path,".RAR");
   if (ZI != 0 && (ZI[4] == '\\' || ZI[4] == '/'))
   {
      char S[300];
      memset(S,0,sizeof(S));
      strncpy(S,Path,ZI - Path + 4);
      ProcessRAR(S,ZI + 5);
      return;
   }

   IDLSequence<DirItem> DirList;
   DirList.construct();
   DirList.reserve(400);

   WildCards(Path,DirList);

   // No Items, add a * to the end and try again
   if (DirList.size() == 0)
   {
      char S[300];
      strcpy(S,Path);
      strcat(S,"*");
      DirList._length = 0;
      WildCards(S,DirList);
   }

   // Only 1 item, see if it's a directory
   if (DirList.size() == 1)
   {
      // Is a directory
      if ((DirList[0].Flags & 1) != 0)
      {
         char S[300];
         strcpy(S,DirList[0].Name);
         strcat(S,"\\*");
         DirList._length = 0;
         DirList[0].Free();
         DirList.erasefrom(DirList.begin());
         WildCards(S,DirList);
      }
   }

   for (IDLSequence<DirItem>::iterator I = DirList.begin(); I != DirList.end(); I++)
   {
      // Not a Directory
      if ((I->Flags & 1) == 0)
      {
         const char *End = I->Name + strlen(I->Name);
         for(;*End != '.' && End != I->Name;End --);

         if (End != I->Name)
            End++;

         if (stricmp(End,"ZIP") != 0 && stricmp(End,"RAR") != 0)
            AddItem(*I);
         else
         {
            if ( !stricmp(End,"ZIP") ) ProcessZip(I->Name,"");
            if ( !stricmp(End,"RAR") ) ProcessRAR(I->Name,"");
            I->Free();
         }
      }
      else
         I->Free();

      if (List.capacity() - List.size() <= 2)
         List.reserve(List.size() + 101);
   }
   DirList.free();
}

/* ########################################################################

   Class - SongList (Handle Creating the song list)
   Member - Dump (Displays each song in the list, testing)

   Test member

   ########################################################################
*/
void SongList::Dump()
{
   for (DirItem *I = List.begin(); I != List.end(); I++)
   {
      if ((I->Flags & 2) != 0)
         cout << "Z ";
      else
         if ((I->Flags & 4) != 0)
            cout << "R ";
         else
            cout << "  ";
      cout << I->Name << ' ' << I->Size << endl;
   }
}

/* ########################################################################

   Class - SongList (Handle Creating the song list)
   Member - Sort (Sorts the list)

   Simple quicksort

   ########################################################################
*/
static int Comparer(const void *A,const void *B)
{
   char *As = ((DirItem *)A)->Name;
   char *Bs = ((DirItem *)B)->Name;

   // Only check the file name
   char *I;
   for (I = As + strlen(As); *I != '\\' && *I != '/' && *I != ':' && I != As; I--);
   if (I != As)
      I++;
   As = I;
   for (I = Bs + strlen(Bs); *I != '\\' && *I != '/' && *I != ':' && I != Bs; I--);
   if (I != Bs)
      I++;
   Bs = I;

   // Compare them if they match compare the whole string
   int Res = stricmp(As,Bs);
   if (Res == 0)
      Res = stricmp(((DirItem *)A)->Name,((DirItem *)B)->Name);
   return Res;
}
void SongList::Sort()
{
   qsort(List.begin(),List.size(),sizeof(DirItem),&Comparer);
}

/* ########################################################################

   Class - SongList (Handle Creating the song list)
   Member - Randomize (Ramonizes the list)

   Simple ramomize routine

   ########################################################################
*/
void SongList::Randomize()
{
   ULONG clk;
   if (DosQuerySysInfo(QSV_MS_COUNT,QSV_MS_COUNT,&clk,sizeof(clk)) != 0)
      clk = 1;

   srand(clk);
   int Size = List.size() - 1;
   if (Size == 0)
      return;
   for (int I2 = 0; I2 != 20; I2++)
      for (DirItem *I = List.begin(); I != List.end(); I++)
      {
         DirItem Temp;
         Temp.copy(*I);
         DirItem *Targ = List.begin() + (rand() % Size);
         I->copy(*Targ);
         Targ->copy(Temp);
      }
}

/* ########################################################################

   Class - SongList (Handle Creating the song list)
   Member - AddListFile (Calls add with the contents of a list file)

   Will use each line in the list file as though it were a search path

   ########################################################################
*/
int SongList::AddListFile(const char *Path)
{
   ifstream inf(Path,ios::in);

   // Unable to open the file.
   if (!inf)
      return 1;

   char string[200];
   inf.width(sizeof(string));

   // Loop till the end of the file.
   while (inf)
   {
      inf >> string;

      // Empty string
      if (string[0] != 0)
         Add(string);
   }

   return 0;
}

/* ########################################################################

   Class - SongList (Handle Creating the song list)
   Member - ProcessZip (Builds a list of zip filenames and chooses the proper
                        subset)

   Will get a list of all the files in the zip and match them to the
   wild card.

   ########################################################################
*/
void SongList::ProcessZip(const char *Archive, const char *Wild)
{

   IDLSequence<DirItem> DirList;
   DirList.construct();
   DirList.reserve(100);

   WildCards(Archive,DirList);
   for (IDLSequence<DirItem>::iterator I = DirList.begin(); I != DirList.end(); I++)
   {
      if (ProcessZipInt(I->Name,Wild) != 0)
         ProcessZipWithZip(I->Name,Wild);
      I->Free();
   }
}

void SongList::ProcessRAR(const char *Archive, const char *Wild)
{

   IDLSequence<DirItem> DirList;
   DirList.construct();
   DirList.reserve(100);

   WildCards(Archive,DirList);
   for (IDLSequence<DirItem>::iterator I = DirList.begin(); I != DirList.end(); I++)
   {
      if (ProcessRARInt(I->Name,Wild) != 0)
         ProcessRARWithRAR(I->Name,Wild);
      I->Free();
   }
}

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

int SongList::ProcessZipInt(const char *Archive, const char *Wild)
{
   int FD = open(Archive,O_RDONLY|O_BINARY,S_IREAD);
   if (FD == -1)
   {
      return 1;
   }

   // Seek and read the central dir at it's non-comment location
   CentralDir Dir;
   if (lseek(FD,-1*sizeof(CentralDir),SEEK_END) == -1 ||
       read(FD,(char *)&Dir,sizeof(CentralDir)) != sizeof(CentralDir))
   {
      close(FD);
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
      return 1;
   }

   // Now at First File Record, Read away
   int ANLen = strlen(Archive);
   for (unsigned int I = 0; I != Dir.NumEntries; I++)
   {
      FileRecord Fil;
      if (read(FD,(char *)&Fil,sizeof(Fil)) != sizeof(Fil) ||
          Fil.Signature != 0x02014b50)
      {
         close(FD);
         return 1;
      }

      // Quick Reality check
      if (!(Fil.UnCompressedSize == 0 || Fil.UnCompressedSize > 20*1024*1024 ||
           (Fil.ExternalAttr & 16) != 0))
      {
         // Read the file name
         char *Name = new char[ANLen + Fil.FileNameLength + 2];
         if (read(FD,Name + ANLen + 1,Fil.FileNameLength) != Fil.FileNameLength)
         {
            close(FD);
            delete [] Name;
            return 1;
         }
         Name[ANLen + Fil.FileNameLength + 1] = 0;

         // Check for wildcard matchup
         if (MatchWild(&Name[ANLen+1],Wild) == 1)
         {
            strcpy(Name,Archive);
            Name[ANLen] = '\\';

            // Add to list
            DirItem Item;
            Item.Name = Name;
            Item.Flags = 2;
            Item.Size = Fil.UnCompressedSize;
            AddItem(Item);
         }
         else
         {
            delete [] Name;
         }

         if (lseek(FD,Fil.ExtraLength + Fil.FileComment, SEEK_CUR) == -1)
         {
            close(FD);
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

   close(FD);
   return 0;
}

int SongList::ProcessRARInt(const char *Archive, const char *Wild)
{
   int FD = open(Archive,O_RDONLY|O_BINARY,S_IREAD);
   if (FD == -1)
   {
      return 1;
   }

   long CurPos;
   long FileSize = lseek ( FD, 0, SEEK_END );
   lseek ( FD, 0, SEEK_SET );

   char Buffer[7];
   char Sig[7] = { 0x52, 0x61, 0x72, 0x21, 0x1a, 0x07, 0x00 };
   #pragma pack(1)
   struct {
   unsigned long PackSize;
   unsigned long FileSize;
   unsigned char HostOS;
   unsigned long FileCRC;
   unsigned long FileTime;
   unsigned char Version;
   unsigned char Method;
   unsigned short NameSize;
   unsigned long Attributes;
   } FileHeader;
   #pragma pack()

   if (read (FD, Buffer, 7) != 1*7)
   {
      close (FD);
      return 1;
   }

   if ( memcmp ( Buffer, Sig, 7 ) != 0)
   {
      close ( FD );
      return ( 1 );
   }

   if (read (FD, Buffer, 7 ) != 7)
   {
      close (FD);
      return 1;
   }

   CurPos = lseek ( FD, *( (unsigned short *)(Buffer+5)) - 7, SEEK_CUR );
   while ( CurPos < FileSize )
   {
      if (read ( FD, Buffer, 7) != 7)
      {
         close (FD);
         return 1;
      }

      if ( Buffer[2] == 0x74 )
      {
         char *Name;

         if (read ( FD,(char *)&FileHeader, sizeof ( FileHeader )) != sizeof(FileHeader))
         {
            close (FD);
            return 1;
         }

         Name = new char[FileHeader.NameSize+1];
         if (Name == 0 || read ( FD, Name, FileHeader.NameSize) != FileHeader.NameSize)
         {
            close (FD);
            delete [] Name;
            return 1;
         }

         Name[FileHeader.NameSize] = 0;
         if (lseek ( FD, *( (unsigned short *)(Buffer+5)) - 7 - sizeof ( FileHeader ) - FileHeader.NameSize, SEEK_CUR ) == -1)
         {
            close (FD);
            delete [] Name;
            return 1;
         }

         CurPos = lseek ( FD, FileHeader.PackSize, SEEK_CUR );
         unsigned short Flag = *( (unsigned short *) (Buffer+3));
         if (((Flag & 0xE0) != 0xE0) && ((Flag & 0x07) == 0) && MatchWild(Name,Wild) == 1)
         {
            char *FullName;
            FullName = new char[FileHeader.NameSize+1+strlen ( Archive )+1];

            sprintf ( FullName, "%s\\%s", Archive, Name );

            // Add to list
            DirItem Item;
            Item.Name = FullName;
            Item.Flags = 1 << 2;
            Item.Size = FileHeader.FileSize;
            AddItem(Item);
         }
         delete Name;
      }
      else
      {
         if ( *( (unsigned short *)(Buffer+3)) & 0x8000 )
         {
            if (read ( FD, Buffer, 2) != 2)
            {
               close (FD);
               return 1;
            }

            if (lseek ( FD, *( (unsigned short *) (Buffer+5)) - 9, SEEK_CUR ) == -1)
            {
               close (FD);
               return 1;
            }
            CurPos = lseek ( FD, *( (unsigned short *) Buffer), SEEK_CUR );
         }
         else
         {
            CurPos = lseek ( FD, *( (unsigned short *) (Buffer+5)) - 7, SEEK_CUR );
         }

         if (CurPos == -1)
         {
            close (FD);
            return 1;
         }

      }
   }

   close(FD);
   return 0;
}

void SongList::ProcessZipWithZip(const char *Archive, const char *Wild)
{
   if (Wild == "")
      Wild = "*";

   int Fd = SpawnZip("unzip.exe",Archive,"-Cl",Wild);
   if (Fd == 0)
      return;

   FILE *F = fdopen(Fd,"rt");

   char S[300];

   int Lines = 0;
   for (;fgets(S,sizeof(S),F) != 0;)
   {
      // Search for the first header line (skip zip comment)
      if (Lines == 1)
      {
         if (!(S[0] == ' ' && S[1] == 'L' && S[2] == 'e'))
            continue;
      }
      Lines++;

      // Skip header lines
      if (Lines <= 3)
         continue;

      // Skip End lines
      if (S[2] == '-')
         break;

      // Isolate the File Size
      char *Size = S;
      for (;*Size == ' '; Size++);
      char *I = Size;
      for (;*I != ' '; I++);
      *I = 0;
      unsigned long Sz = atoi(Size);

      // Directory
      if (Sz == 0)
         continue;

      // Isolate the file name
      I++;
      for (;*I == ' '; I++);
      char *Name = I + 18;
      I += strlen(I) - 3;
      for (;*I != 0;I++)
         if (*I == '\n' || *I == '\r')
         {
            *I = 0;
            break;
         }

      // Convert to Os/2 slashes (to look nice)
      I = Name;
      for (;*I != 0; I++)
         if (*I == '/')
            *I = '\\';

      // Store the item in the list
      char *Buf = new char[strlen(Name) + strlen(Archive) + 2];
      sprintf(Buf,"%s\\%s",Archive,Name);
      DirItem Item;
      Item.Name = Buf;
      Item.Flags = 2;
      Item.Size = Sz;
      AddItem(Item);
   }
   fclose(F);
}

void SongList::ProcessRARWithRAR(const char *Archive, const char *Wild)
{
   if (Wild == "")
      Wild = "*";

   int Fd = SpawnZip("unrar.exe",Archive,"v -c- -y -av-",Wild);
   if (Fd == 0)
      return;

   FILE *F = fdopen(Fd,"rt");

   char S[300];
   char S2[300];

   int Start = 0;
   for (;fgets(S,sizeof(S),F) != 0;)
   {
      // Skip header lines
      if (S[0] == '-')
      {
         Start = 1;
         continue;
      }

      if (Start == 0)
         continue;

      // Skip End lines
      if (S[0] == '-' || S[0] != ' ')
         break;

      // Isolate the file name
      char *I = S + 1;
      while ( *I != '\n' && *I != '\r' ) I++;
      *I = 0;

      if (fgets(S2,sizeof(S2),F) == 0)
         break;
      char *Size = S2;
      while ( *Size == ' ' ) Size++;
      I = Size;
      while ( *I != ' ' ) I++;
      *I = 0;

      // Convert to Os/2 slashes (to look nice)
      char *Name = S + 1;
      for (I = Name;*I != 0; I++)
         if (*I == '/')
            *I = '\\';

      unsigned long Sz = atoi(Size);

      // Directory
      if (Sz == 0)
         continue;

      // Store the item in the list
      char *Buf = new char[strlen(Name) + strlen(Archive) + 2];
      sprintf(Buf,"%s\\%s",Archive,Name);
      DirItem Item;
      Item.Name = Buf;
      Item.Flags = 1 << 2;
      Item.Size = Sz;
      AddItem(Item);
   }
   fclose(F);
}

/* ########################################################################

   Class - SongList (Handle Creating the song list)
   Member - LoadFile (Reads the file into memory)

   If ForceLoad is 0 then 0 will be returned if it's okay to load from the
   file system.

   ########################################################################
*/
unsigned char *SongList::LoadFile(DirItem &File,char)
{
   // Zip?
   if ((File.Flags & 2) != 0)
   {
      // Extract the Zip path
      const char *ZI = stristr(File.Name,".ZIP");
      if (ZI != 0 && (ZI[4] == '\\' || ZI[4] == '/'))
      {
         char S[300];
         memset(S,0,sizeof(S));
         strncpy(S,File.Name,ZI - File.Name + 4);

         int Fd = SpawnZip("unzip.exe",S,"-Cp",ZI+5);
         if (Fd == -1)
         {
            return 0;
         }
         unsigned char *C = new unsigned char[File.Size];
         unsigned long R = 0;
         while (R != File.Size)
         {
            unsigned long R2 = 0;
            int Rc = DosRead(Fd,(char *)(C+R),File.Size-R,&R2);

            if (R2 == 0 || Rc != 0)
            {
               delete [] C;
               DosClose(Fd);
               DosKillProcess(DKP_PROCESSTREE,ZipPid);
               somPrintf("Error 1\n");
               return 0;
            }

            R += R2;
         }
         DosClose(Fd);
         DosKillProcess(DKP_PROCESSTREE,ZipPid);
         return C;
      }
      else
         return 0;
   }

   // Rar?
   if ((File.Flags & 4) != 0)
   {
      // Extract the Zip path
      const char *ZI = stristr(File.Name,".RAR");
      if (ZI != 0 && (ZI[4] == '\\' || ZI[4] == '/'))
      {
         char S[300];
         memset(S,0,sizeof(S));
         strncpy(S,File.Name,ZI - File.Name + 4);

         int Fd = SpawnZip("unrar.exe",S,"p -c- -av- -o- -y",ZI+5);
         if (Fd == -1)
         {
            return 0;
         }
         unsigned char *C = new unsigned char[File.Size];
         unsigned long R = 0;

         // Locate the ---
         unsigned char *I = 0;
         unsigned char I2 = 0;
         unsigned long R2 = 0;
         while (1)
         {
            if (I == C + R2 || R2 == 0)
            {
               I = C;
               int Rc = DosRead(Fd,(char *)(C+R),File.Size,&R2);
               if (Rc != 0)
               {
                  delete [] C;
                  DosClose(Fd);
                  DosKillProcess(DKP_PROCESSTREE,ZipPid);
                  somPrintf("Error 1\n");
                  return 0;
               }
            }

            for (; *I != '-' && I != C + R2;I++);

            if (*I == '-')
            {
               for (; *I == '-' && I != C + R2; I++, I2++);
               if (I == C + R2)
                  continue;
               if (I2 == 6)
                  break;
               I2 = 0;
            }
         }

         // Skip 2 lines
         int Phase = 1;
         while (1)
         {
            if (I == C + R2 || R2 == 0)
            {
               I = C;
               int Rc = DosRead(Fd,(char *)(C+R),File.Size,&R2);
               if (Rc != 0)
               {
                  delete [] C;
                  DosClose(Fd);
                  DosKillProcess(DKP_PROCESSTREE,ZipPid);
                  somPrintf("Error 1\n");
                  return 0;
               }
            }

            if (Phase == 1)
            {
               for (; *I != '\n' && *I != '\r' && I != C + R2;I++);
               if (I == C + R2)
                  continue;
            }
            Phase = 2;

            for (; (*I == '\n' || *I == '\r') && I != C + R2;I++);
            if (I == C + R2)
               continue;
            break;
         }
         R = (C+R2) - I;
         memmove(C,I,R);

         I = C;
         for (;I != (C+R);I++)
         {
            if (I != C && I[-1] == 0x0D && *I == 0x0A)
            {
               memmove(I-1,I,C+R - I);
               R--;
               I--;
            }
         }

         while (R != File.Size)
         {
            R2 = 0;
            int Rc = DosRead(Fd,(char *)(C+R),File.Size-R,&R2);
            for (;I != (C+R+R2);I++)
            {
               if (I != C && I[-1] == 0x0D && *I == 0x0A)
               {
                  memmove(I-1,I,C+R+R2 - I);
                  R2--;
                  I--;
               }
            }
            if (R2 == 0 || Rc != 0)
            {
               delete [] C;
               DosClose(Fd);
               DosKillProcess(DKP_PROCESSTREE,ZipPid);
               somPrintf("Error 1\n");
               return 0;
            }

            R += R2;
         }
         DosClose(Fd);
         DosKillProcess(DKP_PROCESSTREE,ZipPid);
         return C;
      }
      else
         return 0;
   }
   return 0;
}

/* ########################################################################

   Class - SongList (Handle Creating the song list)
   Member - AddItem (Verifies that the item to be added has a sutible
                     extension)

   Checks the extension to be sure it's valid.

   ########################################################################
*/
void SongList::AddItem(DirItem &Item)
{
   if (Handlers != 0)
      if (Handlers->GetClassForFile(Item.Name) == 0)
      {
         Item.Free();
         return;
      }
   List.push_back(Item);
}

/* ########################################################################

   Class - SongList (Handle Creating the song list)
   Member - SpawnZip (Runs unzip)

   Runs unzip with the passed parameters.

   ########################################################################
*/
int SongList::SpawnZip(const char *Archiver,const char *Archive,const char *Args,const char *Files)
{
   char ArgStr[10000];

   sprintf(ArgStr,"%s %s \"%s\" \"%s\"",Archiver,Args,Archive,Files);
   ArgStr[9] = 0;

   HPIPE hpRead = -1;
   HPIPE hpWrite = -1;

   HFILE Save = -1;
   HFILE New = 1;
   DosDupHandle(1,&Save);
   DosCreatePipe(&hpRead,&hpWrite,PIPE_SIZE);
   DosDupHandle(hpWrite,&New);

   RESULTCODES rec;
   char szFailName[100];
   APIRET Rc = DosExecPgm(szFailName,sizeof(szFailName),EXEC_ASYNC,ArgStr,0,&rec,Archiver);

   ZipPid = rec.codeTerminate;

   DosClose(hpWrite);
   DosDupHandle(Save,&New);

   if (Rc != 0)
   {
      DosClose(hpRead);
      return -1;
   }

   return hpRead;
}

/* ########################################################################

   Class - SongList (Handle Creating the song list)
   Member - WildCards (Evaluates a wildcard in the normal filesystem)

   Simply evaluates the given spec as a directory path.

   ########################################################################
*/
void SongList::WildCards(const char *C,IDLSequence<DirItem> &Seq)
{
   // Make a copy of the directory
   char Name[300];

   DosQueryPathInfo(C,5,Name,sizeof(Name));

   for (char *InsertP = Name + strlen(Name);*InsertP != '\\' && *InsertP != '/' && *InsertP != ':' && InsertP != Name; InsertP--);
   if (InsertP != Name)
      *(InsertP+1) = 0;
   else
      *InsertP = 0;

   HDIR hdirFindHandle = HDIR_SYSTEM;
   FILEFINDBUF3 FindBuffer = {0};      //Returned from FindFirst/Next
   ULONG ulResultBufLen = sizeof(FILEFINDBUF3);
   ULONG ulFindCount = 1;              // Look for 1 file at a time
   APIRET rc = NO_ERROR;               // Return code

   rc = DosFindFirst( C,                    // File pattern - all files
                      &hdirFindHandle,      // Directory search handle
                      FILE_NORMAL + FILE_DIRECTORY,  // Search attribute
                      &FindBuffer,          // Result buffer
                      ulResultBufLen,       // Result buffer length
                      &ulFindCount,         // Number of entries to find
                      FIL_STANDARD);        // Return level 1 file info

   // Keep finding the next file until there are no more files
   while (rc == NO_ERROR)
   {
      char *Buf = new char[strlen(FindBuffer.achName) + strlen(Name) + 1];
      strcpy(Buf,Name);
      strcat(Buf,FindBuffer.achName);
      DirItem Item;
      Item.Name = Buf;
      Item.Size = FindBuffer.cbFile;

      if ((FindBuffer.attrFile & FILE_DIRECTORY) != 0)
         Item.Flags = 1;
      else
         Item.Flags = 0;

      Seq.push_back(Item);

      if (Seq.capacity() - Seq.size() <= 2)
         Seq.reserve(Seq.size() + 101);

      ulFindCount = 1;                      // Reset find count.

      rc = DosFindNext(hdirFindHandle,      // Directory handle
                       &FindBuffer,         // Result buffer
                       ulResultBufLen,      // Result buffer length
                       &ulFindCount);       // Number of entries to find
   }

   rc = DosFindClose(hdirFindHandle);    // Close our directory handle
   return;
};

/* ########################################################################

   Class - SongList (Handle Creating the song list)
   Member - MatchWild (Matches a wildcard spec to a path name)

   Returns 1 if the wild card descibes the given string.

   ########################################################################
*/
int SongList::MatchWild(const char *String,const char *Wild)
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
   return 1;
}
