// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   FormatBase - Base class for all Muse/2 File Formats
   
   This class defines the basic interfaces for both the metaclass and 
   the actual class. 
   
   Various derived classes contain the code to play and inquire file
   formats supported by muse.

   ##################################################################### */
									/*}}}*/
#ifndef FORMATBS_H
#define FORMATBS_H

class museFormatBase;
class museFormatClass;
class musePlayerControl;

// This represents each buffered command
struct musePlayRec
{
   unsigned long Type;
   unsigned long Row;
   unsigned long Order;
};

// Struct to represent played rows. (These are commonly used in the formats)
struct PlayedRec
{
   unsigned long Rows[8];
};

struct Command
{
   unsigned long Command;
   unsigned long Command2;
};

/* Alot of information about a song.
   Large amounts of this are not used as of yet ;>
   It will auto-free it's contents when it goes out of scope. */
class museSongInfo
{
   public:
   
   // These are all found in most file formats
   char *Title;
   char *TypeName;
   const char *ClassName;
   unsigned char Channels;
   unsigned short Patterns;
   unsigned short Orders;
   char *ModComment;

   // These are designed for a filemanager system
   char *Author;
   char *AuthComment;
   char *Style;
   unsigned short PlayTime;
   unsigned long FinishDate;
   unsigned char Rating;
   
   void Free();
   museSongInfo();
   ~museSongInfo() {Free();};
};

// Sequence instantiations in this module
#define SequencePlayRec Sequence<musePlayRec>
#define SequenceFormatClass Sequence<museFormatClass *>

class musePlayerControl : public MetaObject
{
   SequencePlayRec JumpList;

   public:
   void Advance(long Rows,long Orders);
   void Jump(unsigned long Row,unsigned long Order);
   void Get(musePlayRec *Rec);

   musePlayerControl();
   ~musePlayerControl();
};

class museFormatBase : public MetaObject
{
   octet *MemSong;                     // Pointer to our loaded mem block
   public:
   static museFormatClass *Meta;
   inline virtual MetaClass *GetMetaClass();

   // Provide a simple general disk loader
   virtual long LoadModule(const char *File);

   // These should all be overriden by derived classes
   virtual void Free();
   virtual long LoadMemModule(octet *Region,unsigned long Size) = 0;
   virtual long Keep();
   virtual void GetSongSamples(SequenceSample &Samples) = 0;
   virtual long Play(museOutputBase *Device,musePlayerControl *Control) = 0;
   virtual long GetSongInfo(museSongInfo &Info);
   
   // These are all being replaced with GetSongInfo.
   virtual unsigned short GetNumPatterns() {return 0;};
   virtual unsigned short GetNumOrders() {return 0;};
   virtual unsigned short GetRowsAPattern() {return 0;};
   virtual char *GetTitle() {return 0;};
   virtual unsigned short GetNumChannels() {return 0;};
   virtual void GetSongComment(char **Comment) {*Comment = 0;};
   virtual char *GetTypeName() {return 0;};

   // This should move to PlayerControl
   bool AllowLoop;

   museFormatBase();
   ~museFormatBase();
};

class museFormatClass : public MetaClass
{
   public:

   virtual const char *GetTypeName() {return 0;};
   virtual museFormatClass *GetClassForFile(const char *) {return 0;};

   virtual unsigned long GetScanSize();
   virtual unsigned long Scan(octet *Region,museSongInfo &Info);
   virtual unsigned long ScanFile(const char *File,museSongInfo &Info);

   // OLD -- REMOVE
   virtual unsigned long Scan(octet *Region,museSongInfo *Info);
   
   virtual bool DescendedFrom(MetaClass *Base)
   {
      if (Base == museFormatBase::Meta)
         return true;
      return MetaClass::DescendedFrom(Base);
   }
   virtual MetaObject *New() {return 0;};
   virtual const char *GetName() {return "museFormatBase";};

   virtual void Link() {};
   museFormatClass();
};

MetaClass *museFormatBase::GetMetaClass() {return Meta;};

#endif
