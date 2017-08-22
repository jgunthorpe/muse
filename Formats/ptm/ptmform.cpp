// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   PTMFormat - Muse file format to handle PTM files.
   
   ##################################################################### */
									/*}}}*/
// Includes								/*{{{*/
#include <muse.h>
#include <ptmform.h>

#include "ptmstruc.h"
   									/*}}}*/

musePTMFormatClass *musePTMFormat::Meta = new musePTMFormatClass;

// PTMFormat::musePTMFormat - Constructor				/*{{{*/
// ---------------------------------------------------------------------
/* 0s data members */
musePTMFormat::musePTMFormat()
{
   // Init the structures
   Header = 0;
   Instruments = 0;
   Patterns = 0;
   FileLoc = 0;
   Private = false;

}
									/*}}}*/
// PTMFormat::Free - Frees the module					/*{{{*/
// ---------------------------------------------------------------------
/* Dumps cloned copies too */
void musePTMFormat::Free()
{
   int x;
   
   if (Header == 0)
      return;
   
   if (Private)
   {
      for (x = 0; x < Header->NoOfInstruments; x++)
	 delete Instruments[x];
      
      for (x = 0; x < Header->NoOfPatterns; x++)
	 delete [] Patterns[x];
      
      delete Header;
   }
   
   Header = 0;
   delete Instruments;
   Instruments = 0;
   delete Patterns;
   Patterns = 0;
   FileLoc = 0;
   Private = false;
}
									/*}}}*/
// PTMFormat::LoadMemModule - Load a song from a memory image		/*{{{*/
// ---------------------------------------------------------------------
/* PTM's are pretty simple.. see ../info/ptm* */
long musePTMFormat::LoadMemModule(unsigned char *Region, unsigned long Size)
{
   unsigned int x, y, z;
   Free();
   
   unsigned char *CurrentPos = Region;
   unsigned char *End = Region + Size;
   
   FileLoc = Region;
   Header = (PTMHeader *) CurrentPos;
   CurrentPos += sizeof (PTMHeader);
   if (CurrentPos > End)
   {
      Free();
      return (LOADPART_Header | LOADFAIL_Truncated);
   };
   
   if (memcmp(Header->Sig, "PTMF", 4))
   {
      // Clean up if an error occured.
      Free();
      return (LOADPART_Header | LOADFAIL_Sigcheck);
   };
   
   Instruments = new PTMInstrument *[Header->NoOfInstruments];
   Patterns = new unsigned char *[Header->NoOfPatterns];
   
   // Load the instrument data
   for (x = 0; x < Header->NoOfInstruments; x++)
   {
      Instruments[x] = (PTMInstrument *) CurrentPos;
      CurrentPos += sizeof (PTMInstrument);
      if (CurrentPos > End)
      {
	 // Clean up if an error occured.
	 Free();
	 return (LOADPART_Instruments | LOADFAIL_Truncated | x);
      };
      
      if (Instruments[x]->SampleOffset + Instruments[x]->Length > Size)
      {
	 // Clean up if an error occured.
	 Free();
	 return (LOADPART_Samples | LOADFAIL_Truncated | x);
      };
      
      if ((Instruments[x]->Flags & 16))
      {
	 char OldVal = 0;
	 for (z = 0; z < Instruments[x]->Length; z++)
	 {
	    OldVal += (Region + Instruments[x]->SampleOffset)[z];
	    (Region + Instruments[x]->SampleOffset)[z] = OldVal;
	 }
      }
      else
      {
	 char OldVal = 0;
	 for (z = 0; z < Instruments[x]->Length; z++)
	 {
	    OldVal += (Region + Instruments[x]->SampleOffset)[z];
	    (Region + Instruments[x]->SampleOffset)[z] = OldVal ^ 128;
	 }
      }
   }
   
   // Integrity check of the pattern data
   for (x = 0; x < Header->NoOfPatterns; x++)
   {
      Patterns[x] = Region + Header->Patterns[x] * 16;
      if (Patterns[x] > End)
      {
	 // Clean up if an error occured.
	 Free();
	 return (LOADPART_Patterns | LOADFAIL_Truncated | x);
      };
      
      unsigned char *DataPointer;
      DataPointer = Patterns[x];
      for (y = 0; y < 64; y++)
      {
	 if (DataPointer >= End)
	 {
	    Free();
	    return (LOADPART_Patterns | LOADFAIL_Corrupt | x);
	 }
	 
	 while (*DataPointer)
	 {
	    z = *DataPointer++;
	    
	    if (z & PATTERN_Note)
	       DataPointer += 2;
	    if (z & PATTERN_Effect)
	       DataPointer += 2;
	    
	    if (z & PATTERN_Volume)
	       DataPointer++;
	    
	    if (DataPointer > End)
	    {
	       Free();
	       return (LOADPART_Patterns | LOADFAIL_Corrupt | x);
	    }
	 }
	 DataPointer++;
      }
   }
   
   return LOADOK;
}
									/*}}}*/
// PTMFormat::GetSongInfo - Returns information about the song		/*{{{*/
// ---------------------------------------------------------------------
/* Simply fills the info structure. */
long musePTMFormat::GetSongInfo(museSongInfo &Info)
{
   Info.Free();
   
   if (Header == 0)
      return FAIL_NotLoaded;
   
   float Ver = (Header->Version & 0xF)*0.01 + ((Header->Version & 0xF0) >> 4)*0.1 + ((Header->Version & 0xF00) >> 8);
   
   char S[300];
   sprintf(S,"PolyTracker Module V%1.2f",Ver);
   Info.TypeName = _strdupcpp(S);
   
   Info.ClassName = GetTypeName();
   Info.Channels = Header->NoOfChannels;
   Info.Patterns = Header->NoOfPatterns;
   Info.Orders = Header->NoOfOrders;
   Info.Title = _strndupcpp(Header->Name,sizeof(Header->Name));

   return FAIL_None;
}
									/*}}}*/
// PTMFormat::Keep - Clone important internal data			/*{{{*/
// ---------------------------------------------------------------------
/* After this function is called the playback routine will be able to
   execute without having the memory block passed to LoadMemModule 
   resident. However, it will be unable to download the samples which
   are not cloned by the routine. The routine was intended for use with
   wavetable devices that are capable of sample downloading (GUS) */
long musePTMFormat::Keep()
{
   if (Header == 0 || Private == true)
      return FAIL_NotLoaded;
   
   Private = true;
   
   // Clone the header
   PTMHeader *OldHeader = Header;
   Header = new PTMHeader;
   memcpy(Header,OldHeader,sizeof(*Header));
   
   // Clone the patterndata
   int I;
   for (I = 0; I != Header->NoOfPatterns; I++)
   {
      unsigned char *OldPat = Patterns[I];
      
      // Strange, there is no size byte in PTMs?
      unsigned char *DataPointer;
      DataPointer = Patterns[I];
      for (int y = 0; y < 64; y++)
      {
	 while (*DataPointer)
	 {
	    unsigned char z = *DataPointer++;
	    
	    if (z & PATTERN_Note)
	       DataPointer += 2;
	    if (z & PATTERN_Effect)
	       DataPointer += 2;
	    
	    if (z & PATTERN_Volume)
	       DataPointer++;
	 }
	 DataPointer++;
      }
      
      Patterns[I] = new unsigned char[DataPointer - OldPat];
      memcpy(Patterns[I],OldPat, DataPointer - OldPat);
   }   
      
   // Clone the instrument data
   for (I = 0; I != Header->NoOfInstruments; I++)
   {
      PTMInstrument *OldInst = Instruments[I];
      Instruments[I] = new PTMInstrument;
      memcpy(Instruments[I],OldInst,sizeof(PTMInstrument));
   }
	
   return LOADOK;
}
									/*}}}*/

// PTMFormat::GetSongSamples - Return the songs sample data		/*{{{*/
// ---------------------------------------------------------------------
/* This just converts/checks the ptm instrument structure */
void musePTMFormat::GetSongSamples(SequenceSample &Samples)
{
   if (Header == 0)
      return;

   Samples.construct();
   Samples.reserve(Header->NoOfInstruments);
   
   for (int x = 0; x < Header->NoOfInstruments; x++) 
   {
      museSample &Samp = Samples[x];
      if ((Instruments[x]->Flags & 3) == 0) 
      {
         Samp.Sample = 0;
         Samp.InstNo = x;
      } 
      else 
      {
         Samp.Sample = FileLoc + Instruments[x]->SampleOffset;
         if ((Instruments[x]->Flags & 12) == 12) 
	 {
            Samp.LoopBegin = Instruments[x]->LoopBegin;
            Samp.LoopEnd = Instruments[x]->LoopEnd - 1;
            Samp.SampleEnd = Instruments[x]->Length;
            if (Samp.LoopEnd > Samp.SampleEnd)
               Samp.LoopEnd = Samp.SampleEnd;
            Samp.Flags = INST_PingPong;
	 } 
	 else 
	    if ((Instruments[x]->Flags & 12) == 4) 
	    {
	       Samp.LoopBegin = Instruments[x]->LoopBegin;
	       Samp.LoopEnd = Instruments[x]->LoopEnd - 1;
	       Samp.SampleEnd = Instruments[x]->Length;
	       if (Samp.LoopEnd > Samp.SampleEnd)
		  Samp.LoopEnd = Samp.SampleEnd;
	       Samp.Flags = INST_Loop;
            } 
	    else 
   	    {
	       Samp.LoopBegin = 0;
	       Samp.LoopEnd = Samp.SampleEnd = Instruments[x]->Length;
	       Samp.Flags = 0;
            }
	 
         Samp.Name = Instruments[x]->Name;
         Samp.MaxNameLen = 28;
         Samp.SampleName = Instruments[x]->FileName;
         Samp.MaxSNameLen = 12;
         Samp.InstNo = x;
         Samp.Center = Instruments[x]->BaseFrequency;
         Samp.Volume = Instruments[x]->Volume * 255 / 64;
         Samp.FineTune = 0;
	 
         if (Instruments[x]->Flags & 16) 
	    Samp.Flags |= INST_16Bit;
      }
   }
}
									/*}}}*/
// PTMFormatClass::GetTypeName - Returns the type of this module	/*{{{*/
// ---------------------------------------------------------------------
/* */
const char *musePTMFormatClass::GetTypeName()
{
   return "PolyTracker Module";
}
									/*}}}*/
// PTMFormatClass::GetClassForFile - Returns the metaclass for fname	/*{{{*/
// ---------------------------------------------------------------------
/* Matches *.ptm */
museFormatClass *musePTMFormatClass::GetClassForFile(const char *FName)
{
   const char *End = FName + strlen(FName);
   for(;*End != '.' && End != FName;End --);

   if (End != FName)
      End++;

   if (_stricmp(End,"PTM") != 0)
      return 0;
   return this;
}
									/*}}}*/
// PTMFormatClass::GetScanSize - Returns the size of the header		/*{{{*/
// ---------------------------------------------------------------------
/* */
unsigned long musePTMFormatClass::GetScanSize()
{
    return sizeof(MinPTMHeader);
}
									/*}}}*/
// PTMFormatClass::Scan - Scans a files header				/*{{{*/
// ---------------------------------------------------------------------
/* */
unsigned long musePTMFormatClass::Scan(octet *Region, museSongInfo &Info)
{
   museFormatClass::Scan(Region,Info);
   
   MinPTMHeader *Header = (MinPTMHeader *)Region;
   
   // Verify Header is for a PTM
   if (memcmp (Header->Sig, "PTMF", 4))
      return 1;
   
   float Ver = (Header->Version & 0xF)*0.01 + ((Header->Version & 0xF0) >> 4)*0.1 + ((Header->Version & 0xF00) >> 8);
   
   char S[300];
   sprintf(S,"PolyTracker Module V%1.2f",Ver);
   Info.TypeName = _strdupcpp(S);
   
   Info.ClassName = GetTypeName();
   Info.Channels = Header->NoOfChannels;
   Info.Patterns = Header->NoOfPatterns;
   Info.Orders = Header->NoOfOrders;
   Info.Title = _strndupcpp(Header->Name,sizeof(Header->Name));
   
   return 0;
}
									/*}}}*/
// PTMFormatClass::musePTMFormatClass - Constructor			/*{{{*/
// ---------------------------------------------------------------------
/* Sets the version numbers */
musePTMFormatClass::musePTMFormatClass()
{
   MajorVersion = 1;
   MinorVersion = 1;
}
									/*}}}*/
