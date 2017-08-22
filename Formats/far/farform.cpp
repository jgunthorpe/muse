// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   FARFormat - Muse file format to handle FAR files.
   
   ##################################################################### */
									/*}}}*/
// Includes								/*{{{*/
#include <muse.h>
#include <farform.h>

#include "farstruc.h"
   									/*}}}*/

museFARFormatClass *museFARFormat::Meta = new museFARFormatClass;

// FARFormat::museFARFormat - Constructor				/*{{{*/
// ---------------------------------------------------------------------
/* 0s data members */
museFARFormat::museFARFormat()
{
   // InFAR the structures

   Header = 0;
   Orders = 0;
   Samples = 0;
   Patterns = 0;
   Private = false;
}
									/*}}}*/
// FARFormat::Free - Frees the module					/*{{{*/
// ---------------------------------------------------------------------
/* Frees the module and cloned data */
void museFARFormat::Free()
{
   int x;
   
   if (Header == 0)
      return;
   
   if (Private)
   {
      if (Patterns)
	 for (x = 0; x < 256; x++)
	    delete [] Patterns[x];
      
      if (Samples)
	 for (x = 0; x < 64; x++)
	    delete [] Samples[x];
      
      delete [] SampleMap;
      delete Orders;
      delete Header;
   }
   
   delete Samples;
   Samples = 0;
   SampleMap = 0;
   delete Patterns;
   Patterns = 0;
   Orders = 0;
   Header = 0;
   Private = false;

   museFormatBase::Free();
}
									/*}}}*/
// FARFormat::LoadMemModule - Loads the module from a memory region	/*{{{*/
// ---------------------------------------------------------------------
/* */
long museFARFormat::LoadMemModule(unsigned char *Region, unsigned long Size)
{
   int x;
   Free();
   
   unsigned char *CurrentPos = Region;
   unsigned char *End = Region + Size;
   
   Header = (FARHeader *) CurrentPos;
   CurrentPos += sizeof (FARHeader);
   if (CurrentPos > End)
   {
      Free();
      return (LOADPART_Header | LOADFAIL_Truncated);
   }
   
   CurrentPos += Header->TextLength;
   if (CurrentPos > End)
   {
      Free();
      return (LOADPART_Message | LOADFAIL_Truncated);
   }
   
   Orders = (FAROrders *) CurrentPos;
   CurrentPos = Region + Header->HeaderSize;
   if (CurrentPos > End)
   {
      Free();
      return (LOADPART_Header | LOADFAIL_Truncated);
   }
   
   Patterns = new FARPattern *[256];
   for (x = 0; x < 256; x++)
   {
      if (!Orders->PatternLength[x])
      {
	 Patterns[x] = 0;
	 continue;
      }
      Patterns[x] = (FARPattern *) CurrentPos;
      CurrentPos += Orders->PatternLength[x];
      if (CurrentPos > End)
      {
	 Free();
	 return (LOADPART_Patterns | LOADFAIL_Truncated | x);
      }
      
   }
   
   SampleMap = (unsigned long *) CurrentPos;
   CurrentPos += 2*sizeof(unsigned long);
   if (CurrentPos > End)
   {
      Free();
      return (LOADPART_Samples | LOADFAIL_Truncated | LOADNUM_None);
   }
   
   Samples = new FARSample *[64];
   for (x = 0; x < 64; x++)
   {
      if (!(SampleMap[x/32] >> (x%32)))
	 Samples[x] = 0;
      else
      {
	 Samples[x] = (FARSample *) CurrentPos;
	 CurrentPos += sizeof (FARSample) + Samples[x]->Length;
	 if (CurrentPos > End)
	 {
	    Free();
	    return (LOADPART_Samples | LOADFAIL_Truncated | x);
	 }
	 
      }
   }
   
   return LOADOK;
}
									/*}}}*/
// FARFormat::Keep - Clone important internal data			/*{{{*/
// ---------------------------------------------------------------------
/* After this function is called the playback routine will be able to
   execute without having the memory block passed to LoadMemModule 
   resident. However, it will be unable to download the samples which
   are not cloned by the routine. The routine was intended for use with
   wavetable devices that are capable of sample downloading (GUS) */
long museFARFormat::Keep()
{
   if (Header == 0 || Private == true)
      return FAIL_NotLoaded;
   
   Private = true;
   
   // Clone the header
   FARHeader *OldH = Header;
   Header = new FARHeader;
   memcpy(Header,OldH,sizeof(FARHeader));
   
   // Clone the orders
   FAROrders *OldO = Orders;
   Orders = new FAROrders;
   memcpy(Orders,OldO,sizeof(FAROrders));

   // Clone the patterns
   int x;
   for (x = 0; x < 256; x++)
   {
      if (!Orders->PatternLength[x]) 
	 continue;

      FARPattern *OldP = Patterns[x];
      Patterns[x] = (FARPattern *) new unsigned char[Orders->PatternLength[x]];
      memcpy (Patterns[x],OldP,Orders->PatternLength[x]);
   }
   
   // Clone the sample map
   unsigned long *OldM = SampleMap;
   SampleMap = (unsigned long *)new unsigned long[2];
   memcpy(SampleMap,OldM,2*sizeof(unsigned long));

   // Clone the samples
   for (x = 0; x < 64; x++)
   {
      if ((SampleMap[x/32] >> (x%32)))
      {
	 FARSample *OldS = Samples[x];
         Samples[x] = new FARSample;
         memcpy(Samples[x],OldS,sizeof(FARSample));
      }
   }
   
   return LOADOK;
}
									/*}}}*/
// FARFormat::GetSongInfo - Returns information about the song		/*{{{*/
// ---------------------------------------------------------------------
/* Simply fills the info structure. */
long museFARFormat::GetSongInfo(museSongInfo &Info)
{
   Info.Free();
   
   if (Header == 0)
      return FAIL_NotLoaded;
   
   Info.Patterns = Orders->NoOfPatterns;
   Info.Orders = Orders->NoOfOrders;
   Info.Channels = 16;
   Info.ClassName = ((museFormatClass *)GetMetaClass())->GetTypeName();
   Info.TypeName = _strdupcpp("Farandole Module");
   Info.Title = _strndupcpp(Header->Name,sizeof(Header->Name));

   return FAIL_None;
}
									/*}}}*/
// FARFormat::GetSongSamples - Gets the samples for the song		/*{{{*/
// ---------------------------------------------------------------------
/* */
void museFARFormat::GetSongSamples(SequenceSample &Samps)
{
   if (Header == 0)
      return;

   Samps.construct();
   Samps.reserve(64);

   for (int x = 0; x < 64; x++)
   {
      museSample &Samp = Samps[x];
      Samp.Name = 0;
      Samp.Sample = 0;

      if (Samples[x] && Samples[x]->Length)
      {
         Samp.Sample = (unsigned char *)Samples[x] + sizeof(FARSample);
         if ((Samples[x]->LoopMode & 8))
         {
            Samp.LoopBegin = Samples[x]->LoopBegin;
            Samp.LoopEnd = Samples[x]->LoopEnd;
            Samp.SampleEnd = Samples[x]->Length;
            Samp.Flags = INST_Loop | INST_Signed;
         }
         else
         {
            Samp.LoopBegin = 0;
            Samp.LoopEnd = Samp.SampleEnd = Samples[x]->Length;
            Samp.Flags = INST_Signed;
         }

         Samp.Name = Samples[x]->Name;

         if (Samples[x]->Type & 1) 
	    Samp.Flags |= INST_16Bit;
      }

      Samp.InstNo = x;
      Samp.SampleName = 0;
      Samp.MaxNameLen = 32;
      Samp.MaxSNameLen = 0;
      Samp.Center = 8363;
      Samp.Volume = 0xFF;
   }
}
									/*}}}*/

// FARFormatClass:GetTypeName - Gets the generic type name		/*{{{*/
// ---------------------------------------------------------------------
/* */
const char *museFARFormatClass::GetTypeName()
{
   return "Farandole Module";
}
									/*}}}*/
// FARFormatClass::GetClassForFile - Gets the metaclass for fname	/*{{{*/
// ---------------------------------------------------------------------
/* Matches *.FAR */
museFormatClass *museFARFormatClass::GetClassForFile(const char *FName)
{
   const char *End = FName + strlen(FName);
   for(;*End != '.' && End != FName;End --);

   if (End != FName)
      End++;

   if (_stricmp(End,"FAR") != 0)
      return 0;
   return this;
}
									/*}}}*/
// FARFormatClass::GetScanSize - Returns the header size 		/*{{{*/
// ---------------------------------------------------------------------
/* */
unsigned long museFARFormatClass::GetScanSize()
{
   return sizeof(FARHeader);
}
									/*}}}*/
// FARFormatClass::Scan - Scans the file for info			/*{{{*/
// ---------------------------------------------------------------------
/* */
unsigned long museFARFormatClass::Scan(octet *Region, museSongInfo &Info)
{
   museFormatClass::Scan(Region,Info);

   FARHeader *Header = (FARHeader *)Region;

   // Get the title
   Info.Title = _strndupcpp(Header->Name,sizeof(Header->Name));
   Info.TypeName = _strdupcpp("Farandole Module");
   Info.ClassName = GetTypeName();
   Info.Channels = 16;
   Info.Patterns = 0;
   Info.Orders = 0;

   return 0;
}
									/*}}}*/
// FARFormatClass::museFARFormatClass - Constructor			/*{{{*/
// ---------------------------------------------------------------------
/* Sets the version info */
museFARFormatClass::museFARFormatClass()
{
   MajorVersion = 1;
   MinorVersion = 2;
}
									/*}}}*/
