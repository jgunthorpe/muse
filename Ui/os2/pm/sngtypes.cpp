/* ########################################################################

   Node Type classes

   ########################################################################
*/

#include <Muse.h>

#include "SngList.h"
#include "SngTypes.h"
#include <ctype.h>
#include <iomanip.h>

int DisksType::ID = 5;
int DiskType::ID = 6;
int DirectoryType::ID = 7;

/* ########################################################################

   Class - DisksType (Parent of the list of disks)
   Member - Refresh (Refreshes the list)

   Maps the list of disks to the currently inserted disks.

   ########################################################################
*/
int DisksType::Refresh(iterator Head)
{
   ULONG Disk;
   ULONG DiskMap;

   DosQueryCurrentDisk(&Disk,&DiskMap);

   // Remove drives that no longer exist
   for (iterator I = Head->Child; I != 0; I++)
   {
      if (I->TypeId != DiskType::ID)
         continue;

      if (I->Name != 0 && (DiskMap & (1 << (toupper(I->Name[0]) -'A'))) == 0)
         Free(I->Name);
   }

   // Check each drive.
   for (char C = 'A';C != 'Z'; C++)
   {
      Disk = C - 'A' + 1;
      FSINFO Inf;
      unsigned long VId = 0;
      int Error = 0;

      // Is this drive installed?
      if ((DiskMap & (1 << (C -'A'))) == 0)
         continue;

      // Turn off the error box and query
      DosError(FERR_DISABLEHARDERR);
      if (DosQueryFSInfo(Disk,FSIL_VOLSER,&Inf,sizeof(Inf)) != 0)
         Error = 1;
      DosError(FERR_ENABLEHARDERR);
      VId = *((unsigned long *)(&Inf.fdateCreation));

      // Just in case no serial #
      unsigned long VHash = 0;
      VHash = Hash(Inf.vol.szVolLabel);

      // Search for this volume ID and this drive
      iterator Match = 0;
      iterator Remove = 0;
      for (I = Head->Child; I != 0 && Match == 0 && Remove == 0; I++)
      {
         if (I->TypeId == DiskType::ID)
         {
            // Convert the bit fields into an unsinged long
            if (VId != 0 && I->Size == VId)
               Match = I;

            // Check the volume label
            if (VId == 0 && I->Info != 0)
               if (IsEqual(I->Info->Title,Inf.vol.szVolLabel,VHash) != 0)
                  Match = I;

            // Check the drive letter, if it's matched then remove it
            if (I->Name != 0 && I->Name[0] == C)
               Remove = I;
         }
      }
      if (Error == 1)
         Match = 0;

      // Remove the previouse drive attached to this disk
      if (Remove != 0 && Remove != Match)
      {
         Free(Remove->Name);
         Remove->Name = 0;
         ReLocate(Remove);
      }

      if (Error == 1)
         continue;

      // New disk
      if (Match == 0)
      {
         Match = New(Head,0,VId,DiskType::ID);
         Match->Refreshed = 1;
         Match->Directory = 1;

         // Add root directory
         New(Match,0,0,DirectoryType::ID)->Directory = 1;
      }

      // Disk has moved
      if (Remove != Match)
      {
         Free(Match->Name);
         char S[2];
         S[0] = C;
         S[1] = 0;
         Match->Name = New(S);
         ReLocate(Match);
      }

      // Update the volume lable
      if (Match->Info == 0)
         Match->Info = NewInfo();

      if (IsEqual(Match->Info->Title,Inf.vol.szVolLabel,VHash) == 0)
      {
         Free(Match->Info->Title);
         Match->Info->Title = New(Inf.vol.szVolLabel);
      }
   }

   return 0;
}

/* ########################################################################

   Class - DiskType (A disk)
   Member - Refresh (Checks to see if the disk is mounted)

   1 is returned if the disk is not present.

   ########################################################################
*/
int DiskType::Refresh(iterator Head)
{
   // Query the disk, free if not present
   FSINFO Inf;
   DosError(FERR_DISABLEHARDERR);
   if (DosQueryFSInfo(Head->Name[0] - 'A' + 1,FSIL_VOLSER,&Inf,sizeof(Inf)) != 0)
   {
      Free(Head->Name);
      Head->Name = 0;
   }
   DosError(FERR_ENABLEHARDERR);

   // Hmm, not mounted..
   if (Head->Name == 0)
      Owner->Refresh(Head->Parent);
   if (Head->Name == 0)
      return 1;

   // Check if it's actually the correct disk
   unsigned long VId = 0;
   VId = *((unsigned long *)(&Inf.fdateCreation));
   if (VId == 0 && Head->Size == 0)
   {
      if (strcmp(Inf.vol.szVolLabel,Head->Info->Title) != 0)
         Owner->Refresh(Head->Parent);
   }
   else
   {
      if (VId != Head->Size )
         Owner->Refresh(Head->Parent);
   }

   if (Head->Name != 0)
      return 0;
   return 1;
}

/* ########################################################################

   Class - DirectoryType (A directory)
   Member - Refresh (Refreshes the list)

   Regens the given directory

   1 is returned if the disk is not present.

   ########################################################################
*/
int DirectoryType::Refresh(iterator Head)
{
   // Find the disk
   for (iterator I = Head; I != 0 && I->TypeId != DiskType::ID; I = I->Parent);

   // Neat, corrupted
   if (I == 0)
      return 1;
   if (Owner->Refresh(I) != 0)
      return 1;

   char Name[300];
   MakePath(Head,Name);
   strcat(Name,"*");
   printf("%s\n",Name);

   // If we don't set there here our search calls will go insane :>
   Head->Refreshed = 1;

   // Set kill flag to 1
   for (I = Head->Child; I != 0; I++)
      I->TempKill = 1;

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

   // Ooh, directory is nuked
   if (rc != 0)
   {
      Free(Head);
      return 1;
   }

   // Keep finding the next file until there are no more files
   iterator Node;
   while (rc == 0)
   {
      FILEFINDBUF3 *C = FindBuffer;
      for (int I = 0; I != ulFindCount; I++,
                  C = (FILEFINDBUF3 *)(((char *)C) + C->oNextEntryOffset))
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
            Node = Search(Head,C->achName,Node);

            // Has it become a directory or vice versa??
            if (Node != 0 && (((C->attrFile & FILE_DIRECTORY) != 0 && Node->TypeId != DirectoryType::ID) ||
                ((C->attrFile & FILE_DIRECTORY) == 0 && Node->TypeId == DirectoryType::ID)))
            {
               Free(Node);
               Node = 0;
            }

            // Make a new one
            if (Node == 0)
               if ((C->attrFile & FILE_DIRECTORY) != 0)
               {
                  Node = New(Head,C->achName,C->cbFile,DirectoryType::ID);
                  Node->Directory = 1;
               }
               else
               {
                  Node = New(Head,C->achName,C->cbFile,0);
                  if (Node == 0)
                     continue;
                  if (Node->Directory == 0)
                     Node->Refreshed = 1;
               }
            Node->TempKill = 0;
            Node->Size = C->cbFile;

            // Case on file names
            if (strcmp(Node->Name,C->achName) != 0)
            {
               Free(Node->Name);
               Node->Name = New(C->achName);
            }
         }
      }
      ulFindCount = SearchSize;                      // Reset find count.

      rc = DosFindNext(hdirFindHandle,      // Directory handle
                       FindBuffer,          // Result buffer
                       ulResultBufLen,      // Result buffer length
                       &ulFindCount);       // Number of entries to find
   }
   delete [] FindBuffer;
   rc = DosFindClose(hdirFindHandle);    // Close our directory handle

   // Kill
   for (I = Head->Child; I != 0; I++)
      if (I->TempKill == 1)
      {
         iterator T = I;
         T--;
         Free(I);
         I = T;
      }

   return 0;
}
