// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   S3MFormat - Muse file format to handle S3M files.
   
   This is the loader and everything but the playback code. When the song
   is loaded everything possible is done to ensure that it will playback,
   even if parts of the song are missing. The loader accepts a single
   data block which is a memory image of the song, builds a table of
   pointers for the various data things and then range checks everything.

   The original memory block is left totaly unchanged ONLY if the song is
   completely valid. Otherwise small corrections will be written into
   som of the headers.
   
   ##################################################################### */
									/*}}}*/
// Includes								/*{{{*/
#include <muse.h>
#include <s3mform.h>

#include "s3mstruc.h"
   									/*}}}*/

museS3MFormatClass *museS3MFormat::Meta = new museS3MFormatClass;

// S3MFormat::museS3MFormat - Constructor				/*{{{*/
// ---------------------------------------------------------------------
/* 0 data members */
museS3MFormat::museS3MFormat()
{
   // Init the structures
   Header = 0;
   Instruments = 0;
   Patterns = 0;
   Orders = 0;
   Private = false;
   memset(&ChannelPan[0],0,32);
   memset(ChannelMask,0,sizeof(ChannelMask));
}
									/*}}}*/
// S3MFormat::Free - Frees the song data				/*{{{*/
// ---------------------------------------------------------------------
/* If a private copy of the patterndata has been made then it will be
   freed at this time too. */
void museS3MFormat::Free()
{
   int x;
    
   if (Private)
   {
      for (x = 0; x < Header->PatNum; x++)
	 delete [] Patterns[x].Pattern;
      
      delete Orders;
      delete Header;
   }

   delete [] Patterns;
   delete [] Instruments;
   
   Orders = 0;
   Patterns = 0;
   Instruments = 0;
   Header = 0;
   Private = false;
   memset(&ChannelPan[0],0,32);
   memset(ChannelMask,0,sizeof(ChannelMask));

   museFormatBase::Free();
}
									/*}}}*/
// S3MFormat::LoadMemModule - Read a module from a memory block		/*{{{*/
// ---------------------------------------------------------------------
/* This takes a memory image of the song file, verifies it and then 
   assigns a series of internal pointers to the critical sections.
   After this is done their will either be the new song or no song 
   stored by this class. */
long museS3MFormat::LoadMemModule(unsigned char *Region,unsigned long Size)
{
   // Dump the old module 
   Free();
    
   unsigned char *CurrentPos = Region;
   unsigned char *End = Region + Size;
   
   Header = (S3MHeader *)CurrentPos;
   CurrentPos += sizeof(S3MHeader);
   if (CurrentPos > End)
   {
      Free();
      return LOADPART_Header | LOADFAIL_Truncated;
   }
   
   // Check the header for the Signature
   if (memcmp (Header->ID, "SCRM", 4) )
   {
      Free();
      return LOADPART_Header | LOADFAIL_Sigcheck;
   }
   
   // Allocate and read the orders info
   Orders = CurrentPos;
   CurrentPos += Header->OrderNum;
    
   // Temp structures to store the locations of the instruments and patterns
   unsigned short *InstP = (unsigned short *)CurrentPos;
   CurrentPos += 2*Header->InstNum;
   unsigned short *PatP = (unsigned short *)CurrentPos;
   CurrentPos += 2*Header->PatNum;
   
   if (CurrentPos > End)
   {
      Free();
      return LOADPART_Header | LOADFAIL_Truncated;
   }
    
   // Read the channel pan positions
   if (Header->DefaultPan == 252)
   {
      if (CurrentPos + 32 > End)
      {
	 Free();
	 return LOADPART_Header | LOADFAIL_Truncated;
      }
      memcpy(&ChannelPan[0],CurrentPos,32);
      CurrentPos += 32;
   }
   else
      memset(&ChannelPan[0],0,32);
   
   // Read the instrument structures
   Instruments = new S3MBoundInst[Header->InstNum + 1];
   memset(Instruments,0,sizeof(S3MBoundInst)*Header->InstNum);
   unsigned int I;
   for (I = 0; I != Header->InstNum; I++)
   {
      CurrentPos = Region + InstP[I]*16;
      if (CurrentPos + sizeof(Instruments[I].Inst) > End)
	 continue;
      
      memcpy(&Instruments[I].Inst,CurrentPos,sizeof(Instruments[I].Inst));
      CurrentPos += sizeof(Instruments[I].Inst);
      
      // Check the instrument type, only digital
      if (Instruments[I].Inst.Type != 1 || CurrentPos > End)
      {
	 // Preserve the name
	 char Name[40];
	 Instruments[I].Inst.ID[0] = 0;
	 strcpy(Name,Instruments[I].Inst.Name);
	 memset(&Instruments[I],0,sizeof(Instruments[I]));
	 strcpy(Instruments[I].Inst.Name,Name);
	 continue;
      }
        
      // Check the signature for the instrument
      if (memcmp(Instruments[I].Inst.ID, "SCRS", 4 ))
      {
	 Free();
	 return LOADPART_Instruments | LOADFAIL_Sigcheck | I;
      }
      
      // Read the sample information.
      unsigned long Pos = (Instruments[I].Inst.MemSeg1 << 16) + (Instruments[I].Inst.MemSeg2);
      Instruments[I].Inst.MemSeg1 = 0;
      CurrentPos = Region + Pos*16;
      Instruments[I].Sample = CurrentPos;
      
      // Hmm, a single inst larger than the file? Corrupted.
      if (Instruments[I].Inst.Length > Size)
	 Instruments[I].Inst.Length = Instruments[I].Inst.LoopEnd;
    
      // Truncate the instrument.
      if (CurrentPos + Instruments[I].Inst.Length > End)
	 Instruments[I].Inst.Length = End - CurrentPos;
      
      // Hm, instrument starts past the end of the file?!
      if (CurrentPos > End)
	 Instruments[I].Inst.Length = 0;
      
      // Adjust its length
      if (Instruments[I].Inst.Length == 0)
	 Instruments[I].Sample = 0;
   }
   
   // Read the pattern data
   Patterns = new S3MBoundPattern[Header->PatNum];
   memset(Patterns,0,sizeof(S3MBoundPattern)*Header->PatNum);
   for (I = 0; I != Header->PatNum; I++)
   {
      if (PatP[I] == 0)
	 continue;
      
      CurrentPos = Region + PatP[I]*16;
      if (CurrentPos + sizeof(Patterns[I].Length) > End)
      {
	 Free();
	 return LOADPART_Patterns | LOADFAIL_Truncated | I;
      }
      
      Patterns[I].Length = *((unsigned short *)CurrentPos);
      CurrentPos += sizeof(Patterns[I].Length);
      Patterns[I].Pattern = CurrentPos;
   }
   
   /* Channel # scanner. This accurately determines the exact number of
      channels used in the song. This allows proper scaling to be applied.
      This also has the happy side effect of verifying the pattern data
      before playback begins. */
   memset(ChannelMask,0,sizeof(ChannelMask));
   int CurOrder = 0;
   unsigned char ChangeVol = 1;
   while (CurOrder < Header->OrderNum)
   {
      // Position Marker Pattern
      // End of song mark
      if (Orders[CurOrder] >= 254)
      {
	 CurOrder++;
	 continue;
      }
      
      // Be sure we are still in a safe range
      if ( (Orders[CurOrder] >= Header->PatNum))
	 break;
      
      unsigned char *DataPointer = Patterns[Orders[CurOrder]].Pattern;
      unsigned char *DataEnd = Patterns[Orders[CurOrder]].Pattern + Patterns[Orders[CurOrder]].Length;
      if (DataPointer == 0) 
      {
	 CurOrder++;
	 continue;
      }
      
      for (unsigned char Row = 0;Row != 64; Row++)
      {
	 // Decode each row
	 if (DataPointer >= DataEnd)
	 {
	    Free();
	    return LOADPART_Patterns | LOADFAIL_Corrupt | Orders[CurOrder];
	 }
	 
	 while (DataPointer && *DataPointer)
	 {
	    unsigned char Flags = *DataPointer;
	    unsigned char Channel = Flags & 31;
	    
	    DataPointer++;      // We have decoded the flags byte, move on
	    
	    if ((Flags & 32) == 32)
	       DataPointer += 2;
	    if ((Flags & 64) == 64)
	       DataPointer++;
	    if ((Flags & 128) == 128)
	    {
	       if (DataPointer + 1 >= DataEnd)
	       {
		  Free();
		  return LOADPART_Patterns | LOADFAIL_Corrupt | Orders[CurOrder];
	       }
	       
	       unsigned char Command = *(DataPointer++);
	       unsigned char Info = *(DataPointer++);
	       if (Command == 22)
		  if (Info != 0)
		     if (Info <= 0x40)
			ChangeVol = 0;
	    }
	    
	    if (((Header->ChannelF[Channel] & 128) == 0) &&
		((Header->ChannelF[Channel] & 127) < 16))
	       ChannelMask[Channel] = true;
	 }
	 
	 if (DataPointer != 0)
	    DataPointer++;
      }
      
      if ( DataPointer > DataEnd )
      {
	 Free();
	 return LOADPART_Patterns | LOADFAIL_Corrupt | Orders[CurOrder];
      }
      CurOrder++;
   }
   
   GlobalVol = Header->GlobalVolume;
   if (GlobalVol < 30 && ChangeVol == 1)
      GlobalVol = 64;
   return LOADOK;
}
									/*}}}*/
// S3MFormat::GetSongSamples - Return a list of the sample data		/*{{{*/
// ---------------------------------------------------------------------
/* This formats the internal sample data into something the mixer will 
   understand. This is also the same format the UI uses for display.*/
void museS3MFormat::GetSongSamples(SequenceSample &Samples)
{
   Samples.construct();

   if (Header == 0)
      return;

   Samples.reserve(Header->InstNum);
   unsigned int I = 0;
   for (; I < Header->InstNum; I++)
   {
      museSample *Sample = &Samples[I];
      Sample->Sample = Instruments[I].Sample;

      // Looping is enabled
      if ((Instruments[I].Inst.Flags & 1) == 1)
      {
         Sample->LoopBegin = Instruments[I].Inst.LoopBeg;
         Sample->LoopEnd = Instruments[I].Inst.LoopEnd;
         Sample->SampleEnd = Instruments[I].Inst.LoopEnd;
         Sample->Flags = 1 << 0;
      }
      else
      {
         Sample->LoopBegin = 0;
         Sample->LoopEnd = Instruments[I].Inst.Length;
         Sample->SampleEnd = Instruments[I].Inst.Length;
         Sample->Flags = 0;
      }
      Sample->Name = Instruments[I].Inst.Name;
      Sample->MaxNameLen = 28;
      Sample->SampleName = Instruments[I].Inst.FName;
      Sample->MaxSNameLen = 14;
      Sample->InstNo = I;
      Sample->Center = Instruments[I].Inst.C2Speed;
      Sample->Volume = Instruments[I].Inst.Volume*0xFF/64;
   }
}
									/*}}}*/
// S3MFormat::Keep - Clone important internal data			/*{{{*/
// ---------------------------------------------------------------------
/* After this function is called the playback routine will be able to
   execute without having the memory block passed to LoadMemModule 
   resident. However, it will be unable to download the samples which
   are not cloned by the routine. The routine was intended for use with
   wavetable devices that are capable of sample downloading (GUS) */
long museS3MFormat::Keep()
{
   if (Header == 0 || Private == true)
      return FAIL_NotLoaded;
   
   Private = true;
   
   // Clone the header
   S3MHeader *OldHeader = Header;
   Header = new S3MHeader;
   memcpy(Header,OldHeader,sizeof(*Header));
   
   // Clone the patterndata
   int I;
   for (I = 0; I != Header->PatNum; I++)
   {
      unsigned char *OldPat = Patterns[I].Pattern;
      Patterns[I].Pattern = new unsigned char[Patterns[I].Length];
      memcpy(Patterns[I].Pattern,OldPat,Patterns[I].Length);
   }   
      
   // Clone the orders
   octet *OldOrds = Orders;
   Orders = new octet[Header->OrderNum];
   memcpy(Orders,OldOrds,Header->OrderNum*sizeof(octet));

   return LOADOK;
}
									/*}}}*/
// S3MFormat::GetSongInfo - Returns information about the song		/*{{{*/
// ---------------------------------------------------------------------
/* Simply fills the info structure. */
long museS3MFormat::GetSongInfo(museSongInfo &Info)
{
   Info.Free();
   
   if (Header == 0)
      return FAIL_NotLoaded;
   
   Info.Patterns = Header->PatNum;
   Info.Orders = Header->OrderNum;

   // Ennumerate the # of channels
   Info.Channels = 0;
   for (unsigned int I = 0; I != 32; I++)
      if (ChannelMask[I] == true)
         Info.Channels++;

   Info.ClassName = ((museFormatClass *)GetMetaClass())->GetTypeName();
   Info.TypeName = _strdupcpp("Scream Tracker 3 Module");
   Info.Title = _strndupcpp(Header->Name,sizeof(Header->Name));

   return FAIL_None;
}
									/*}}}*/

// S3MFormatClass::museS3MFormatClass - MetaClass Constructor		/*{{{*/
// ---------------------------------------------------------------------
/* Sets the version number */
museS3MFormatClass::museS3MFormatClass()
{
   MajorVersion = 1;
   MinorVersion = 6;
}
									/*}}}*/
// S3MFormatClass::GetTypeName - Returns the General Name		/*{{{*/
// ---------------------------------------------------------------------
/* This returns the general typename before any specific name is known */
const char *museS3MFormatClass::GetTypeName()
{
   return "Scream Tracker 3 Module";
}
									/*}}}*/
// S3MFormatClass::GetClassForFile - Find the matching metaclass	/*{{{*/
// ---------------------------------------------------------------------
/* This simply checks if the file extension is S3M, returns this if
   it is. */
museFormatClass *museS3MFormatClass::GetClassForFile(const char *FName)
{
   const char *End = FName + strlen(FName);
   for(;*End != '.' && End != FName;End --);

   if (End != FName)
      End++;

   if (_stricmp(End,"S3M") != 0)
      return 0;

   return this;
}
									/*}}}*/
// S3MFormatClass::GetScanSize - Return the size of the s3m header	/*{{{*/
// ---------------------------------------------------------------------
/* This is used when a UI is going to prescan files for their titles. */
unsigned long museS3MFormatClass::GetScanSize()
{
   return sizeof(S3MHeader);
}
									/*}}}*/
// S3MFormatClass::Scan - Return basic information about the song	/*{{{*/
// ---------------------------------------------------------------------
/* This reads the song's header and extracts some basic information about
   the song. */
unsigned long museS3MFormatClass::Scan(octet *Region, museSongInfo &Info)
{
   Info.Free();

   S3MHeader &Head = *((S3MHeader *)Region);

   // Check the header for the Signature
   if (memcmp(Head.ID,"SCRM",4))
      return 1;

   Info.Title = _strndupcpp(Head.Name,sizeof(Head.Name));
   Info.TypeName = _strdupcpp("Scream Tracker 3 Module");
   Info.ClassName = GetTypeName();

   Info.Channels = 0;
   for (unsigned int I = 0; I != 32; I++)
   {
      if (((Head.ChannelF[I] & 128) == 0) &&
          ((Head.ChannelF[I] & 127) < 16))
         Info.Channels++;
   }

   Info.Patterns = Head.PatNum;
   Info.Orders = Head.OrderNum;

   return 0;
}
									/*}}}*/
