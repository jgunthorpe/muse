/* ########################################################################

   Node Type classes

   ########################################################################
*/

#include <Muse.h>

#include "SongLst.h"
#include "SngTypes.h"
#include <ctype.h>
#include <iomanip.h>

/* ########################################################################

   Class - DisksType (Parent of the list of disks)
   Member - Refresh (Refreshes the list)

   Maps the list of disks to the currently inserted disks.

   ########################################################################
*/
void DisksType::Refresh(Song *Head)
{
   ULONG Disk;
   ULONG DiskMap;

   /* Remove disks no longer in the disk map, this only blanks the name,
      doesn't kill the disk. */
   DosQueryCurrentDisk(&Disk,&DiskMap);
   for (Song *I = Head->Child; I != 0; I = I->Next)
   {
      if (I->TypeId != 1)
         continue;

      if (I->Name != 0 && (DiskMap & (1 << (toupper(I->Name[0]) -'A'))) == 0)
         FreeString(I->Name);
   }

   // Scan all disks for
   for (char C = 'A';C != 'Z'; C++)
   {
      Disk = C - 'A' + 1;
      FSINFO Inf;
      unsigned long VId = 0;
      int Error = 0;

      // Woops, not in the disk map, abort.
      if ((DiskMap & (1 << (C -'A'))) == 0)
         continue;

      // Get the disk info block
      DosError(FERR_DISABLEHARDERR);
      if (DosQueryFSInfo(Disk,FSIL_VOLSER,&Inf,sizeof(Inf)) != 0)
         Error = 1;
      DosError(FERR_ENABLEHARDERR);

      // Serial # is the creation date casted to a long
      VId = *((unsigned long *)(&Inf.fdateCreation));

      // Nice, no serial #
      unsigned long VHash = 0;
      if (VId == 0)
         VHash = Hash(Inf.vol.szVolLabel);

      // Search for this volume ID and this drive
      Song *Match = 0;
      Song *Remove = 0;
      for (I = Head->Child; I != 0; I = I->Next)
      {
         if (I->TypeId == 1)
         {
            // Convert the bit fields into an unsinged long
            if (VHash == 0 && I->Size == VId)
               Match = I;

            if (VHash != 0)
               if (I->Info.Title != 0 && GetHash(I->Info.Title) == VHash && strcmp(I->Info.Title,Inf.vol.szVolLabel) == 0)
                  Match = I;

            if (I->Name != 0 && I->Name[0] == C)
               Remove = I;
         }
      }
      if (Error == 1)
         Match = 0;

      // Remove the previouse drive attached to this disk
      if (Remove != 0 && Remove != Match)
      {
         FreeString(Remove->Name);
         Remove->Name = 0;
      }

      if (Error == 1)
         continue;

      // New disk
      if (Match == 0)
      {
         Match = GetNewSong();
         Match->TypeId = 1;
         Match->Size = VId;

         // Add root directory
         Match->Child = GetNewSong();
         Match->TypeId = 2;
      }

      // Disk has moved
      if (Remove != Match)
      {
         FreeString(Match->Name);
         char S[3];
         S[0] = C;
         S[1] = ':';
         S[2] = 0;
         Match->Name = GetString(S);

         Insert(Head,Match); // Resort;
      }

      // Update the volume lable
      if (Match->Info.Title == 0 || strcmp(Match->Info.Title,Inf.vol.szVolLabel) != 0)
      {
         FreeString(Match->Info.Title);
         Match->Info.Title = GetString(Inf.vol.szVolLabel);
      }
   }
}

/* ########################################################################

   Class - DirectoryType (Parent of the list of files in a directory)
   Member - Refresh (Refreshes the list)

   Updates the directory contents to match the on disk stuff.

   ########################################################################
*/
void DirectoryType::Refresh(Song *Head)
{
   // Make a copy of the directory
   char Name[300];
   Owner->MakePath(Head,Name);
   strcat(Name,"*");

   printf("%s\n",Name);

   // Set all the sizes to 0
   for (Song *I = Head->Child; I != 0; I = I->Next);
      if (I->TypeId == 3 || I->TypeId == 2)
         I->Size = 0;

   HDIR hdirFindHandle = HDIR_SYSTEM;
   ULONG SearchSize = 64*1024/sizeof(FILEFINDBUF3);
   FILEFINDBUF3 *FindBuffer = new FILEFINDBUF3[SearchSize];      //Returned from FindFirst/Next
   ULONG ulResultBufLen = sizeof(FILEFINDBUF3)*SearchSize;
   ULONG ulFindCount = SearchSize;              // Look for 100 files at a time
   APIRET rc = 0;               // Return code

   rc = DosFindFirst( Name,                    // File pattern - all files
                      &hdirFindHandle,      // Directory search handle
                      FILE_NORMAL + FILE_DIRECTORY,  // Search attribute
                      FindBuffer,          // Result buffer
                      ulResultBufLen,       // Result buffer length
                      &ulFindCount,         // Number of entries to find
                      FIL_STANDARD);        // Return level 1 file info

   // Keep finding the next file until there are no more files
   while (rc == 0)
   {
      FILEFINDBUF3 *C = FindBuffer;
      for (int I = 0; I != ulFindCount; I++)
      {
         char Skip = 0;
         if (C->achName[0] == '.' &&
             C->achName[1] == 0)
            Skip = 1;
         if (C->achName[0] == '.' &&
             C->achName[1] == '.' &&
             C->achName[2] == 0)
            Skip = 1;

         if (Skip == 0)
         {
            // Search to see if it's here
            for (I = Head->Child; I != 0; I++)
            {
            }

            Song *S = Owner->GetNewSong();
            S->Name = Owner->GetString(C->achName);
            S->Size = C->cbFile;

            Owner->Insert(Head,S);
            if ((C->attrFile & FILE_DIRECTORY) != 0)
              S->TypeId = TypeId;
            else
              S->TypeId = 2;
         }
         C = (FILEFINDBUF3 *)(((char *)C) + C->oNextEntryOffset);
      }
      ulFindCount = SearchSize;                      // Reset find count.

      rc = DosFindNext(hdirFindHandle,      // Directory handle
                       FindBuffer,         // Result buffer
                       ulResultBufLen,      // Result buffer length
                       &ulFindCount);       // Number of entries to find
   }
   delete [] FindBuffer;
   rc = DosFindClose(hdirFindHandle);    // Close our directory handle

/*   for (Song *I = Head->Child; I != 0; I = I->Next)
      if (I->TypeId == 1)
         Owner->Types[1]->Refresh(I);
*/
   return;
}
