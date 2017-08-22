// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   STMFormat - Muse file format to handle STM files.
   
   ##################################################################### */
									/*}}}*/
// Includes								/*{{{*/
#include <muse.h>
#include <math.h>

#include <stmform.h>

#include "stmstruc.h"
   									/*}}}*/

museSTMFormatClass *museSTMFormat::Meta = new museSTMFormatClass;

// STMFormat::museSTMFormat - Constructor				/*{{{*/
// ---------------------------------------------------------------------
/* 0's data members */
museSTMFormat::museSTMFormat()
{
   // Init the structures
   Samples = 0;
   Header = 0;
   Patterns = 0;
   Private = false;
}
									/*}}}*/
// STMFormat::Free - Frees the loaded module 				/*{{{*/
// ---------------------------------------------------------------------
/* Frees the module */
void museSTMFormat::Free()
{
   if (Private)
       delete [] Patterns;
    
   delete [] Samples;
   Patterns = 0;
   Header = 0;
   Private = false;

   museMODGeneric::Free();
}
									/*}}}*/
// STMFormat::LoadMemModule - Loads the STM from memory block		/*{{{*/
// ---------------------------------------------------------------------
/* STMS are very simple ;> */
long museSTMFormat::LoadMemModule(unsigned char *Region,unsigned long Size)
{
   Free();
   
   unsigned char *CurrentPos = Region;
   unsigned char *End = Region + Size;
   
   Header = (STMHeader *)CurrentPos;
   CurrentPos += sizeof(STMHeader);
   
   if (CurrentPos > End)
   {
      Free();
      return LOADPART_Header | LOADFAIL_Truncated;
   }
   
   if (memcmp (Header->Sig, "!Scream!", 8))
   {
      Free();
      return LOADPART_Header | LOADFAIL_Sigcheck;
   }
   
   // Read the pattern data
   if (CurrentPos + sizeof (STMPattern) * Header->NoOfPatterns > End)
   {
      Free();
      return LOADPART_Patterns | LOADFAIL_Truncated | ((End - CurrentPos) / sizeof (STMPattern));
   }
   Patterns = (STMPattern *)CurrentPos;
   CurrentPos += sizeof (STMPattern) * Header->NoOfPatterns;
   
   // Read the sample data
   Samples = new unsigned char *[31];
   
   for (int I = 0; I != 31; I++)
   {
      Samples[I] = CurrentPos;
      CurrentPos += Header->Samples[I].Length;
      
      // Corrupted
      if (CurrentPos >= End)
      {
	 Samples[I] = 0;
	 continue;
      }
   }
   return 0;
}
									/*}}}*/
// STMFormat::GetRowElements - Return the protracker row data		/*{{{*/
// ---------------------------------------------------------------------
/* A simple table transform is used to convert the effect numbers the
   protracker command '250' is STM Tremor and is not protracker. */
long museSTMFormat::GetRowElements(SequenceMODElement *Elements,
				   unsigned long Row,unsigned long Pattern)
{
   const char EffectTable[16] = {0,15,11,13,10,2,1,3,4,250,0,6,15,7,9,0};
   const short PeriodTable[16] = {1712,1616,1525,1440,1357,1281,1209,1141,
                                  1077,1017,961,907,1712/2,1616/2,1525/2,
                                  1440/2};
   if (Header == 0 || Row >= 64)
      return 1;

   // Go through each channel
   STMRow *CurrentRow = Patterns[Pattern].Row[Row];
   for (unsigned int I = 0; I != 4; I++)
   {
      if (CurrentRow[I].Note == 0xFF)
         (*Elements)[I].Period = 0;
      else
         (*Elements)[I].Period = (long)(PeriodTable[(CurrentRow[I].Note & 15)]/pow(2,(CurrentRow[I].Note/16)))*16;
      (*Elements)[I].Sample = CurrentRow[I].VolInst / 8;
      (*Elements)[I].Effect = EffectTable[CurrentRow[I].VolEffect & 15];
      (*Elements)[I].EffectParam = CurrentRow[I].EffectData;
      if ((*Elements)[I].Effect == 15)
         (*Elements)[I].EffectParam /= 10;
      if ((*Elements)[I].Effect >= 1 && (*Elements)[I].Effect <= 3)
         (*Elements)[I].EffectParam /= 2;
      (*Elements)[I].Volume = (CurrentRow[I].VolInst & 7) + (CurrentRow[I].VolEffect & ~15) / 2;
   }

   return 0;
}
									/*}}}*/
// STMFormat::GetOrderList - Returns the order data for the song.	/*{{{*/
// ---------------------------------------------------------------------
/* This simply returns the order data for the song. */
void museSTMFormat::GetOrderList(unsigned char **List,unsigned long *Count)
{
   if (Header == 0)
      return;

   *List = &Header->Patterns[0];

   int x = 0;
   while (x < 128 && Header->Patterns[x] != 0x63)
      x++;
   *Count = x;
}
									/*}}}*/
// STMFormat::GetMODPan - Return Panning data				/*{{{*/
// ---------------------------------------------------------------------
/* GUSian pan values are used. */
void museSTMFormat::GetMODPan(unsigned char *Pan)
{
   // Set default pan positions
   for (unsigned int I = 0; I != 4; I++)
   {
      switch (I % 4)
      {
         case 0:
            Pan[I] = 0x3;
            break;
         case 1:
            Pan[I] = 0xC;
            break;
         case 2:
            Pan[I] = 0xC;
            break;
         case 3:
            Pan[I] = 0x3;
            break;
      }
   }
};
									/*}}}*/
// STMFormat::GetSongInfo - Returns information about the song		/*{{{*/
// ---------------------------------------------------------------------
/* Simply fills the info structure. */
long museSTMFormat::GetSongInfo(museSongInfo &Info)
{
   Info.Free();
   
   if (Header == 0)
      return FAIL_NotLoaded;
   
   Info.Patterns = Header->NoOfPatterns;
   Info.Orders = 0;
   while (Info.Orders < 128 && Header->Patterns[Info.Orders] != 0x63)
      Info.Orders++;
   Info.Channels = 4;

   Info.ClassName = ((museFormatClass *)GetMetaClass())->GetTypeName();
   Info.TypeName = _strdupcpp("Scream Tracker 2.x Module");
   Info.Title = _strndupcpp(Header->Name,sizeof(Header->Name));

   return FAIL_None;
}
									/*}}}*/
// STMFormat::GetInitValues - Return Inital Playback values.		/*{{{*/
// ---------------------------------------------------------------------
/* Simply returns the intial tempo, speed and volume from the stm header. */
octet museSTMFormat::GetInitValues (unsigned char *Speed, 
				    unsigned char *Tempo, 
				    unsigned char *GlobalVol)
{
   if (Header == 0) return (0);

   *Speed = Header->DefaultTempo / 10;
   *Tempo = 180;
   *GlobalVol = Header->GlobalVolume;
   return (1);
}
									/*}}}*/
// STMFormat::GetSongSamples - Return the songs samples			/*{{{*/
// ---------------------------------------------------------------------
/* */
void museSTMFormat::GetSongSamples(SequenceSample &Samps)
{
   if (Header == 0)
      return;

   Samps.construct();
   Samps.reserve(31);
   for (unsigned int I = 0; I < 31; I++)
   {
      museSample &Samp = Samps[I];
      
      if (Header->Samples[I].Length == 0)
         Samp.Sample = 0;
      else
         Samp.Sample = Samples[I];

      // Looping is enabled
      if (Header->Samples[I].LoopLen != 0xFFFF && Header->Samples[I].LoopLen != 0)
      {
         Samp.LoopBegin = Header->Samples[I].LoopBeg;
         Samp.LoopEnd = Header->Samples[I].LoopLen;
         Samp.SampleEnd = Header->Samples[I].Length;
         if (Samp.LoopEnd > Samp.SampleEnd)
            Samp.LoopEnd = Samp.SampleEnd;
         Samp.Flags = INST_Loop | INST_Signed;
      }
      else
      {
         Samp.LoopBegin = 0;
         Samp.LoopEnd = Header->Samples[I].Length;
         Samp.SampleEnd = Header->Samples[I].Length;
         Samp.Flags = INST_Signed;
      }
      
      Samp.Name = Header->Samples[I].FName;
      Samp.MaxNameLen = 22;
      Samp.SampleName = 0;
      Samp.MaxSNameLen = 0;
      Samp.InstNo = I;

      Samp.Center = Header->Samples[I].FineTune;
      Samp.Volume = Header->Samples[I].Volume*0xFF/64;
      Samp.FineTune = Header->Samples[I].FineTune;
   }
}
									/*}}}*/
// STMFormat::Keep - Clone important internal data			/*{{{*/
// ---------------------------------------------------------------------
/* After this function is called the playback routine will be able to
   execute without having the memory block passed to LoadMemModule 
   resident. However, it will be unable to download the samples which
   are not cloned by the routine. The routine was intended for use with
   wavetable devices that are capable of sample downloading (GUS) */
long museSTMFormat::Keep()
{
   if (Header == 0 || Private == true)
      return FAIL_NotLoaded;
   
   Private = true;
   
   // Clone the pattern data
   STMPattern *OldPats = Patterns;
   Patterns = new STMPattern[Header->NoOfPatterns];
   memcpy(Patterns,OldPats,sizeof(STMPattern)*Header->NoOfPatterns);

   return LOADOK;
}
									/*}}}*/

// STMFormatClass::GetTypeName - Return the type name			/*{{{*/
// ---------------------------------------------------------------------
/* Returns a string describing the module type */
const char *museSTMFormatClass::GetTypeName()
{
   return "Scream Tracker 2.x Module";
}
									/*}}}*/
// STMFormatClass::GetClassForFile - Return the MetaClass for the fname	/*{{{*/
// ---------------------------------------------------------------------
/* Matches *.stm */
museFormatClass *museSTMFormatClass::GetClassForFile(const char *FName)
{
   const char *End = FName + strlen(FName);
   for(;*End != '.' && End != FName;End --);

   if (End != FName)
      End++;

   while (1)
   {
      if (_stricmp(End,"STM") == 0)
         break;
      return 0;
   }
   return this;
}
									/*}}}*/
// STMFormatClass::GetScanSize - Return the header size.		/*{{{*/
// ---------------------------------------------------------------------
/* */
unsigned long museSTMFormatClass::GetScanSize()
{
   return sizeof(STMHeader);
}
									/*}}}*/
// STMFormatClass::Scan - Scan the songs header				/*{{{*/
// ---------------------------------------------------------------------
/* */
unsigned long museSTMFormatClass::Scan(octet *Region, museSongInfo &Info)
{
   STMHeader *Header = (STMHeader *)Region;

   if (memcmp(Header->Sig, "!Scream!", 8))
      return 1;

   Info.TypeName = _strdupcpp("Scream Tracker 2.x Module");
   Info.ClassName = GetTypeName();

   Info.Channels = 4;
   Info.Patterns = Header->NoOfPatterns;
   Info.Orders = 0;
   while (Info.Orders < 128 && Header->Patterns[Info.Orders] != 0x63)
      Info.Orders++;
   Info.Title = _strndupcpp(Header->Name,sizeof(Header->Name));

   return 0;
}
									/*}}}*/
// STMFormatClass::museSTMFormatClass - Constructor			/*{{{*/
// ---------------------------------------------------------------------
/* Set Version info */
museSTMFormatClass::museSTMFormatClass()
{
   MajorVersion = 1;
   MinorVersion = 1;
}
									/*}}}*/
