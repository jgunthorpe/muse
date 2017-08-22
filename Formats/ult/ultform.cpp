// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   ULTFormat - Muse file format to handle ULT files.
   
   ##################################################################### */
									/*}}}*/
// Includes									/*{{{*/
#include <muse.h>
#include <ultform.h>
#include <math.h>

#include "ultstruc.h"
   									/*}}}*/

museULTFormatClass *museULTFormat::Meta = new museULTFormatClass;

// ULTFormat::museULTFormat - Constructor				/*{{{*/
// ---------------------------------------------------------------------
/* 0s data members */
museULTFormat::museULTFormat()
{
   // Init the structures

   Header = 0;
   HeaderData = 0;
   Samples = 0;
   PatHeader = 0;
   Patterns = 0;
   UltType = 0;
   Comment = 0;
   SampleData = 0;
   Private = false;
   
}
									/*}}}*/
// ULTFormat::Free - Frees the song					/*{{{*/
// ---------------------------------------------------------------------
/* Frees the cloned data and the song */
void museULTFormat::Free()
{
   int x;
    
   if (Header == 0)
      return;
   
   if (Private)
   {
      if (Patterns)
      {
	 for (x = 0; x < (PatHeader->NoOfPatterns + 1) * (PatHeader->NoOfChannels + 1); x++)
	    delete [] Patterns[x];
	 delete Patterns;
      }
      
      delete [] PatHeader;
      delete [] Samples;
      delete HeaderData;
      delete [] Comment;
      delete Header;
   }
   
   Header = 0;
   HeaderData = 0;
   Samples = 0;
   PatHeader = 0;
   Patterns = 0;
   UltType = 0;
   Comment = 0;
   SampleData = 0;
   Private = false;
   
   museFormatBase::Free();
}
									/*}}}*/
// ULTFormat::LoadMemModule - Loads the song from a memory image	/*{{{*/
// ---------------------------------------------------------------------
/* */
long museULTFormat::LoadMemModule(unsigned char *Region, unsigned long Size)
{
   Free();
   
   unsigned char *CurrentPos = Region;
   unsigned char *End = Region + Size;
   
   if (CurrentPos + sizeof (ULTHeader) > End)
   {
      Free();
      return (LOADPART_Header | LOADFAIL_Truncated);
   }
   
   Header = (ULTHeader *) CurrentPos;
   
   if (!memcmp (Header->IDText, "MAS_UTrack_V001", 15))
      UltType = 1;
   else 
      if (!memcmp (Header->IDText, "MAS_UTrack_V002", 15))
         UltType = 2;
      else
         if (!memcmp (Header->IDText, "MAS_UTrack_V003", 15))
            UltType = 3;
         else 
            if (!memcmp (Header->IDText, "MAS_UTrack_V004", 15))
               UltType = 4;
   
   if (UltType == 0)
   {
      Free();
      return (LOADPART_Header | LOADFAIL_Sigcheck);
   }
   
   CurrentPos += sizeof (ULTHeader);
   if (Header->CommentLength) 
   {
      Comment = (char *)CurrentPos;
      CurrentPos += Header->CommentLength * 32;
   }
   
   if (CurrentPos > End)
   {
      Free();
      return (LOADPART_Message | LOADFAIL_Truncated);
   }
   HeaderData = (ULTData *) CurrentPos;
   CurrentPos += sizeof (ULTData);
   
   if (CurrentPos > End)
   {
      Free();
      return (LOADPART_Header | LOADFAIL_Truncated | 1);
   }
   
   Samples = (ULTSample *) CurrentPos;
   CurrentPos += HeaderData->NoOfSamples * (sizeof (ULTSample) - (UltType < 4) * 2);
   
   if (CurrentPos > End)
   {
      Free();
      return (LOADPART_Samples | LOADFAIL_Truncated);
   }
   
   PatHeader = (ULTPatternHeader *) CurrentPos;
   CurrentPos += sizeof (ULTPatternHeader);
   
   if (UltType >= 3)
      CurrentPos += PatHeader->NoOfChannels;
   else
      CurrentPos--;
   
   if (CurrentPos > End)
   {
      Free();
      return (LOADPART_Header | LOADFAIL_Truncated | 2);
   }
   
   Patterns = new unsigned char *[(PatHeader->NoOfPatterns + 1) * (PatHeader->NoOfChannels + 1)];
   int x;
   for (x = 0; x < (PatHeader->NoOfPatterns + 1) * (PatHeader->NoOfChannels + 1); x++)
   {
      Patterns[x] = CurrentPos;
      int y = 0;
      do
      {
	 if (CurrentPos >= End)
	 {
	    Free();
	    return (LOADPART_Patterns | LOADFAIL_Corrupt | x);
	 }
	 
	 if (*CurrentPos != 0xFC)
	 {
	    CurrentPos += 5;
	    y++;
	 }
	 else
	 {
	    y += CurrentPos[1];
	    CurrentPos += 7;
	 }
	 
      } 
      while (y < 64);
      
      if (y != 64 || CurrentPos > End)
      {
	 Free();
	 return (LOADPART_Patterns | LOADFAIL_Corrupt | x);
      }
   }
   
   SampleData = new unsigned char *[HeaderData->NoOfSamples];
   
   ULTSample *CurrentSample = Samples;
   for (x = 0; x < HeaderData->NoOfSamples; x++)
   {
      SampleData[x] = CurrentPos;
      CurrentPos += CurrentSample->SizeEnd - CurrentSample->SizeStart;
      
      if (CurrentSample->Flags & FLAG_16Bit)
	 CurrentPos += CurrentSample->SizeEnd - CurrentSample->SizeStart;
      
      if (CurrentPos > End)
      {
	 Free();
	 return (LOADPART_Samples | LOADFAIL_Truncated | x);
      }
      
      CurrentSample++;
      if (UltType < 4)
	 CurrentSample = (ULTSample *) (((char *) CurrentSample) - 2);
   }
   
   return LOADOK;
}
									/*}}}*/
// ULTFormat::GetSongInfo - Returns information about the song		/*{{{*/
// ---------------------------------------------------------------------
/* Simply fills the info structure. */
long museULTFormat::GetSongInfo(museSongInfo &Info)
{
   Info.Free();
   
   if (Header == 0)
      return FAIL_NotLoaded;
   
   Info.Patterns = PatHeader->NoOfPatterns;
   Info.Channels = PatHeader->NoOfChannels;
   Info.ClassName = ((museFormatClass *)GetMetaClass())->GetTypeName();

   // Get the number of orders.
   Info.Orders = 0;
   while (PatHeader->Orders[Info.Orders] != 0xFF && Info.Orders < 256) 
      Info.Orders++;

   // Get the title
   Info.Title = new char[32];
   unsigned int I;
   for (I = 0; I != 32; I++)
      Info.Title[I] = Header->Name[I];
   Info.Title[I] = 0;


   // Get the title
   Info.Title = new char[34];
   for (I = 0; I != 32; I++)
      Info.Title[I] = Header->Name[I];
   Info.Title[32] = 0;
   char *I2;
   for (I2 = &Info.Title[31]; *I2 == ' ' && I2 != Info.Title; I2--);
   if (I2 == Info.Title)
      *I2 = 0;
   else
      I2[1] = 0;

   // Get the type name
   char *C = "UltraTracker Module";
   if (UltType == 1)
      C = "UltraTracker V1.0 Module";

   if (UltType == 2)
      C = "UltraTracker V1.4 Module";

   if (UltType == 3)
      C = "UltraTracker V1.5 Module";

   if (UltType == 4)
      C = "UltraTracker V1.6 Module";

   Info.TypeName = _strdupcpp(C);

   // Get the songs comment
   if (Comment == 0)
      return FAIL_None;
   
   char *CommentData = Comment;
   char *End = CommentData + Header->CommentLength*32;

   // See if the comment is empty
   for (C = CommentData; C != End && (*C == ' ' || *C == 0);C++);
   if (C == End)
      return FAIL_None;

   // Copy the comment adding \ns every 32 chars. 
   Info.ModComment = new char[End - CommentData + Header->CommentLength];
   char *Comm = Info.ModComment;
   for (C = CommentData; C != End;)
   {
      char *Start = C;
      for (;*C != 0 && C != End && (C - Start != 32); C++,Comm++)
         *Comm = *C;
      *Comm = '\n';
      Comm++;
      for (;*C == 0 && C != End; C++);
   }
   *Comm = 0;
   
   return FAIL_None;
}
									/*}}}*/
// ULTFormat::GetSongSamples - Get the songs sampledata			/*{{{*/
// ---------------------------------------------------------------------
/* Converts the ult sample format */
void museULTFormat::GetSongSamples(SequenceSample &Samps)
{
   Samps.construct();
   Samps.reserve(HeaderData->NoOfSamples);

   if (Samples == 0)
      return;

   int I = 0;
   
   ULTSample *CurSampleData = Samples;
   for (int x = 0; x < HeaderData->NoOfSamples; x++, I++)
   {
      museSample &Samp = Samps[I];
      
      Samp.SampleEnd = CurSampleData->SizeEnd - CurSampleData->SizeStart;
      Samp.Sample = SampleData[x];

      if ((CurSampleData->Flags & (FLAG_Loop | FLAG_Bidir)) == (FLAG_Loop | FLAG_Bidir))
      {
         Samp.LoopBegin = CurSampleData->LoopStart;
         Samp.LoopEnd = CurSampleData->LoopEnd;
         if (Samp.LoopEnd > Samp.SampleEnd)
            Samp.LoopEnd = Samp.SampleEnd;
         Samp.Flags = INST_PingPong | INST_Signed;   // Enable looping
      }
      else 
	 if ((CurSampleData->Flags & FLAG_Loop))
         {
	    Samp.LoopBegin = CurSampleData->LoopStart;
	    Samp.LoopEnd = CurSampleData->LoopEnd;
	    if (Samp.LoopEnd > Samp.SampleEnd)
	       Samp.LoopEnd = Samp.SampleEnd;
	    Samp.Flags = INST_Loop | INST_Signed;   // Enable looping
	 }
         else
         {
	    Samp.LoopBegin = 0;
	    Samp.LoopEnd = CurSampleData->SizeEnd - CurSampleData->SizeStart;
	    Samp.Flags = INST_Signed;
	 }
      
      Samp.Name = CurSampleData->Name;
      Samp.MaxNameLen = 32;
      Samp.MaxSNameLen = 12;
      Samp.InstNo = x;
      Samp.SampleName = CurSampleData->SampleName;

      if (CurSampleData->Flags & FLAG_16Bit)
      {
         Samp.Flags |= INST_16Bit;
         Samp.Flags &= ~INST_Signed;
         Samp.LoopBegin *= 2;
         Samp.LoopEnd *= 2;
         Samp.SampleEnd *= 2;
      }

      Samp.Volume = CurSampleData->Volume;
      if (UltType >= 4)
         Samp.Center = (int)((float) CurSampleData->FineTune + pow (2, (float) CurSampleData->BaseFrequency / (32768.0 * 12.0)));
      else
      {
         Samp.Center = (int)((float) 8363 + pow (2, (float) CurSampleData->FineTune / (32768.0 * 12.0)));
         CurSampleData = (ULTSample *) (((char *) CurSampleData) - 2);
      }

      CurSampleData++;
   }
}
									/*}}}*/
// ULTFormat::Keep - Clone important internal data			/*{{{*/
// ---------------------------------------------------------------------
/* After this function is called the playback routine will be able to
   execute without having the memory block passed to LoadMemModule 
   resident. However, it will be unable to download the samples which
   are not cloned by the routine. The routine was intended for use with
   wavetable devices that are capable of sample downloading (GUS) */
long museULTFormat::Keep()
{
   if (Header == 0 || Private == true)
      return FAIL_NotLoaded;
   
   Private = true;

   // Clone the header
   ULTHeader *OldH = Header;
   Header = new ULTHeader;
   memcpy(Header,OldH,sizeof(ULTHeader));

   // Clone the comment data
   if (Header->CommentLength != 0)
   {
      char *OldC = Comment;
      Comment = new char[Header->CommentLength * 32];
      memcpy(Comment,OldC,Header->CommentLength * 32);
   }

   // Clone the header data
   ULTData *OldD = HeaderData;
   HeaderData = new ULTData;
   memcpy(HeaderData,OldD,sizeof(ULTData));

   // Clone the sample data
   unsigned int SamplesLen = HeaderData->NoOfSamples*(sizeof(ULTSample) - (UltType < 4)*2);
   ULTSample *OldS = Samples;
   Samples = new ULTSample[HeaderData->NoOfSamples];
   memcpy(Samples,OldS,SamplesLen);

   // Clone the pattern headers
   unsigned long PatHeaderSize = sizeof(ULTPatternHeader) - 1 + 
                                 (UltType == 3)*PatHeader->NoOfChannels;
   ULTPatternHeader *OldPH = PatHeader;
   PatHeader = (ULTPatternHeader *)new unsigned char[PatHeaderSize];
   memcpy(PatHeader,OldPH,PatHeaderSize);
   
   // Clone the patterns
   int x;
   for (x = 0; x < (PatHeader->NoOfPatterns + 1) * (PatHeader->NoOfChannels + 1); x++)
   {
      unsigned char *CurrentPos = (unsigned char *)Patterns[x];
      unsigned char *Pattern = CurrentPos;
      int y = 0;
      do
      {
	 if (*CurrentPos != 0xFC)
	 {
	    CurrentPos += 5;
	    y++;
	 }
	 else
	 {
	    y += CurrentPos[1];
	    CurrentPos += 7;
	 }
	 
      } 
      while (y < 64);
      
      if (y != 64)
      {
	 Free();
	 return (LOADPART_Patterns | LOADFAIL_Corrupt | x);
      }
      
      Patterns[x] = new unsigned char[CurrentPos - Pattern];
      memcpy(Patterns[x],Pattern,CurrentPos - Pattern);
   }

   return LOADOK;
}
									/*}}}*/

// ULTFormatClass::GetTypeName - Returns the general typename		/*{{{*/
// ---------------------------------------------------------------------
/* */
const char *museULTFormatClass::GetTypeName()
{
   return "UltraTracker Module";
}
									/*}}}*/
// ULTFormatClass::GetClassForFile - Return the metclass for fname	/*{{{*/
// ---------------------------------------------------------------------
/* Matches *.ult */
museFormatClass *museULTFormatClass::GetClassForFile(const char *FName)
{
   const char *End = FName + strlen(FName);
   for(;*End != '.' && End != FName;End --);

   if (End != FName)
      End++;

   if (_stricmp(End,"ULT") != 0)
      return 0;
   return this;
}
									/*}}}*/
// ULTFormatClass::GetScanSize - Return the songs header size		/*{{{*/
// ---------------------------------------------------------------------
/* */
unsigned long museULTFormatClass::GetScanSize()
{
   return sizeof(ULTHeader);
}
									/*}}}*/
// ULTFormatClass::Scan - Get simple song information			/*{{{*/
// ---------------------------------------------------------------------
/* Returns information from the first ult header. */
unsigned long museULTFormatClass::Scan(octet *Region, museSongInfo &Info)
{
   museFormatClass::Scan(Region,Info);
   ULTHeader *Header = (ULTHeader *)Region;
   
   int UltType = 0;
   if (!memcmp (Header->IDText, "MAS_UTrack_V001", 15))
      UltType = 1;
   else 
      if (!memcmp (Header->IDText, "MAS_UTrack_V002", 15))
         UltType = 2;
      else 
         if (!memcmp (Header->IDText, "MAS_UTrack_V003", 15))
            UltType = 3;
         else 
            if (!memcmp (Header->IDText, "MAS_UTrack_V004", 15))
               UltType = 4;

   if (UltType == 0)
      return 1;
   
   // Get the title
   Info.Title = new char[32];
   unsigned int I;
   for (I = 0; I != 32; I++)
      Info.Title[I] = Header->Name[I];
   Info.Title[I] = 0;


   // Get the title
   Info.Title = new char[34];
   for (I = 0; I != 32; I++)
      Info.Title[I] = Header->Name[I];
   Info.Title[32] = 0;
   char *I2;
   for (I2 = &Info.Title[31]; *I2 == ' ' && I2 != Info.Title; I2--);
   if (I2 == Info.Title)
      *I2 = 0;
   else
      I2[1] = 0;

   char *C = "UltraTracker Module";
   if (UltType == 1)
      C = "UltraTracker V1.0 Module";
   
   if (UltType == 2)
      C = "UltraTracker V1.4 Module";
   
   if (UltType == 3)
      C = "UltraTracker V1.5 Module";
   
   if (UltType == 4)
      C = "UltraTracker V1.6 Module";

   Info.TypeName = _strdupcpp(C);
   Info.ClassName = GetTypeName();
   
   // These are all in secondary headers
   Info.Channels = 0;
   Info.Patterns = 0;
   Info.Orders = 0;

   return 0;
}
									/*}}}*/
// UTLFormatClass::museULTFormatClass - Constructor			/*{{{*/
// ---------------------------------------------------------------------
/* Sets the version numbers */
museULTFormatClass::museULTFormatClass()
{
   MajorVersion = 1;
   MinorVersion = 2;
}
									/*}}}*/
