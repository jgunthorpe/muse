// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   669Format - Muse file format to handle 669 files.

   ##################################################################### */
									/*}}}*/
// Includes								/*{{{*/
#include <muse.h>
#include <669form.h>

#include "669struc.h"
   									/*}}}*/

muse669FormatClass *muse669Format::Meta = new muse669FormatClass;

// 669Format::muse669Format - Constructor				/*{{{*/
// ---------------------------------------------------------------------
/* 0s data members */
muse669Format::muse669Format()
{
   // Init the structures

   Header = 0;
   PatternData = 0;
   SampleData = 0;
   Private = false;
}
									/*}}}*/
// 669Format::Free - Frees the song from memory				/*{{{*/
// ---------------------------------------------------------------------
/* */
void muse669Format::Free()
{
   int x;
   
   if (Header == 0)
      return;
   
   if (Private)
   {
      if (PatternData)
	 for (x = 0; x < Header->NoOfPatterns; x++)
	    delete PatternData[x];

      delete Header;
   }
   
   // Delete the pattern structures
   delete [] PatternData;
   PatternData = 0;

   delete [] SampleData;
   SampleData = 0;

   Header = 0;
   Private = false;

   museFormatBase::Free();
}
									/*}}}*/
// 669Format::LoadMemModule - Loads the module from a memory region	/*{{{*/
// ---------------------------------------------------------------------
/* */
long muse669Format::LoadMemModule(unsigned char *Region, unsigned long Size)
{
   Free();
   
   unsigned char *CurrentPos = Region;
   unsigned char *End = Region + Size;
   
   Header = (F669Header *) CurrentPos;
   
   CurrentPos += sizeof (F669Header);
   if (CurrentPos > End)
   {
      Free();
      return (LOADPART_Header | LOADFAIL_Truncated);
   }
    
   if (Header->Sig != 0x6669)
   {
      Warn("Bad Header");
      Free();
      return (LOADPART_Header | LOADFAIL_Sigcheck);
   }
   
   SampleData = new F669Sample[Header->NoOfSamples];
   memset (SampleData, 0, sizeof (F669Sample) * Header->NoOfSamples);
   
   unsigned int x;
   for (x = 0; x < Header->NoOfSamples; x++)
   {
      if (CurrentPos + sizeof (F669Sample) - sizeof (unsigned char *) > End)
      {
	 Free();
	 return (LOADPART_Instruments | LOADFAIL_Truncated | x);
      }
      memcpy (&SampleData[x], CurrentPos, sizeof (F669Sample));
      CurrentPos += sizeof (F669Sample) - sizeof (unsigned char *);
   }
   
   PatternData = new F669Pattern *[Header->NoOfPatterns];
   
   for (x = 0; x < Header->NoOfPatterns; x++)
   {
      PatternData[x] = (F669Pattern *) CurrentPos;
      CurrentPos += sizeof (F669Pattern);
      if (CurrentPos > End)
      {
	 Free();
	 return (LOADPART_Patterns | LOADFAIL_Truncated | x);
      }
   }
   
   for (x = 0; x < Header->NoOfSamples; x++)
   {
      SampleData[x].SampleData = CurrentPos;
      CurrentPos += SampleData[x].Length;
      if (CurrentPos > End)
      {
	 Free();
	 return (LOADPART_Samples | LOADFAIL_Truncated | x);
      }
   }
   
   return LOADOK;
}
									/*}}}*/
// 669Format::Keep - Clone important internal data			/*{{{*/
// ---------------------------------------------------------------------
/* After this function is called the playback routine will be able to
   execute without having the memory block passed to LoadMemModule 
   resident. However, it will be unable to download the samples which
   are not cloned by the routine. The routine was intended for use with
   wavetable devices that are capable of sample downloading (GUS) */
long muse669Format::Keep()
{
   if (Header == 0 || Private == true)
      return FAIL_NotLoaded;
   
   Private = true;

   // Clone the header.
   F669Header *OldH = Header;
   Header = new F669Header;
   memcpy(Header,OldH,sizeof(F669Header));

   // Clone the pattern data.
   int x;
   for (x = 0; x < Header->NoOfPatterns; x++)
   {
      F669Pattern *OldP = PatternData[x];
      PatternData[x] = new F669Pattern;
      memcpy(PatternData[x],OldP,sizeof(F669Pattern));
   }
   
   return LOADOK;
}
									/*}}}*/
// S3MFormat::GetSongInfo - Returns information about the song		/*{{{*/
// ---------------------------------------------------------------------
/* Simply fills the info structure. */
long muse669Format::GetSongInfo(museSongInfo &Info)
{
   Info.Free();
   
   if (Header == 0)
      return FAIL_NotLoaded;
   
   Info.Patterns = Header->NoOfPatterns;
   Info.Channels = 8;

   // Count the orders
   Info.Orders = 0;
   while (Info.Orders < 128 && Header->OrderList[Info.Orders] != 0xFF) 
      Info.Orders++;

   Info.ClassName = ((museFormatClass *)GetMetaClass())->GetTypeName();
   Info.TypeName = _strdupcpp("Composd Module");
   Info.Title = _strndupcpp(Header->SongMessage,36);

   char *CommentData = Header->SongMessage;
   char *End = CommentData + 108;
   char *C;
   for (C = CommentData; C != End && (*C == ' ' || *C == 0);C++);
   if (C == End)
      return FAIL_None;

   Info.ModComment = new char[108 + 21];

   char *Comm = Info.ModComment;
   for (C = CommentData; C != End;)
   {
      char *Start = C;
      for (;*C != 0 && C != End && (C - Start != 36); C++,Comm++)
         *Comm = *C;
      *Comm = '\n';
      Comm++;
      for (;*C == 0 && C != End; C++);
   }
   *Comm = 0;

   return FAIL_None;
}
									/*}}}*/
// 669Format::GetSongSamples - Return the samples for a song.		/*{{{*/
// ---------------------------------------------------------------------
/* */
void muse669Format::GetSongSamples(SequenceSample &Samples)
{
   if (Header == 0)
      return;

   Samples.construct();
   Samples.reserve(Header->NoOfSamples);

   int I = 0;

   for (int x = 0; x < Header->NoOfSamples; x++,I++)
   {
      museSample &Samp = Samples[I];
      Samp.Sample = SampleData[x].SampleData;
      Samp.SampleEnd = SampleData[x].Length;
      if (Samp.SampleEnd == 0)
         Samp.Sample = 0;

      if (SampleData[x].LoopEnd == 1048575)
      {
         Samp.LoopBegin = 0;
         Samp.LoopEnd = SampleData[x].Length;
         Samp.Flags = 0;
      }
      else
      {
         Samp.LoopBegin = SampleData[x].LoopBegin;
         Samp.LoopEnd = SampleData[x].LoopEnd;
         if (Samp.LoopEnd > Samp.SampleEnd)
            Samp.LoopEnd = Samp.SampleEnd;
         Samp.Flags = (1 << 0);   // Enable looping
      }
      Samp.Name = SampleData[x].FileName;
      Samp.MaxNameLen = 13;
      Samp.SampleName = 0;
      Samp.MaxSNameLen = 0;
      Samp.InstNo = x;
      Samp.Center = 8363;
      Samp.Volume = 0xFF;
   }
}
									/*}}}*/
// 669FormatClass::GetTypeName - Return the generic typename		/*{{{*/
// ---------------------------------------------------------------------
/* */
const char *muse669FormatClass::GetTypeName()
{
   return "Composd Module";
}
									/*}}}*/
// 669FormatClass::GetClassForFile - Return the metaclass for fname	/*{{{*/
// ---------------------------------------------------------------------
/* */
museFormatClass *muse669FormatClass::GetClassForFile(const char *FName)
{
   const char *End = FName + strlen(FName);
   for(;*End != '.' && End != FName;End --);

   if (End != FName)
      End++;

   if (_stricmp(End,"669") != 0)
      return 0;
   return this;
}
									/*}}}*/
// 669FormatClass::GetScanSize - Get the header size			/*{{{*/
// ---------------------------------------------------------------------
/* */
unsigned long muse669FormatClass::GetScanSize()
{
   return sizeof(F669Header);
}
									/*}}}*/
// 669FormatClass::Scan - Scans the header				/*{{{*/
// ---------------------------------------------------------------------
/* */
unsigned long muse669FormatClass::Scan(octet *Region, museSongInfo &Info)
{
   museFormatClass::Scan(Region,Info);

   F669Header *Header = ((F669Header *)Region);

   if (Header->Sig != 0x6669)
      return 1;

   // Get the title
   Info.Title = _strndupcpp(Header->SongMessage,36);
   Info.TypeName = _strdupcpp("Composd Module");
   Info.ClassName = GetTypeName();
   Info.Channels = 8;
   Info.Patterns = Header->NoOfPatterns;
   Info.Orders = 0;
   while (Info.Orders < 128 && Header->OrderList[Info.Orders] != 0xFF)
      Info.Orders++;

   return 0;
}
									/*}}}*/
// 669FormatClass::muse669FormatClass - Constructor			/*{{{*/
// ---------------------------------------------------------------------
/* Sets version information */
muse669FormatClass::muse669FormatClass()
{
   MajorVersion = 1;
   MinorVersion = 2;
}
									/*}}}*/
