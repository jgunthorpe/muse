// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* #####################################################################

   FormatBase - Base class for all Muse/2 Module file formats 

   ##################################################################### */
									/*}}}*/
// Include Files							/*{{{*/
#include <muse.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
   									/*}}}*/

// Unix does not have the concept of binaryness, dos and os2 do.
#ifdef __unix__
# define O_BINARY 0
#endif

// Force template instantiation
template class Sequence<musePlayRec>;
template class Sequence<museFormatClass *>;

// The class object
museFormatClass *museFormatBase::Meta = new museFormatClass;

// SongInfo::museSongInfo - Constructor					/*{{{*/
// ---------------------------------------------------------------------
/* 0's data members */
museSongInfo::museSongInfo()
{
   // These are all found in most file formats
   Title = 0;
   TypeName = 0;
   ClassName = 0;
   Channels = 0;
   Patterns = 0;
   Orders = 0;
   ModComment = 0;

   // These are designed for a filemanager system
   Author = 0;
   AuthComment = 0;
   Style = 0;
   PlayTime = 0;
   FinishDate = 0;
   Rating = CharNull;   
}
									/*}}}*/
// SongInfo::Free - Frees the classes allocated memory.			/*{{{*/
// ---------------------------------------------------------------------
/* */
void museSongInfo::Free()
{
   delete [] Title;
   Title = 0;

   delete [] TypeName;
   TypeName = 0;
   
   delete [] ModComment;
   ModComment = 0;

   delete [] Author;
   Author = 0;
   
   delete [] AuthComment;

   AuthComment = 0;
}
									/*}}}*/

// FormatBase::museFormatBase() - Constructor				/*{{{*/
// ---------------------------------------------------------------------
/* Simply 0 data members */
museFormatBase::museFormatBase() : AllowLoop(true)
{
   MemSong = 0;
}
   									/*}}}*/
// FormatBase::~museFormatBase - Destructor				/*{{{*/
// ---------------------------------------------------------------------
/* Calls free */
museFormatBase::~museFormatBase()
{
   Free();
}
									/*}}}*/
// FormatBase::Free - Frees the song from memory inits the class	/*{{{*/
// ---------------------------------------------------------------------
/* Free any LoadModule() song. */
void museFormatBase::Free()
{
   delete [] MemSong;
   MemSong = 0;
}
									/*}}}*/
// FormatBase::LoadModule - Loads a module from disk			/*{{{*/
// ---------------------------------------------------------------------
/* This simply opens the file and reads the entire module into a single
   memory block. LoadMemModule is then called to create the module 
   context that the formats Play() requires.
   memmap may be a good function to use in future instead of just reading
   the whole module.

   NOTICE - The memory copy -IS- changed! It is not readonly. Some formats
   like S3M are able to advoid changing the image, but formats like XM and
   IT require it to convert the samples from delta encoded to pcm.
 */
long museFormatBase::LoadModule(const char *FileName)
{
   Free();

   // Open the file
   int Handle = open(FileName,O_RDONLY | O_BINARY,S_IREAD | S_IWRITE);
   if (Handle == -1)
      return 1;

   // Determine the size
   long Size = lseek(Handle,0,SEEK_END);
   lseek(Handle,0,SEEK_SET);

   // Get some memory and read it in
   unsigned char *MemSong = new unsigned char[Size + 64];
   
   // Some systems have trouble reading > 64k so we loop. (SMBfsys!)
   int SizeLeft = Size;
   int MaxRead = 0;
   while (SizeLeft != 0)
   {
      int Res = read(Handle,(char *)MemSong + (Size - SizeLeft),SizeLeft);
      if (Res <= 0)
      {
	 delete [] MemSong;
	 return 1;
      }
      MaxRead = max(MaxRead,Res);
      SizeLeft -= Res;
   }

   // Little debug aid for wanked file systems
   if (MaxRead < Size)
   {
      WarnC();
      DebugMessage("Read maxed out at %i on %i",MaxRead,Size);
   }
   
   
   // Use LoadMemModule to configure the modules state.
   if (LoadMemModule(MemSong,Size) != 0)
   {
      delete [] MemSong;
      return 1;
   }

   
   this->MemSong = MemSong;
   close(Handle);

   return 0;
}
									/*}}}*/
// FormatBase::Keep - Copy the module                               	/*{{{*/
// ---------------------------------------------------------------------
/* This just returns a fail code for formats that do not yet support it.
   */
long museFormatBase::Keep()
{
   return FAIL_NotSupported;
}
									/*}}}*/
// FormatBase::GetSongInfo - Returns with gobs of info about the song   /*{{{*/
// ---------------------------------------------------------------------
/* This is provided to ease upgrading from the old style using lots of 
   functions to this more usefull style. It calls the lots of functions
   to fill the structure. */
long museFormatBase::GetSongInfo(museSongInfo &Info)
{
   Info.Free();

   Info.Title = GetTitle();
   Info.TypeName = GetTypeName();
   Info.ClassName = ((museFormatClass *)GetMetaClass())->GetTypeName();
   Info.Channels = GetNumChannels();
   Info.Patterns = GetNumPatterns();
   Info.Orders = GetNumOrders();
   GetSongComment(&Info.ModComment);
   
   return FAIL_None;
}
									/*}}}*/

// FormatClass::museFormatClass - Constructor				/*{{{*/
// ---------------------------------------------------------------------
/* Set the version numbers for the metaclass/class pair */
museFormatClass::museFormatClass()
{
   MajorVersion = 1;
   MinorVersion = 1;
}
									/*}}}*/
// FormatClass::GetScanSize - Return the scan byte count		/*{{{*/
// ---------------------------------------------------------------------
/* This returns the number of bytes required to identify and extract 
   information from this module. It is usually the size of the songs
   header */
unsigned long museFormatClass::GetScanSize()
{
   return 0;
}
									/*}}}*/
// FormatClass::Scan - OLD! Erase it.                    		/*{{{*/
// ---------------------------------------------------------------------
unsigned long museFormatClass::Scan(octet *,museSongInfo *)
{
   return FAIL_NotSupported;
}
									/*}}}*/
// FormatClass::Scan - Return information about the song.		/*{{{*/
// ---------------------------------------------------------------------
/* This should extract basic information about a song before it
   has been loaded into memory. Be sure to call this function to
   initiliaze the structure in derived classes */
unsigned long museFormatClass::Scan(octet *,museSongInfo &)
{
   return FAIL_NotSupported;
}
									/*}}}*/
// FormatClass::ScanFile - Cover function to perform the scan procedure	/*{{{*/
// ---------------------------------------------------------------------
/* Simply reads GetScanSize() bytes from the file and calls Scan() */
unsigned long museFormatClass::ScanFile(const char *FileName,museSongInfo &Info)
{
   long ReadSize = GetScanSize();
   if (ReadSize == 0)
      return 1;

   // Open the file
   int Handle = open(FileName,O_RDONLY | O_BINARY,S_IREAD | S_IWRITE);
   if (Handle == -1)
      return 1;

   ReadSize = min(lseek(Handle,0,SEEK_END),ReadSize);
   lseek(Handle,0,SEEK_SET);

   unsigned char *Header = new unsigned char[ReadSize + 64];
   if (read(Handle,(char *)Header,ReadSize) != ReadSize)
   {
      delete [] Header;
      return 1;
   }

   unsigned long Rc = Scan(Header,Info);

   delete [] Header;
   close(Handle);
   return Rc;
}
									/*}}}*/

// PlayerControl::musePlayerControl - Constructor			/*{{{*/
// ---------------------------------------------------------------------
/* Initializes the jump queue. */
musePlayerControl::musePlayerControl()
{
   JumpList.construct();
   JumpList.reserve(10);
}
   									/*}}}*/
// PlayerControl::~musePlayerControl - Destructor			/*{{{*/
// ---------------------------------------------------------------------
/* */
musePlayerControl::~musePlayerControl()
{
   JumpList.free();
}
									/*}}}*/
// PlayerControl::Advance - Advance a number of Rows/Orders		/*{{{*/
// ---------------------------------------------------------------------
/* Advances the given number of rows/orders which may be negative. */
void musePlayerControl::Advance(long Rows,long Orders)
{
   musePlayRec R;
   R.Type = PlayAdvance;
   R.Row = Rows;
   R.Order = Orders;
   JumpList.push_backv(R);
}
									/*}}}*/
// PlayerControl::Jump - Jump to a specific location 			/*{{{*/
// ---------------------------------------------------------------------
/* Absolute jump */
void musePlayerControl::Jump(unsigned long Row,unsigned long Order)
{
   musePlayRec R;
   R.Type = PlayJump;
   R.Row = Row;
   R.Order = Order;
   JumpList.push_backv(R);
}
									/*}}}*/
// PlayerControl::Get - Returns the next PlayRec			/*{{{*/
// ---------------------------------------------------------------------
/* Used by Play() to return the next command record in sequence */
void musePlayerControl::Get(musePlayRec *Rec)
{
   if (JumpList.pop_front(*Rec) == 0)
      Rec->Type = 0;
}
									/*}}}*/
