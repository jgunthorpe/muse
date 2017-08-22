// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   XMFormat - Muse file format to handle XM files.

   The XM loader rewrites all of the sample data to de-delta encode it. 
   This is required because playing delta encoded data is simply not 
   practicle without conversion.
   
   ##################################################################### */
									/*}}}*/
// Includes								/*{{{*/
#include <muse.h>
#include <xmform.h>
#include <math.h>

#include "xmstruc.h"
   									/*}}}*/

museXMFormatClass *museXMFormat::Meta = new museXMFormatClass;

// XMFormat::museXMFormat - Constructor					/*{{{*/
// ---------------------------------------------------------------------
/* 0s data members */
museXMFormat::museXMFormat()
{
   // Init the structures
   Header = 0;
   PatternData = 0;
   InstrumentData = 0;
   SampleMappings = 0;
   NoOfSamples = 0;
   Private = false;
}
									/*}}}*/
// XMFormat::Free - Frees any memory allocated for the module		/*{{{*/
// ---------------------------------------------------------------------
/* If any private copies of the pattern data have been made they are
   freed. */
void museXMFormat::Free()
{
   int x;
   
   if (Header == 0) 
      return;
   
   if (Private)
   {
      if (PatternData != 0) 
	 for (x = 0; x < Header->NoOfPatterns; x++)
	    delete [] PatternData[x].PatternData;
   }
   
   delete [] PatternData;
   PatternData = 0;
   
   // Delete the Instrument structures.
   if (InstrumentData) 
   {
      for (x = 0; x < Header->NoOfInstruments; x++)
	 delete [] InstrumentData[x].Samples;
      delete [] InstrumentData;
      InstrumentData = 0;
   }
   
   if (SampleMappings)
   {
      for (x = 0; x < Header->NoOfInstruments; x++)
	 delete [] SampleMappings[x];
      delete [] SampleMappings;
      SampleMappings = 0;
   }
   
   delete Header;
   Header = 0;
   NoOfSamples = 0;
   Private = false;
   museFormatBase::Free();
}
									/*}}}*/
// XMFormat::LoadMemModule - Load the module from memory		/*{{{*/
// ---------------------------------------------------------------------
/* This sets up and required data pointers for the playback code. It
   also delta decodes the samples. */
long museXMFormat::LoadMemModule(unsigned char *Region, unsigned long Size)
{
   Free();
   
   unsigned char *CurrentPos = Region;
   unsigned char *End = Region + Size;
   
   if (Size <= sizeof(XMHeader))
      return LOADPART_Header | LOADFAIL_Truncated;
   
   Header = new XMHeader;
   memcpy(Header,CurrentPos,sizeof(XMHeader));
   
   // Verify Header is for a FT II XM
   CurrentPos += sizeof(XMHeader);					
   if (strncmp(Header->IDText,"Extended Module:",strlen("Extended Module:")))
   {
      Free();
      return LOADPART_Header | LOADFAIL_Sigcheck;
   }
   
   // Load patterndata
   unsigned long PCount = 0;
   unsigned int x;
   for (x = 0; x < Header->SongLength; x++)
      PCount = max(Header->OrderList[x],PCount);

   PCount++;
   PatternData = new PatternHeader[max(Header->NoOfPatterns,PCount)];
   memset(PatternData,0,sizeof(*PatternData)*max(Header->NoOfPatterns,PCount));
   for (x = 0; x < max(PCount,Header->NoOfPatterns); x++)
   {
      if (x < Header->NoOfPatterns)
      {
	 if (CurrentPos + min(((PatternHeader*)CurrentPos)->HeaderSize,sizeof(*PatternData)) > End)
	 {
	    Warn("Pattern Data Corrupt");
	    Free();
	    return (LOADPART_Patterns | LOADFAIL_Truncated | x);
	 }
	 
	 memcpy(&(PatternData[x]),CurrentPos,min(((PatternHeader*)CurrentPos)->HeaderSize,sizeof(*PatternData)));
	 CurrentPos += min(PatternData[x].HeaderSize,sizeof(*PatternData));
	 PatternData[x].PatternData = CurrentPos;
	 CurrentPos += PatternData[x].PatternDataSize;
	 
	 if (PatternData[x].PatternDataSize == 0)
	    PatternData[x].PatternData = 0;
	 
	 if (CurrentPos > End)
	 {
	    Warn("Pattern Data Corrupt");
	    Free();
	    return (LOADPART_Patterns | LOADFAIL_Truncated | x);
	 }	 
      }
      else 
	 PatternData[x].NoOfRows = 64;

      // check that the patterndata is not corrupt.            
      if (PatternData[x].PatternData)
      {
	 unsigned char *DataPointer = PatternData[x].PatternData;
	 unsigned char *DataEnd = DataPointer + PatternData[x].PatternDataSize;
	 int Broke = 0;
	 for (unsigned int y = 0; y < PatternData[x].NoOfRows; y++)
	 {
	    for (unsigned int z = 0; z < Header->NoOfChannels; z++)
	    {
	       Broke = 1;
	       if (DataPointer >= DataEnd)
		  break;
	     
	       // Quick skip the whole lot.
	       if (!(*DataPointer & 128))
		  DataPointer += 5;
	       else 
	       {
		  if (DataPointer >= DataEnd)
		     break;
		  
		  // Skip a byte for each bit that is set in the lower nibble
		  unsigned char z = *DataPointer++;
		  unsigned char v = 1;
		  while (v <= 16)
		  {
		     if (z & v)
			DataPointer++;
		     v *= 2;
		  }
		  
		  if (DataPointer > DataEnd)
		     break;
	       }
	       Broke = 0;
	    }
	    
	    // If we got here then its corrupted.
	    if (Broke == 1)
	    {
	       Warn("Pattern Data Internally Corrupt");
	       Free();
	       return (LOADPART_Patterns | LOADFAIL_Corrupt | x);
	    }	    
	 }
      }
   }
   
   // Load insts
   InstrumentData = new InstrumentHeader[Header->NoOfInstruments];
   memset(InstrumentData,0,sizeof(InstrumentHeader)*Header->NoOfInstruments);
   NoOfSamples = 0;
   for (x = 0; x < Header->NoOfInstruments; x++)
   {
      if (CurrentPos + sizeof(InstrumentHeader) > End)
	 continue;
      
      memcpy(&InstrumentData[x],CurrentPos,sizeof(InstrumentHeader));
      CurrentPos += InstrumentData[x].Size;
      InstrumentData[x].Samples = 0;
      
      if (CurrentPos > End)
      {
	 memset(&InstrumentData[x],0,sizeof(InstrumentHeader));
	 continue;
      }
      
      InstrumentData[x].Samples = new SampleHeader[InstrumentData[x].NoOfSamples];
      memset(InstrumentData[x].Samples,0,sizeof(*InstrumentData[x].Samples)*InstrumentData[x].NoOfSamples);
      
      // Copy the subsample data.
      int y;
      for(y = 0; y < InstrumentData[x].NoOfSamples; y++)
      {
	 // Cant use sizeof sampleheader due to the extra long
	 if (CurrentPos + 40 > End)
	 {
	    WarnC();
	    DebugMessage("Sample Data Corrupt 1 %u,%u\n",CurrentPos,Region+Size);
	    Free();
	    return (LOADPART_Samples | LOADFAIL_Truncated | y*16+x);
	 }
	 memcpy(&InstrumentData[x].Samples[y],CurrentPos,40);
	 CurrentPos +=  40;
      }
      
      // Do the sample data
      for (y = 0; y < InstrumentData[x].NoOfSamples; y++)
      {
	 InstrumentData[x].Samples[y].SampleData = CurrentPos;
	 
	 // Corrupted
	 if (CurrentPos >= End)
	 {
	    InstrumentData[x].Samples[y].Length = 0;
	    InstrumentData[x].Samples[y].LoopStart = 0;
	    InstrumentData[x].Samples[y].LoopEnd = 0;
	    InstrumentData[x].Samples[y].Volume = 0;
	    InstrumentData[x].Samples[y].SampleData = 0;
	    continue;
	 }
	 else
	    if (CurrentPos + InstrumentData[x].Samples[y].Length >= End)
	    {
	       InstrumentData[x].Samples[y].Length = End - CurrentPos;
	       if (InstrumentData[x].Samples[y].Length == 0)
	       {
		  InstrumentData[x].Samples[y].LoopStart = 0;
		  InstrumentData[x].Samples[y].LoopEnd = 0;
		  InstrumentData[x].Samples[y].Volume = 0;
		  InstrumentData[x].Samples[y].SampleData = 0;
		  continue;
	       }
	    }
	 
	 // 16 bit delta decode
	 if ((InstrumentData[x].Samples[y].Flags & 16))
	 {
	    signed short OldVal = 0;
	    for (unsigned int z = 0; z < InstrumentData[x].Samples[y].Length / 2; z++)
	    {
	       OldVal += ((signed short *) InstrumentData[x].Samples[y].SampleData)[z];
	       ((signed short *) InstrumentData[x].Samples[y].SampleData)[z] = OldVal;
	    }
	 }
	 else
	 {
	    char OldVal = 0;
	    for (unsigned int z = 0; z < InstrumentData[x].Samples[y].Length; z++)
	    {
	       OldVal += InstrumentData[x].Samples[y].SampleData[z];
	       InstrumentData[x].Samples[y].SampleData[z] = OldVal ^ 128;
	    }
	 }
	 
	 CurrentPos += InstrumentData[x].Samples[y].Length;
      }
      
      if (InstrumentData[x].NoOfSamples != 0) 
	 NoOfSamples += InstrumentData[x].NoOfSamples;
      else 
	 NoOfSamples++;
   }
   
   return LOADOK;
}
   									/*}}}*/
// XMFormat::GetSongInfo - Gets information about the song		/*{{{*/
// ---------------------------------------------------------------------
/* Reads the relevant information from the XM header. */
long museXMFormat::GetSongInfo(museSongInfo &Info)
{
   Info.Free();
   if (Header == 0)
      return FAIL_NotLoaded;
   
   Info.Patterns = Header->NoOfPatterns;
   Info.Orders = Header->SongLength;
   Info.Channels = Header->NoOfChannels;

   Info.ClassName = ((museFormatClass *)GetMetaClass())->GetTypeName();
   Info.Title = _strndupcpp(Header->Name,sizeof(Header->Name));   
   
   // Get the type name + version
   char S[50];
   sprintf(S,"Extended Module V%2.2f",Header->Version/100.0);
   Info.TypeName = _strdupcpp(S);

   return FAIL_None;
} 
   									/*}}}*/
// XMFormat::GetSongSamples - Get the sample information.		/*{{{*/
// ---------------------------------------------------------------------
/* Decode the XM samples, expanding them from the multilevel format that
   XM's use to Muse's flat format with a inst number. */
void museXMFormat::GetSongSamples(SequenceSample &Samples)
{
   if (Header == 0) return;
   
   Samples.construct();
   
   int I = 0;
   unsigned int x, y, z;

   if (SampleMappings != 0)
   {
      for (x = 0; x < Header->NoOfInstruments; x++)
	 delete [] SampleMappings[x];
      delete [] SampleMappings;
   }
   
   SampleMappings = new unsigned short *[Header->NoOfInstruments];
   memset(SampleMappings,0,sizeof(*SampleMappings)*Header->NoOfInstruments);
   Samples.reserve (NoOfSamples);

   for (x = 0; x < Header->NoOfInstruments; x++)
   {
      Samples[I].Name = InstrumentData[x].Name;
      Samples[I].MaxNameLen = 22;
      Samples[I].SampleName = 0;
      Samples[I].MaxSNameLen = 22;
      Samples[I].InstNo = x;
      Samples[I].Sample = 0;

      if (InstrumentData[x].NoOfSamples) 
	 SampleMappings[x] = new unsigned short[InstrumentData[x].NoOfSamples];
      else 
	 I++;
      
      for (y = 0; y < InstrumentData[x].NoOfSamples; y++)
      {
         for (z = 0; z < 96; z++) if (InstrumentData[x].NoteMap[z] >= InstrumentData[x].NoOfSamples) InstrumentData[x].NoteMap[z] = InstrumentData[x].NoOfSamples - 1;
	 
         Samples[I].SampleEnd = InstrumentData[x].Samples[y].Length;

         if ((InstrumentData[x].Samples[y].Length) == 0 && InstrumentData[x].Samples[y].Name[0] == 0 && y)
	    continue;
         
         if ((InstrumentData[x].Samples[y].Length) == 0)
            Samples[I].Sample = 0;
         else
            Samples[I].Sample = InstrumentData[x].Samples[y].SampleData;

         if ((InstrumentData[x].Samples[y].Flags & 3) == 1)
         {
	    Samples[I].LoopBegin = InstrumentData[x].Samples[y].LoopStart;
	    Samples[I].LoopEnd = InstrumentData[x].Samples[y].LoopEnd + InstrumentData[x].Samples[y].LoopStart;
	    if (Samples[I].LoopEnd > Samples[I].SampleEnd)
	       Samples[I].LoopEnd = Samples[I].SampleEnd;
	    Samples[I].Flags = INST_Loop;
         }
         else
         {
	    if ((InstrumentData[x].Samples[y].Flags & 3) == 2)
	    {
	       Samples[I].LoopBegin = InstrumentData[x].Samples[y].LoopStart;
	       Samples[I].LoopEnd = InstrumentData[x].Samples[y].LoopEnd + InstrumentData[x].Samples[y].LoopStart;
	       if (Samples[I].LoopEnd > Samples[I].SampleEnd)
		  Samples[I].LoopEnd = Samples[I].SampleEnd;
	       Samples[I].Flags = INST_PingPong;
	    }
	    else
	    {
	       Samples[I].LoopBegin = 0;
	       Samples[I].LoopEnd = InstrumentData[x].Samples[y].Length;
	       Samples[I].Flags = 0;
	    }
         }
         
         Samples[I].Name = InstrumentData[x].Name;
         Samples[I].MaxNameLen = 22;
         Samples[I].MaxSNameLen = 22;
         Samples[I].InstNo = x;
         Samples[I].SampleName = InstrumentData[x].Samples[y].Name;
         Samples[I].FineTune = 0;
         Samples[I].Center = (int)((float) 8363 * pow (2, (float) InstrumentData[x].Samples[y].NoteAdjust / 12.0 + (float) InstrumentData[x].Samples[y].FineTune / (12.0 * 128.0)));
         Samples[I].Volume = InstrumentData[x].Samples[y].Volume*0xFF/64;
         Samples[I].FineTune = 0;

         if (InstrumentData[x].Samples[y].Flags & 16)
            Samples[I].Flags |= INST_16Bit;

         SampleMappings[x][y] = I++;
      }
   }
}
									/*}}}*/
// XMFormat::Keep - Duplicates the patterndata				/*{{{*/
// ---------------------------------------------------------------------
/* This is called to duplicate the patterndata in preperation for the
   freeing of the memory block. */
long museXMFormat::Keep()
{
   if (Header == 0)
      return FAIL_NotLoaded;
   
   // All that is cloned is the pattern data.
   Private = true;
   for (unsigned int I = 0; I != Header->NoOfPatterns;I++)
   {
      unsigned char *Old = PatternData[I].PatternData;
      PatternData[I].PatternData = new unsigned char[PatternData[I].PatternDataSize];
      memcpy(PatternData[I].PatternData,Old,PatternData[I].PatternDataSize);
   }

   return LOADOK;
}
   									/*}}}*/

// XMFormatClass::GetTypeName - Returns the type name for this format	/*{{{*/
// ---------------------------------------------------------------------
/* */
const char *museXMFormatClass::GetTypeName()
{
   return "Extended Module";
}
									/*}}}*/
// XMFormatClass::GetClassForFile - Find the format for the filename	/*{{{*/
// ---------------------------------------------------------------------
/* Checks the extension for .xm */
museFormatClass *museXMFormatClass::GetClassForFile(const char *FName)
{
   const char *End = FName + strlen(FName);
   for(;*End != '.' && End != FName;End --);

   if (End != FName)
      End++;

   if (_stricmp(End,"XM") != 0)
      return 0;
   return this;
}
									/*}}}*/
// XMFormatClass::GetScanSize - Return the header size			/*{{{*/
// ---------------------------------------------------------------------
/* Just returns the XM header size. */
unsigned long museXMFormatClass::GetScanSize()
{
   return sizeof(XMHeader);
}
									/*}}}*/
// XMFormatClass:Scan - Scan the header for info about the song		/*{{{*/
// ---------------------------------------------------------------------
/* Read all of the info from the XM header */
unsigned long museXMFormatClass::Scan(octet *Region, museSongInfo &Info)
{
   XMHeader &Header = *(XMHeader *)Region;

   // Verify Header is for a FT II XM
   if (strncmp(Header.IDText,"Extended Module:",strlen("Extended Module:")))
      return LOADPART_Header | LOADFAIL_Sigcheck;

   Info.TypeName = _strdupcpp("Extended Module");
   Info.ClassName = GetTypeName();
   Info.Channels = Header.NoOfChannels;
   Info.Patterns = Header.NoOfPatterns;
   Info.Orders = Header.SongLength;

   Info.Title = _strndupcpp(Header.Name,sizeof(Header.Name));

   return LOADOK;
}
   									/*}}}*/
// XMFormatClass::museXMFormatClass - Sets the version information	/*{{{*/
// ---------------------------------------------------------------------
/* */
museXMFormatClass::museXMFormatClass()
{
   MajorVersion = 1;
   MinorVersion = 6;
}
									/*}}}*/
