
// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   GZip - This is a simple archiver class for Muse, it handles gzip'd
          files.
   
   ##################################################################### */
									/*}}}*/
// Includes								/*{{{*/
#include <muse.h>
#include <songlist.h>
#include <gzip.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

   									/*}}}*/

museGZipClass *museGZipArchiver::Meta = new museGZipClass;

// GZipClass::museGZipClass - Constructor				/*{{{*/
// ---------------------------------------------------------------------
/* Sets the version numbers */
museGZipClass::museGZipClass()
{
   MajorVersion = 1;
   MinorVersion = 0;
}
									/*}}}*/
// GZipClass::IsArchivePath - Returns true if .gz/ is present		/*{{{*/
// ---------------------------------------------------------------------
/* */
bool museGZipClass::IsArchivePath(const char *Path)
{
   // Check to see if its a zip path
   const char *ZI = _stristr(Path,".gz");
   if (ZI != 0 && (ZI[3] == '\\' || ZI[3] == '/'))
      return true;
   return false;
}
									/*}}}*/
// GZipClass::IsArchiveFile - Returns true if path ends in .gz		/*{{{*/
// ---------------------------------------------------------------------
/* */
bool museGZipClass::IsArchiveFile(const char *Path)
{
   const char *End = Path + strlen(Path);
   for(;*End != '.' && End != Path;End --);
   
   if (End != Path)
      End++;
   
   if (_stricmp(End,"GZ") != 0)
      return false;
   return true;
}
									/*}}}*/
// GZipClass::ProcessArchive - Exctract the file names from an archive	/*{{{*/
// ---------------------------------------------------------------------
/* This simply checks that the .gz file exists in the file system. */
void museGZipClass::ProcessArchive(const char *Path,museSongList &List)
{
   // Buffer overflow check.
   if (strlen(Path) > 150)
      return;
   
   // Clip off any garbage after the .gz 
   char *C = _stristr(Path,".gz");
   if (C == 0)
      return;
   
   // S is now blah/foo.gz
   char S[300];
   strncpy(S,Path,C - Path + 4);
   S[C-Path+4] = 0;
   
   // Stat the gzip file
   struct stat Inf;
   stat(S,&Inf);

   // Check if it is a directory.
   if (S_ISDIR(Inf.st_mode) != 0)
      return;

   // Build the final filename
   char F[300];
   strcpy(F,S);
   
   // Strip off the .gz
   S[C-Path] = 0;
   
   // Stip off the filename
   for (C = S + strlen(S); C != S && *C != '/' && *C != '\\'; C--);

   // Cat it to the final filename
   if (C == S)
   {
      F[strlen(F)+1] = 0;
      F[strlen(F)] = museSongList::DirSep;
   }
   
   strcat(F,C);
   
   museSongItem Item;
   Item.FileName = _strdupcpp(F);
   Item.CompressedSize = Inf.st_size;
   Item.Flags |= SITEM_Archive;
   List.AddItem(Item);
}
									/*}}}*/
// GZipClass::ExtractArchive - Extracts the archive file into memory	/*{{{*/
// ---------------------------------------------------------------------
/* Because gzip doesn't store the final archive size we have to extract
   to a temp file and then load the temp file into memory.. */
bool museGZipClass::ExtractArchive(museSongItem &Item,octet *&Block,
				      unsigned long &Size)
{
   Block = 0;
   Size = 0;

   // Buffer overflow check.
   if (strlen(Item.FileName) > 300)
      return false;
   
   char *C = _stristr(Item.FileName,".gz");
   char S[300];
   strncpy(S,Item.FileName,C - Item.FileName + 3);
   S[C - Item.FileName + 4] = 0;
   printf("%s\n",S);

   return false;
}
									/*}}}*/
