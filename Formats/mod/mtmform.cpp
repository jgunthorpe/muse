// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   MTMFormat - Muse file format to handle MTM files.
   
   ##################################################################### */
									/*}}}*/
// Includes								/*{{{*/
#include <muse.h>
#include <math.h>

#include <mtmform.h>

#include "mtmstruc.h"
   									/*}}}*/

museMTMFormatClass *museMTMFormat::Meta = new museMTMFormatClass;

// MTMFormat::museMTMFormat - Constructor				/*{{{*/
// ---------------------------------------------------------------------
/* 0s data members */
museMTMFormat::museMTMFormat()
{
   // Init the structures
   Header = 0;
   PatternData = 0;
   TrackData = 0;
   SampleData = 0;
   PatternData = 0;
   OrderData = 0;
   CommentData = 0;
   Samples = 0;
   Private = false;
}
									/*}}}*/
// MTMFormat::Free - Frees the module					/*{{{*/
// ---------------------------------------------------------------------
/* */
void museMTMFormat::Free()
{
   if (Private)
   {
      delete [] CommentData;
      delete [] PatternData;
      delete [] TrackData;
      delete [] OrderData;
      delete [] SampleData;
      delete Header;
   }
    
   delete [] Samples;
   Header = 0;
   TrackData = 0;
   SampleData = 0;
   PatternData = 0;
   OrderData = 0;
   CommentData = 0;
   Samples = 0;
   Private = false;

   museMODGeneric::Free();
}
									/*}}}*/
// MTMFormat::LoadMemModule - Reads the module from a memory block	/*{{{*/
// ---------------------------------------------------------------------
/* Just maps the pointers directly into the file, it's quite nicely setup
   for this. BTW, a Track is a channel. If I remember MTM's store the
   pattern data by channel and have a mapping of what channels compose 
   a pattern */
long museMTMFormat::LoadMemModule(unsigned char *Region,unsigned long Size)
{
   Free();
   
   unsigned char *CurrentPos = Region;
   unsigned char *End = Region + Size;

   // Read the header
   Header = (MTMHeader*) CurrentPos;
   CurrentPos += sizeof(MTMHeader);
   if (CurrentPos > End)
   {
      Free();
      return LOADPART_Header | LOADFAIL_Truncated | 0;
   }
   
   // Check the header for the Signature
   if (memcmp (Header->ID, "MTM", 3))
   {
      Free();
      return (LOADPART_Header | LOADFAIL_Sigcheck);
   }
   
   // Read the instruments
   if (CurrentPos + Header->NoOfSamples * sizeof (MTMInstrument) > End)
   {
      Free();
      return LOADPART_Samples | LOADFAIL_Truncated | (End - CurrentPos) / sizeof (MTMInstrument);
   }
   SampleData = (MTMInstrument *) CurrentPos;
   CurrentPos += Header->NoOfSamples * sizeof (MTMInstrument);
   
   // Read the Orders
   OrderData = (unsigned char *) CurrentPos;
   CurrentPos += 128;
   if (CurrentPos > End)
   {
      Free();
      return (LOADPART_Header | LOADFAIL_Truncated | 1);
   }
   
   // Read the Tracks
   if (CurrentPos + Header->NoTracks*192 > End)
   {
      Free();
      return LOADPART_Patterns | LOADFAIL_Truncated | 0x0000 | ((End - CurrentPos) / 192);
   }
   TrackData = CurrentPos;
   CurrentPos += Header->NoTracks*192;
   
   // Read the patterns
   if (CurrentPos + 32 * 2 * (Header->LastPattern + 1) > End)
   {
      Free();
      return (LOADPART_Patterns | LOADFAIL_Truncated | 0x8000 | (End - CurrentPos) / (32 * 2));
   }
   PatternData = (unsigned short *) CurrentPos;
   CurrentPos += 32 * 2 * (Header->LastPattern + 1);
   
   // Read the Comments
   CommentData = (char *)CurrentPos;
   CurrentPos += Header->CommentLength;
   if (CurrentPos > End)
   {
      Free();
      return (LOADPART_Message | LOADFAIL_Truncated);
   }
   
   // And the samples
   Samples = new unsigned char *[Header->NoOfSamples];
   memset(Samples,0,sizeof(unsigned char *)*Header->NoOfSamples);
   for (int I = 0; I != Header->NoOfSamples; I++)
   {
      Samples[I] = CurrentPos;
      CurrentPos += SampleData[I].SampleLen;
      
      // Corrupt -- Very rare, just nuke the entire sample, makes rest simpler
      if (CurrentPos > End)
	 Samples[I] = 0;
   }
      
   return LOADOK;
}
									/*}}}*/
// MTMFormt::GetRowElements - Convert the MTM to Protracker Data	/*{{{*/
// ---------------------------------------------------------------------
/* Nothing fancy here, simple translation. */
long museMTMFormat::GetRowElements(SequenceMODElement *Elements,unsigned long Row,unsigned long Pattern)
{
   unsigned int NoteTable[17] = {1712,1616,1524,1440,1356,1280,1208,1140,
                                 1076,1016,960, 907, 907, 907, 907, 907, 907};
   unsigned char BlankData[3] = {0, 0, 0};

   if (Header == 0 || Row >= Header->BeatsPerTrack || Pattern > Header->LastPattern)
      return 1;

   // Go through each channel
   for (unsigned int I = 0; I != Header->NoOfTracks; I++)
   {
      unsigned char *CurrentByte;

      if (PatternData[Pattern*32+I] == 0)
         CurrentByte = BlankData;
      else 
	 if (PatternData[Pattern*32+I]-1 < Header->NoTracks)
	    CurrentByte = &TrackData[((PatternData[Pattern*32+I] - 1) * 64 + Row) * 3];
         else 
	    CurrentByte = BlankData;

      (*Elements)[I].Period = CurrentByte[0] >> 2;

      // Apply a note translation to the period value.
      if ((*Elements)[I].Period != 0)
         (*Elements)[I].Period = (int)(((float)NoteTable[(*Elements)[I].Period%12])/pow(2,(*Elements)[I].Period / 12))*16;
      
      (*Elements)[I].Sample = (CurrentByte[1] >> 4) + ((CurrentByte[0] & 3) << 4);
      (*Elements)[I].Effect = CurrentByte[1] & 15;
      (*Elements)[I].EffectParam = (unsigned char) CurrentByte[2];
      (*Elements)[I].Volume = 0xFF;
   };
   return 0;
}
									/*}}}*/
// MTMFormat::GetInitValues - Return inital setup values		/*{{{*/
// ---------------------------------------------------------------------
/* MTM's do not store these in the header so they are constant */
octet museMTMFormat::GetInitValues(unsigned char *Speed,unsigned char *Tempo, 
				   unsigned char *GlobalVol)
{
   *Speed = 6;
   *Tempo = 125;
   *GlobalVol = 64;
   return 1;
}
									/*}}}*/
// MTMFormat::GetOrderList - Return the order data for the Song		/*{{{*/
// ---------------------------------------------------------------------
/* */
void museMTMFormat::GetOrderList(unsigned char **List,unsigned long *Count)
{
   if (Header == 0)
      return;

   *List = OrderData;
   *Count = Header->LastOrder + 1;
}
									/*}}}*/
// MTMFormat::GetMODPan - Get MOD panning data				/*{{{*/
// ---------------------------------------------------------------------
/* The pan positions are taken from the MTM header */
void museMTMFormat::GetMODPan(unsigned char *Pan)
{
   // Set default pan positions
   for (unsigned int I = 0; I != Header->NoOfTracks; I++)
      Pan[I] = Header->PanPos[I];
}
									/*}}}*/
// MTMFormat::Keep - Clone important internal data			/*{{{*/
// ---------------------------------------------------------------------
/* After this function is called the playback routine will be able to
   execute without having the memory block passed to LoadMemModule 
   resident. However, it will be unable to download the samples which
   are not cloned by the routine. The routine was intended for use with
   wavetable devices that are capable of sample downloading (GUS) */
long museMTMFormat::Keep()
{
   if (Header == 0 || Private == true)
      return FAIL_NotLoaded;
   
   Private = true;

   // Clone the header
   MTMHeader *OldH = Header;
   Header = new MTMHeader;
   memcpy(Header,OldH,sizeof(MTMHeader));

   // Clone the Sample data
   MTMInstrument *OldI = SampleData;
   SampleData = new MTMInstrument[Header->NoOfSamples];
   memcpy(SampleData,OldI,Header->NoOfSamples*sizeof(MTMInstrument));

   // Clone the order data
   unsigned char *OldOrds = OrderData;
   OrderData = new unsigned char[128];
   memcpy(OrderData,OldOrds,128);

   // Clone the Tracks
   unsigned char *OldT = TrackData;
   TrackData = new unsigned char[Header->NoTracks*192];
   memcpy(TrackData,OldT,Header->NoTracks*192);

   // Clone the pattern data
   unsigned short *OldP = PatternData;
   PatternData = new unsigned short[32 * (Header->LastPattern + 1)];
   memcpy(PatternData,OldP,32*2*(Header->LastPattern + 1));

   // Clone the comment data
   char *OldC = CommentData;
   CommentData = new char[Header->CommentLength];
   memcpy(CommentData,OldC,Header->CommentLength);
   
   return LOADOK;
}
									/*}}}*/

// MTMFormat::GetSongInfo - Returns information about the song		/*{{{*/
// ---------------------------------------------------------------------
/* Simply fills the info structure. */
long museMTMFormat::GetSongInfo(museSongInfo &Info)
{
   Info.Free();
   
   if (Header == 0)
      return FAIL_NotLoaded;
   
   Info.Patterns = Header->LastPattern + 1;
   Info.Orders = Header->LastOrder + 1;
   Info.Channels = Header->NoOfTracks;

   Info.ClassName = ((museFormatClass *)GetMetaClass())->GetTypeName();
   Info.TypeName = _strdupcpp("MulitTracker Module");
   Info.Title = _strndupcpp(Header->Name,sizeof(Header->Name));

   // Process the comment
   char *End = CommentData + Header->CommentLength;
   char *C;
   for (C = CommentData; C != End && (*C == ' ' || *C == 0);C++);
   if (Header->CommentLength == 0 || C == End)
      return FAIL_None;

   // 20 possible lines in mtm comments
   Info.ModComment = new char[Header->CommentLength + 21];

   // Process the Comment adding in \ns in the correct spots. (40 cols)
   char *Comm = Info.ModComment;
   for (C = CommentData; C != End;)
   {
      char *Start = C;
      for (;*C != 0 && C != End && (C - Start != 40); C++,Comm++)
         *Comm = *C;
      *Comm = '\n';
      Comm++;
      for (;*C == 0 && C != End; C++);
   }
   *Comm = 0;

   return FAIL_None;
}
									/*}}}*/
// MTMFormat::GetRowsAPattern - Return the row count from the header	/*{{{*/
// ---------------------------------------------------------------------
/* MTM's can have variable rows per pattern */
unsigned short museMTMFormat::GetRowsAPattern()
{
   if (Header == 0)
      return 0;
   return Header->BeatsPerTrack;
}
									/*}}}*/
// MTMFormat::GetSongSamples - Return the songs sampledata		/*{{{*/
// ---------------------------------------------------------------------
/* I have never seen a MTM with 16 bit samples so this might not work
   for such songs */
void museMTMFormat::GetSongSamples(SequenceSample &Samps)
{
   if (Header == 0)
      return;
   
   Samps.construct();
   Samps.reserve(Header->NoOfSamples);
   for (unsigned int I = 0; I < Header->NoOfSamples; I++)
   {
      museSample &Samp = Samps[I];
      if ((SampleData[I].SampleLen) == 0)
         Samp.Sample = 0;
      else
         Samp.Sample = Samples[I];

      // Looping is enabled
      if (SampleData[I].LoopLen - SampleData[I].LoopBeg > 2)
      {
         Samp.LoopBegin = SampleData[I].LoopBeg;
         Samp.LoopEnd = SampleData[I].LoopLen;
         Samp.SampleEnd = SampleData[I].SampleLen;
         if (Samp.LoopEnd > Samp.SampleEnd)
            Samp.LoopEnd = Samp.SampleEnd;
         Samp.Flags = INST_Loop;
      }
      else
      {
         Samp.LoopBegin = 0;
         Samp.LoopEnd = SampleData[I].SampleLen;
         Samp.SampleEnd = SampleData[I].SampleLen;
         Samp.Flags = 0;
      }
      Samp.Name = SampleData[I].FName;
      Samp.MaxNameLen = 22;
      Samp.SampleName = 0;
      Samp.MaxSNameLen = 0;
      Samp.InstNo = I;

      if (SampleData[I].Attrib & 1) 
	 Samp.Flags |= INST_16Bit;

      double LocalFT;
      LocalFT = SampleData[I].FineTune & 0xF;
      Samp.FineTune = SampleData[I].FineTune & 0xF;
      if (LocalFT > 7)
         LocalFT -= 16;

      Samp.Center = (int)(14317056/4/(428 * (1 - LocalFT * 0.0037164 * 2)));
      Samp.Volume = SampleData[I].Volume*0xFF/64;
   }
}
									/*}}}*/
// MTMFormatClass::GetTypeName - Returns the Typename			/*{{{*/
// ---------------------------------------------------------------------
/* */
const char *museMTMFormatClass::GetTypeName()
{
   return "MultiTracker Module";
}
									/*}}}*/
// MTMFormatClass::GetClassForFile - Returns the metaclass for fname	/*{{{*/
// ---------------------------------------------------------------------
/* Matches *.mtm */
museFormatClass *museMTMFormatClass::GetClassForFile(const char *FName)
{
   const char *End = FName + strlen(FName);
   for(;*End != '.' && End != FName;End --);

   if (End != FName)
      End++;

   if (_stricmp(End,"MTM") != 0)
      return 0;
   return this;
}
									/*}}}*/
// MTMFormatClass::GetScanSize - Return the size of the header 		/*{{{*/
// ---------------------------------------------------------------------
/* */
unsigned long museMTMFormatClass::GetScanSize()
{
   return sizeof(MTMHeader);
}
									/*}}}*/
// MTMFormatClass::Scan - Scans the MTM header				/*{{{*/
// ---------------------------------------------------------------------
/* Converts the mtm header to a museSongInfo */
unsigned long museMTMFormatClass::Scan(octet *Region, museSongInfo &Info)
{
   MTMHeader *Header = (MTMHeader *)Region;

   // Check the header for the Signature
   if (!(Header->ID[0] == 'M' && Header->ID[1] == 'T' &&
       Header->ID[2] == 'M'))
      return 1;

   Info.TypeName = _strdupcpp("MultiTracker Module");
   Info.ClassName = GetTypeName();
   Info.Channels = Header->NoOfTracks;
   Info.Patterns = Header->LastPattern + 1;
   Info.Orders = Header->LastOrder + 1;

   Info.Title = _strndupcpp(Header->Name,sizeof(Header->Name));

   return 0;
}
									/*}}}*/
// MTMFormatClass::museMTMFormatClass - Constructor			/*{{{*/
// ---------------------------------------------------------------------
/* Sets the version numbers */
museMTMFormatClass::museMTMFormatClass()
{
   MajorVersion = 1;
   MinorVersion = 2;
}
									/*}}}*/
