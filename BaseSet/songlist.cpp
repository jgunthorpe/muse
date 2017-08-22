// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   SongList - Class to encapsulate a song list
   
   Be carefull with how the museSongItem is constructed, in the IDLSeq
   it never has it's constructor called, the sequence mearly 0's the
   entire data block -- It was designed to store structures not classes.
   
   As long as the class is just a 'smart' structure it will be okay.
   
   ##################################################################### */
									/*}}}*/
// Includes								/*{{{*/
#include <muse.h>
#include <songlist.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
   									/*}}}*/

// Unix does not have the concept of binaryness, dos and os2 do.
#ifdef __unix__
# define O_BINARY 0
#endif

// Instantiate the templates
template class Sequence<museSongItem>;
museArchiverClass *museArchiverBase::Meta = new museArchiverClass;

char museSongList::DirSep = '/';

// SongItem - A Single songlist item.         				/*{{{*/
// ---------------------------------------------------------------------
/* Just some simple members, the FileName string is always newed []. Because
   of the way ownership is transfered the destructor cannot call Free
   automatically.. (AddItem) */
museSongItem::museSongItem()
{
   FileName = 0;
   Flags = 0;
}

void museSongItem::Free()
{
   delete [] FileName;
   FileName = 0;
   Flags = 0;
}
   									/*}}}*/

// SongList::Add - Add a Path to the song list				/*{{{*/
// ---------------------------------------------------------------------
/* This will perform full wildcard expansion and matching against the 
   format class list.
 
   What is done is first the given path is passed to the wildcard expander
   which generates a list from the filesystem. If that doesn't give anything
   then a * is appended to Path. This makes it a bit simpler to use for the
   user. If the above only result in 1 matched directory then that entire
   directory is brought in. Archives are handled by checking each path
   for archive extentions (.zip etc) and then invoking the proper sub 
   handler for that -- the handler will parse the archive looking for matching
   songs. 
 */
unsigned long museSongList::Add(const char *Path)
{
   museArchiverClass **Archivers = (museArchiverClass **)museArchiverBase::Meta->GetDerived();

   // Check to see if it is an archiver path
   museArchiverClass **Arc;
   for (Arc = Archivers; *Arc != 0; Arc++)
      if ((*Arc)->IsArchivePath(Path) == true)
      {
	 unsigned long OldSize = List.size();
	 (*Arc)->ProcessArchive(Path,*this);
	 return List.size() - OldSize;
      }
      
   SequenceSongItem DirList;
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

   // Only 1 item, see if its a directory
   if (DirList.size() == 1)
   {
      // Is a directory
      if ((DirList[0].Flags & 1) != 0)
      {
         char S[300];
	 sprintf(S,"%s%c*",DirList[0].FileName,DirSep);

	 DirList[0].Free();
         DirList.erasefrom(DirList.begin());
         WildCards(S,DirList);
      }
   }

   // Go and add each found item.
   unsigned long OldSize = List.size();
   for (iterator I = DirList.begin(); I != DirList.end(); I++)
   {
      // Not a Directory
      if ((I->Flags & SITEM_Dir) == 0)
      {
	 for (Arc = Archivers; *Arc != 0; Arc++)
	    if ((*Arc)->IsArchiveFile(I->FileName) == true)
	       (*Arc)->ProcessArchive(I->FileName,*this);

	 if (*Arc == 0)
	    AddItem(*I);
	 else
	    I->Free();
      }
      else
         I->Free();
   }
   
   /* This is safe because anything that was not directly added has been
      erased. */
   DirList.free();
   
   return List.size() - OldSize;
}
									/*}}}*/
// SongList::AddItem - Add a single item to the list			/*{{{*/
// ---------------------------------------------------------------------
/* Note that ownership of the item is transfered to the list, free
   must not be called! This checks the item agaist the supported file
   formats to ensure that it matches */
bool museSongList::AddItem(museSongItem &Item)
{
   museFormatClass **Formats = (museFormatClass **)museFormatBase::Meta->GetDerived();

   // Check for a matching file format
   for (; *Formats != 0; Formats++)
      if ((*Formats)->GetClassForFile(Item.FileName) != 0)
	 break;

   // No Match
   if (*Formats == 0)
   {
      Item.Free();
      return false;
   }
   
   // Add it to the list
   List.push_backv(Item);
   return true;
}
   									/*}}}*/
// SongList::WildCards - Generate a list of files matching Path		/*{{{*/
// ---------------------------------------------------------------------
/* This performs wildcard expansion on the given path and returing the
   set of matches in List. The called must free the items in list. 
 
   Posix compliant functions are used to read the directory information
   so any compatible compiler should have no problems with this function.
 */
void museSongList::WildCards(const char *Path,SequenceSongItem &List)
{
   char S[300];
   strncpy(S,Path,sizeof(S));
   S[sizeof(S)-1] = 0;
   
   // First we parse off the directory portion,
   char *C;
   for (C = S + strlen(S); C != S && *C != DirSep; C--);
   
   char *Match = S;
   char *Dir = ".";
   if (C != S)
   {
      *C = 0;
      Dir = S;
      Match = C+1;
   }
   
   // Open the directory
   struct DIR *DirFd = opendir(Dir);
   if (Dir == 0)
      return;

   // Parse over it
   struct dirent *Ent;
   for (Ent = readdir(DirFd); Ent != 0; Ent = readdir(DirFd))
   {
      // Skip . and .. 
      if (strcmp(Ent->d_name,".") == 0 || strcmp(Ent->d_name,"..") == 0)
	 continue;
      
      // Perform wildcard matching 
      if (MatchWild(Ent->d_name,Match) == true)
      {
	 // Buffer overflow :> Frigging sprintf.
	 char F[300];

	 // Prepend the directory if needed
	 if (Match == S)
	 {
	    strncpy(F,Ent->d_name,sizeof(F));
	    F[sizeof(F)-1] = 0;
	 }
	 else
	    sprintf(F,"%s%c%s",Dir,DirSep,Ent->d_name);
   
	 // Stat the item to see if it is a directory
	 struct stat Inf;
	 stat(F,&Inf);
	    
	 museSongItem Item;
	 Item.FileName = _strdupcpp(F);
	 
	 // Check if it is a directory
	 if (S_ISDIR(Inf.st_mode) != 0)
	    Item.Flags |= SITEM_Dir;
	 Item.Size = Inf.st_size;
	 
	 List.push_backv(Item);
      }
   };  
}
									/*}}}*/
// SongList::MatchWild - Wildcard string matcher			/*{{{*/
// ---------------------------------------------------------------------
/* This is a simple recursive string matcher. It handles * and ?. It 
   was designed to match against file names and knows that 
   Foo\bar does not match Foo*. It matches against both kinds of slashes
   because zip sometimes returns the opposite one for the platform, likely
   ditto for alot of other archivers. 
 
   It is CASE INSENSITIVE which might confuse unix users.. In the long run 
   it makes sense to be like that, if you are using Muse's matcher then you 
   are probably scanning a zip file and case senitivity is a pain then. */
bool museSongList::MatchWild(const char *String,const char *Wild)
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
               return true;
            return false;
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

      // Check the characters normally, also check for inverted slashes
      if (*I == '\\' && *I2 == '/')
         continue;
      if (*I == '/' && *I2 == '\\')
         continue;
      if (toupper(*I) != toupper(*I2))
         return false;
   }

   if (*I == 0 && *I2 == 0)
      return true;
   return false;
}
									/*}}}*/
// SongList::Load - Load a file into memory				/*{{{*/
// ---------------------------------------------------------------------
/* This will extract the file from an archive if needed. */
bool museSongList::Load(museSongItem &Item,octet *&Block,
			   unsigned long &Size)
{
   Block = 0;
   Size = 0;
   
   if ((Item.Flags & SITEM_Archive) != 0)
   {
      museArchiverClass **Archivers = (museArchiverClass **)museArchiverBase::Meta->GetDerived();
      
      // Check to see if it is an archiver path
      museArchiverClass **Arc;
      for (Arc = Archivers; *Arc != 0; Arc++)
	 if ((*Arc)->IsArchivePath(Item.FileName) == true)
	    return (*Arc)->ExtractArchive(Item,Block,Size);

      Error("Couldn't match archiver");
      return false;
   }
   
   // This is ripped out of formatbase
   // Open the file
   int Handle = open(Item.FileName,O_RDONLY | O_BINARY,S_IREAD | S_IWRITE);
   if (Handle == -1)
      return 1;

   // Determine the size
   Size = lseek(Handle,0,SEEK_END);
   lseek(Handle,0,SEEK_SET);

   // Get some memory and read it in
   Block = new unsigned char[Size + 64];

   // Some systems have trouble reading > 64k so we loop. (SMBfsys!)
   int SizeLeft = Size;
   int MaxRead = 0;
   while (SizeLeft != 0)
   {
      int Res = read(Handle,(char *)Block + (Size - SizeLeft),SizeLeft);
      if (Res <= 0)
      {
	 delete [] Block;
	 return false;
      }
      MaxRead = max(MaxRead,Res);
      SizeLeft -= Res;
   }

   // Little debug aid for wanked file systems
   if (MaxRead < (int)Size)
   {
      WarnC();
      DebugMessage("Read maxed out at %i on %i",MaxRead,Size);
   }   

   return true;
}
									/*}}}*/
