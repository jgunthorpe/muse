// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   ITFormat - Muse file format to handle IT files.
   
   ##################################################################### */
									/*}}}*/
// Includes								/*{{{*/
#include <muse.h>
#include <itform.h>
#include <math.h>

#include "itstruc.h"
   									/*}}}*/

museITFormatClass *museITFormat::Meta = new museITFormatClass;

// ITFormat::museITFormat - Constructor					/*{{{*/
// ---------------------------------------------------------------------
/* 0s data members */
museITFormat::museITFormat()
{
   Header = 0;
   Samples = 0;
   Instruments = 0;
   Instruments2 = 0;
   Patterns = 0;
   TempPatMem = 0;
   MaxNoOfChannels = 32;
   Private = false;
   memset(ChannelMask,0,sizeof(ChannelMask));

}
									/*}}}*/
// ITFormat::Free - Frees the module					/*{{{*/
// ---------------------------------------------------------------------
/* Cloned data is dumped too */
void museITFormat::Free()
{
   int x;
   
   if (Header == 0)
      return;

   if (Private)
   {
      if (Samples) for (x = 0; x < Header->NoOfSamples; x++)
	 delete Samples[x];

      if (Instruments) for (x = 0; x < Header->NoOfInstruments; x++)
	 delete Instruments[x];
      
      if (Patterns) for (x = 0; x < Header->NoOfPatterns; x++)
	 delete [] Patterns[x];
      
      delete Header;
   }

   delete [] Samples;
   Samples = 0;

   delete [] Instruments;
   Instruments = 0;

   delete [] Instruments2;
   Instruments2 = 0;

   delete [] Patterns;
   Samples = 0;

   delete [] TempPatMem;
   TempPatMem = 0;

   Private = false;
   memset (ChannelMask, 0, sizeof(ChannelMask));
   Header = 0;

   museFormatBase::Free();
}
									/*}}}*/
// ITFormat::LoadMemModule - Loads a module from a memory region	/*{{{*/
// ---------------------------------------------------------------------
/* This loads and integrity checks the entire song. */
long museITFormat::LoadMemModule(unsigned char *Region, unsigned long Size)
{
   int x;
   Free();
   
   unsigned char *CurrentPos = Region;
   unsigned char *End = Region + Size;
   
   FileLoc = Region;
   FileSize = Size;
   Header = (ITHeader *) CurrentPos;
   CurrentPos += sizeof (ITHeader);
   if (CurrentPos > End)
   {
      Free();
      return (LOADPART_Header | LOADFAIL_Truncated);
   }
   if (memcmp (Header->Sig, "IMPM", 4))
   {
      Free();
      return (LOADPART_Header | LOADFAIL_Sigcheck);
   }
   
   Orders = CurrentPos;
   CurrentPos += Header->NoOfOrders;
   
   register unsigned long InstSize;
   if (Header->Compatability < 0x200) 
      InstSize = sizeof (ITInstrument);
   else 
      InstSize = sizeof (IT2Instrument);
   
   Instruments = new ITInstrument *[Header->NoOfInstruments];
   if (Header->Compatability >= 0x200) 
      Instruments2 = new IT2Instrument *[Header->NoOfInstruments];
   for (x = 0; x < Header->NoOfInstruments; x++) 
   {
      if (CurrentPos + 4 > End)
      {
	 Free();
	 return (LOADPART_Instruments | LOADFAIL_Truncated | x);
      }
      
      if ((unsigned long) (*((unsigned long *) CurrentPos) + InstSize) > Size)
      {
	 Free();
	 return (LOADPART_Instruments | LOADFAIL_Truncated | x);
      }
      
      if (memcmp (((ITInstrument *) (Region + *((unsigned long *) CurrentPos)))->Sig, "IMPI", 4))
      {
	 Free();
	 return (LOADPART_Instruments | LOADFAIL_Sigcheck | x);
      }
      
      Instruments[x] = (ITInstrument *) (*((unsigned long *) CurrentPos) + Region);
      if (Header->Compatability >= 0x200) 
	 Instruments2[x] = (IT2Instrument *)Instruments[x];
      CurrentPos += 4;
   }
   
   Samples = new ITSample *[Header->NoOfSamples];
   for (x = 0; x < Header->NoOfSamples; x++) 
   {
      register ITSample *Sam;
      
      if (CurrentPos + 4 > End)
      {
	 Free();
	 return (LOADPART_Samples | LOADFAIL_Truncated | x);
      }
      
      if ((unsigned long) (*((unsigned long *) CurrentPos) + sizeof (ITSample)) > Size)
      {
	 Free();
	 return (LOADPART_Samples | LOADFAIL_Truncated | x);
      }
      
      Sam = (ITSample *) (Region + *((unsigned long *) CurrentPos));
      
      if (memcmp (Sam->Sig, "IMPS", 4))
      {
	 Free();
	 return (LOADPART_Samples | LOADFAIL_Sigcheck | x);
      }
      
      if (Sam->SamplePointer + Sam->Length > Size)
      {
	 Free();
	 return (LOADPART_Samples | LOADFAIL_Sigcheck | x);
      }
      
      if ((Sam->Flags & 2) && (Header->Version < 0x202 || !(Sam->Convert & 1)))
      {
	 unsigned int y;
	 unsigned short *Data;
	 
	 Data = (unsigned short *) (Region + Sam->SamplePointer);
	 
	 for (y = 0; y < Sam->Length; y++)
	    Data[y] ^= 0x8000;
      }
      
      Samples[x] = (ITSample *) (*((unsigned long *) CurrentPos) + Region);
      CurrentPos += 4;
   }
   
   Patterns = new ITPattern *[Header->NoOfPatterns];
   for (x = 0; x < Header->NoOfPatterns; x++) 
   {
      if (CurrentPos + 4 > End)
      {
	 Free();
	 return (LOADPART_Patterns | LOADFAIL_Truncated | x);
      }
      
      if (*((unsigned long *) CurrentPos) == 0)
      {
	 Patterns[x] = 0;
      }
      else 
      {
	 if (*((unsigned long *) CurrentPos) + 4 > Size)
	 {
	    Free();
	    return (LOADPART_Patterns | LOADFAIL_Truncated | x);
	 }
	 
	 if ((*((unsigned long *) CurrentPos) + 8 + ((ITPattern *) (Region + *((unsigned long *) CurrentPos)))->Length) > Size)
	 {
	    Free();
	    return (LOADPART_Patterns | LOADFAIL_Truncated | x);
	 }
	 
	 Patterns[x] = (ITPattern *) (*((unsigned long *) CurrentPos) + Region);
      }
      
      CurrentPos += 4;
   }
   
   memset(ChannelMask, 0, sizeof(ChannelMask));
   for (int CurOrder = 0; CurOrder < Header->NoOfOrders; CurOrder++)
   {
      if (Orders[CurOrder] == 254)
	 continue;
      if (Orders[CurOrder] == 255)
	 break;
      
      x = Orders[CurOrder];
      if (x < Header->NoOfPatterns && Patterns[x] != 0)
      {
	 unsigned char *DataPointer = &Patterns[x]->Data;
	 unsigned char LastBytes[64];
	 int CurRow = 0;
	 
	 memset (LastBytes, 0, 64);
	 
	 while (CurRow < Patterns[x]->Rows)
	 {
	    //For each channel, calculate the parameters
	    while (*DataPointer)
	    {
	       unsigned char Channel, Mask;
	       if (DataPointer >= (&Patterns[x]->Data + 8 + Patterns[x]->Length))
	       {
		  Free();
		  return (LOADPART_Patterns | LOADFAIL_Corrupt | x);
	       }
	       Channel = *DataPointer++;
	       
	       if (Channel & 128)
	       {
		  if (DataPointer >= (&Patterns[x]->Data + 8 + Patterns[x]->Length))
		  {
		     Free();
		     return (LOADPART_Patterns | LOADFAIL_Corrupt | x);
		  }
		  Mask = *DataPointer++;
		  LastBytes[Channel & 63] = Mask;
	       }
	       else
		  Mask = LastBytes[Channel & 63];
	       Channel &= 63;
	       Channel--;
	       
	       if ((Mask & 1) && Channel != 0xFF && Header->ChannelPan[Channel] < 128) ChannelMask[Channel] = true;
	       
	       if (Mask & 1)
	       {
		  if (DataPointer >= (&Patterns[x]->Data + 8 + Patterns[x]->Length))
		  {
		     Free();
		     return (LOADPART_Patterns | LOADFAIL_Corrupt | x);
		  }
		  DataPointer++;
	       }
	       if (Mask & 2)
	       {
		  if (DataPointer >= (&Patterns[x]->Data + 8 + Patterns[x]->Length))
		  {
		     Free();
		     return (LOADPART_Patterns | LOADFAIL_Corrupt | x);
		  }
		  DataPointer++;
	       }
	       if (Mask & 4)
	       {
		  if (DataPointer >= (&Patterns[x]->Data + 8 + Patterns[x]->Length))
		  {
		     Free();
		     return (LOADPART_Patterns | LOADFAIL_Corrupt | x);
		  }
		  DataPointer++;
	       }
	       if (Mask & 8)
	       {
		  if (DataPointer + 1 >= (&Patterns[x]->Data + 8 + Patterns[x]->Length))
		  {
		     Free();
		     return (LOADPART_Patterns | LOADFAIL_Corrupt | x);
		  }
		  DataPointer += 2;
	       }
	    }
	    if (DataPointer >= (&Patterns[x]->Data + 8 + Patterns[x]->Length))
	    {
	       Free();
	       return (LOADPART_Patterns | LOADFAIL_Corrupt | x);
	    }
	    DataPointer++;
	    
	    CurRow++;
	 }
      }
   }
    
   // Instrument mode
   if (Header->Flags & 4)
      MaxNoOfChannels = 0;
   else
   {
      MaxNoOfChannels = 0;
      for (int I = 0; I != 64; I++)
	 if (ChannelMask[I] == true)
      {
	 MaxNoOfChannels++;
      }
   }

   // Check the comment too 
   if (Header->MsgLen + Header->MsgOffset > Size)
      Header->MsgLen = 0;
   
   return LOADOK;
}
   									/*}}}*/
// ITFormat::GetSongInfo - Returns information about the song		/*{{{*/
// ---------------------------------------------------------------------
/* Simply fills the info structure. */
long museITFormat::GetSongInfo(museSongInfo &Info)
{
   Info.Free();
   
   if (Header == 0)
      return FAIL_NotLoaded;
   
   Info.Patterns = Header->NoOfPatterns;
   Info.Orders = Header->NoOfOrders;
   Info.Channels = MaxNoOfChannels;
   
   Info.ClassName = ((museFormatClass *)GetMetaClass())->GetTypeName();
   Info.Title = _strndupcpp(Header->SongName,sizeof(Header->SongName));

   // Get the type name
   float Ver = (Header->Version & 0xF)*0.01 + 
               ((Header->Version & 0xF0) >> 4)*0.1 + 
               ((Header->Version & 0xF00) >> 8);
   
   char S[40];
   if (Header->Flags & 4)
      sprintf(S,"Impulse Inst Module V%1.2f",Ver);
   else 
      sprintf(S,"Impulse Samp Module V%1.2f",Ver);
   Info.TypeName  = _strdupcpp(S);

   if (Header->MsgLen == 0)
      return FAIL_None;
   
   // Get the comment data
   Info.ModComment = new char[Header->MsgLen + 1];
   strncpy(Info.ModComment,(char *)(FileLoc + Header->MsgOffset),
	   Header->MsgLen);

   // Strip the \rs from the text. Impulse puts them in for some reason.
   int I = 0;
   for (char *C = Info.ModComment; C != Info.ModComment + Header->MsgLen; 
	C++,I++)
   {
      if (*C == 0 || *C == '\r')
	 *C = '\n';
   }
   Info.ModComment[Header->MsgLen] = 0;
   
   return FAIL_None;
}
									/*}}}*/
// ITFormat::GetSongSamples - Return the songs samples			/*{{{*/
// ---------------------------------------------------------------------
/* This converts the songs samples into muse format. */
void museITFormat::GetSongSamples(SequenceSample &Samps)
{
   if (Header == 0)
      return;

   Samps.construct();
   Samps.reserve(Header->NoOfSamples);

   for (int x = 0; x < Header->NoOfSamples; x++)
   {
      museSample &Samp = Samps[x];
      
      // Enable 0->' ' translation
      Samp.Flags = 1 << 31;
      
      Samp.Sample = FileLoc + Samples[x]->SamplePointer;
      Samp.SampleEnd = Samples[x]->Length;
      if (Samples[x]->Length == 0)
	 Samp.Sample = 0;
      else 
      {
	 if ((Samples[x]->Flags & 160) == 32)
	 {
	    Samp.LoopBegin = Samples[x]->SustainLoopBegin;
	    Samp.LoopEnd = Samples[x]->SustainLoopEnd;
	    if (Samp.LoopEnd > Samp.SampleEnd)
	       Samp.LoopEnd = Samp.SampleEnd;
	    Samp.Flags = INST_Loop;
	 }
	 else 
	    if ((Samples[x]->Flags & 160) == 160)
	    {
	       Samp.LoopBegin = Samples[x]->SustainLoopBegin;
	       Samp.LoopEnd = Samples[x]->SustainLoopEnd;
	       if (Samp.LoopEnd > Samp.SampleEnd)
		  Samp.LoopEnd = Samp.SampleEnd;
               Samp.Flags = INST_PingPong;
	    } 
	    else 
	       if ((Samples[x]->Flags & 80) == 16)
	       {
		  Samp.LoopBegin = Samples[x]->LoopBegin;
		  Samp.LoopEnd = Samples[x]->LoopEnd;
		  if (Samp.LoopEnd > Samp.SampleEnd)
		     Samp.LoopEnd = Samp.SampleEnd;
		  Samp.Flags = INST_Loop;
	       }
	       else 
	          if ((Samples[x]->Flags & 80) == 80)
	          { 
		     Samp.LoopBegin = Samples[x]->LoopBegin;
		     Samp.LoopEnd = Samples[x]->LoopEnd;
		     if (Samp.LoopEnd > Samp.SampleEnd)
			Samp.LoopEnd = Samp.SampleEnd;
		     Samp.Flags = INST_PingPong;
		  }
    	          else
	          {
		     Samp.LoopBegin = 0;
		     Samp.LoopEnd = Samples[x]->Length;
		     Samp.Flags = 0;
		  }
      }

      Samp.Name = Samples[x]->Name;
      Samp.SampleName = Samples[x]->SampleName;
      Samp.InstNo = x;
      Samp.MaxNameLen = 25;
      Samp.MaxSNameLen = 13;
      Samp.Center = Samples[x]->BaseFrequency;
      Samp.Volume = Samples[x]->Volume*0xFF/64;
      Samp.FineTune = 0;

      if (Samp.Sample + Samp.SampleEnd > FileLoc + FileSize)
      {
	 Samp.Sample = 0;
	 Samp.SampleEnd = 0;
	 continue;
      }
       
      if (Samples[x]->Flags & 2) 
      {
	 Samp.Flags |= INST_16Bit;
	 Samp.SampleEnd *= 2;
	 Samp.LoopEnd *= 2;
	 Samp.LoopBegin *= 2;
      } 
      else 
	 if (Samples[x]->Convert & 1) 
	    Samp.Flags |= INST_Signed;
   }      
}
									/*}}}*/
// ITFormat::Keep - Clone important internal data			/*{{{*/
// ---------------------------------------------------------------------
/* After this function is called the playback routine will be able to
   execute without having the memory block passed to LoadMemModule 
   resident. However, it will be unable to download the samples which
   are not cloned by the routine. The routine was intended for use with
   wavetable devices that are capable of sample downloading (GUS) */
long museITFormat::Keep()
{
   if (Header == 0 || Private == true)
      return FAIL_NotLoaded;
   
   Private = true;
   
   // Clone the header
   ITHeader *OldH = Header;
   Header = new ITHeader;
   memcpy(Header,OldH,sizeof (ITHeader));
   
   // Clone the orders
   octet *OldO = Orders;
   Orders = new octet[Header->NoOfOrders];
   memcpy(Orders,OldO,Header->NoOfOrders);
   
   // Clone the Instruments
   int x;
   for (x = 0; x < Header->NoOfInstruments; x++) 
   {
      if (Header->Compatability >= 0x200)
      {
	 IT2Instrument *Inst = Instruments2[x];

	 Instruments2[x] = new IT2Instrument;
	 memcpy(Instruments2[x],Inst,sizeof(IT2Instrument));
	 Instruments[x] = (ITInstrument *)Instruments2[x];
      } 
      else 
      {
	 ITInstrument *Inst = Instruments[x];
	 Instruments[x] = new ITInstrument;
	 memcpy(Instruments[x],Inst,sizeof(ITInstrument));
      }
   }

   // Clone the samples
   for (x = 0; x < Header->NoOfSamples; x++) 
   {
      ITSample *OldS = Samples[x];
      Samples[x] = new ITSample;
      memcpy (Samples[x],OldS,sizeof(ITSample));
   }

   // Clone the pattern data
   for (x = 0; x < Header->NoOfPatterns; x++) 
   {
      if (Patterns[x] == 0)
	 continue;

      ITPattern *OldP = Patterns[x];
      Patterns[x] = (ITPattern *)new unsigned char[8 + OldP->Length];
      memcpy (Patterns[x],OldP,8 + OldP->Length);
   }
   
   return LOADOK;
}
									/*}}}*/

// ITFormatClass::GetTypeName - Returns the general type name		/*{{{*/
// ---------------------------------------------------------------------
/* */
const char *museITFormatClass::GetTypeName()
{
   return "Impulse Tracker Module";
}
									/*}}}*/
// ITFormatClass:GetClassForFile - Returns the metaclass for fname	/*{{{*/
// ---------------------------------------------------------------------
/* Matches *.it */
museFormatClass *museITFormatClass::GetClassForFile(const char *FName)
{
   const char *End = FName + strlen(FName);
   for(;*End != '.' && End != FName;End --);

   if (End != FName)
      End++;

   if (_stricmp(End,"IT") != 0)
      return 0;
   return this;
}
									/*}}}*/
// ITFormatClass::GetScanSize - Return the header size			/*{{{*/
// ---------------------------------------------------------------------
/* A Special mini header is used because the main header contians alot of
   junk that is not needed */
unsigned long museITFormatClass::GetScanSize()
{
    return sizeof(MinITHeader);
}
									/*}}}*/
// ITFormatClass::Scan - Scans the song for header info			/*{{{*/
// ---------------------------------------------------------------------
/* */
unsigned long museITFormatClass::Scan(octet *Region, museSongInfo &Info)
{
   museFormatClass::Scan(Region,Info);
   
   MinITHeader *Header = (MinITHeader *)Region;
   
   // Verify Header is for an IT
   if (memcmp (Header->Sig, "IMPM", 4))
      return 1;
   
   float Ver = (Header->Version & 0xF)*0.01 + 
               ((Header->Version & 0xF0) >> 4)*0.1 + 
               ((Header->Version & 0xF00) >> 8);
   
   char S[40];
   if (Header->Flags & 4)
      sprintf(S,"Impulse Inst Module V%1.2f", Ver);
   else 
      sprintf(S,"Impulse Samp Module V%1.2f", Ver);
   Info.TypeName  = _strdupcpp(S);
   
   Info.ClassName = GetTypeName();
   Info.Channels = 0;                  // Can't tell without more info
   Info.Patterns = Header->NoOfPatterns;
   Info.Orders = Header->NoOfOrders;
   Info.Title = _strndupcpp(Header->SongName,sizeof(Header->SongName));
   
   return 0;
}
									/*}}}*/
// ITFormatClass::museITFormatClass - Constructor			/*{{{*/
// ---------------------------------------------------------------------
/* Sets the version numbers */
museITFormatClass::museITFormatClass()
{
   MajorVersion = 1;
   MinorVersion = 1;
}
									/*}}}*/
