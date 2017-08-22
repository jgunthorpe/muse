// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   MODFormat - Muse file format to handle MOD files.
   
   ##################################################################### */
									/*}}}*/
// Includes								/*{{{*/
#include <muse.h>
#include <modform.h>

#include <math.h>

#include "modstruc.h"
   									/*}}}*/

#define SWAPWORD(A) ((((A) & 0x00FF) << 8) + (((A) & 0xFF00) >> 8))
#define IsAscii(x)(x >= 0x20 && x <= 0x7D)

museMODFormatClass *museMODFormat::Meta = new museMODFormatClass;
museWOWFormatClass *museWOWFormat::Meta = new museWOWFormatClass;

// ByteSwap - Swaps byte pairs on a short				/*{{{*/
// ---------------------------------------------------------------------
/* */
static void ByteSwap(unsigned short &Byte)
{
   unsigned short Temp = Byte >> 8;
   Byte = (Byte << 8) + Temp;
}
									/*}}}*/
// Round - Rounds pitch values.						/*{{{*/
// ---------------------------------------------------------------------
/* */
static unsigned long Round(long Period)
{
    float Frequency = 1712*16/Period;
    
    float Current = 8363*256;
    while (Current > Frequency) Current /= 2;
    while (Current < Frequency) Current *= 1.059463094;
//    if (Frequency - Current <= Current * 1.059463094 - Frequency) return (Current);
//        else return (Current * 1.059463094);
//    if (((long) Current) != Frequency) Current *= 1.059463094;
    return (long)((1712*16)/Current);
}
									/*}}}*/

// MODFormat::museMODFormat - Constructor 				/*{{{*/
// ---------------------------------------------------------------------
/* */
museMODFormat::museMODFormat()
{
   // Init the structures
   Header = 0;
   Patterns = 0;
   Samp15 = false;
   MODChans = 0;
   Samples = new unsigned char *[32];
   Private = false;
   memset(Samples,0,sizeof(unsigned char *)*32);
}
									/*}}}*/
// MODFormat::Free - Frees the module					/*{{{*/
// ---------------------------------------------------------------------
/* Dump the private copy of the pattern data and the header. */
void museMODFormat::Free()
{
   int x;

   if (Private)
       for (x = 0; x < PatNum; x++)
           delete [] Patterns[x].Pattern;
   
   delete [] Patterns;
   delete Header;

   Patterns = 0;
   Header = 0;
   MODChans = 0;
   Samp15 = false;
   Private = false;
   memset(Samples,0,sizeof(unsigned char *)*32);

   museMODGeneric::Free();
}
									/*}}}*/
// MODFormat::LoadMemModule - Load a module from a memory region.	/*{{{*/
// ---------------------------------------------------------------------
/* */
#include <iostream.h>
long museMODFormat::LoadMemModule(unsigned char *Region,unsigned long Size)
{
   Free();

   Private = false;
    
   Samp15 = false;
   unsigned char *CurrentPos = Region;
   unsigned char *End = Region + Size;

   Header = (MODHeader *)CurrentPos;
   CurrentPos += sizeof(*Header);
   if (CurrentPos > End)
   {
       Header = 0;
       return (LOADPART_Header | LOADFAIL_Truncated);
   }  

   // Check the header for the Signature
   MODChans = 0;
   unsigned int MODInsts = 31;
   if (!memcmp (Header->ID, "M.K.", 4)||!memcmp (Header->ID, "N.T.", 4))
   {
       MODChans = 4;
   }

   if (!memcmp (Header->ID, "CD81", 4)||!memcmp (Header->ID, "OCTA", 4))
   {
       MODChans = 8;
   }
   
   if (!memcmp (Header->ID+1,"CHN", 3)||!memcmp (Header->ID+1,"FLT", 3))
   {
       MODChans = (Header->ID[0] - '0');
   }
   
   if (!memcmp (Header->ID+2, "CH", 2))
   {
       MODChans = 10 * (Header->ID[0] - '0') + (Header->ID[1] - '0');
   }

   if (!memcmp (Header->ID, "WOW",  3))
   {
       MODChans = 8;
   }
   
   if (MODChans == 0)
   {
      MODChans = 4;
      Trace(TRACE_INFO,"In 15 samp code..");

      /* Scan the header to see if it makes any sense in terms of a 31 sample
         song */
      for (unsigned int I = 15; I < 31; I++)
      {
         if (SWAPWORD(Header->Samples[I].LoopLen) +
             SWAPWORD(Header->Samples[I].LoopBeg) -
             SWAPWORD(Header->Samples[I].SampleWords) > 4 ||
             Header->Samples[I].Volume > 64 ||
             Header->Samples[I].FineTune & 0xF0 != 0)
         {
            MODInsts = 15;
            break;
         }
      }

      // See if there are ascii letters in the 4char sig
      if (!(IsAscii(Header->ID[0]) && IsAscii(Header->ID[1]) &&
          IsAscii(Header->ID[2]) && IsAscii(Header->ID[3])))
         MODInsts = 15;
   }

   // Nope, must be 15
   if (MODInsts == 15)
   {
      Trace(TRACE_INFO,"Reading 15 smp header");
      MODChans = 4;
      MODInsts = 15;
      Samp15 = true;
      Header = new MODHeader;
      memset(Header,0,sizeof(MODHeader));
      CurrentPos = Region;
      memcpy(Header,CurrentPos,15*30+20);
      memcpy(&Header->OrderNum,CurrentPos + 15*30 + 20,134);
      CurrentPos += sizeof(*Header) - 16 * 30 - 4;
   }
   else
   {
      Trace(TRACE_INFO,"Loading normal header");

      // Since we change values in the header we must have a private copy
      Header = new MODHeader;
      memcpy(Header,Region,sizeof(MODHeader));
   }

   if (MODChans > 32 || MODChans < 4)
   {
      Free();
      return (LOADPART_Header | LOADFAIL_Corrupt);
   }

   // Count the number of patterns.
   PatNum = 0;
   unsigned int I;
   for (I = 0; I != 128; I++)
      if (Header->Patterns[I] > PatNum)
         PatNum = Header->Patterns[I];
   PatNum++;

   // Read the pattern data
   Patterns = new MODBoundPattern[PatNum];
   memset(Patterns,0,sizeof(MODBoundPattern)*PatNum);
   for (I = 0; I != PatNum; I++)
   {
      Patterns[I].Length = 256 * MODChans;

      if (CurrentPos + Patterns[I].Length > End)
      {
	 Warn("Hopelessly Corrupted");
	 Free();
	 return LOADPART_Patterns | LOADFAIL_Truncated | I;
      }
      
      Patterns[I].Pattern = CurrentPos;
      CurrentPos += Patterns[I].Length;
   }

   // Read the sample data (31 should be 15 for 15 smp mods)
   for (I = 0; I != 31; I++)
   {
      if (I < MODInsts)
      {
         Samples[I] = CurrentPos;

         // Corrupted
         if (CurrentPos >= End)
         {
            Header->Samples[I].SampleWords = 0;
            Header->Samples[I].LoopBeg = 0;
            Header->Samples[I].LoopLen = 0;
            Samples[I] = 0;
            continue;
         }

         // Byte Swap
         ByteSwap(Header->Samples[I].SampleWords);
         ByteSwap(Header->Samples[I].LoopBeg);
         ByteSwap(Header->Samples[I].LoopLen);

         CurrentPos += Header->Samples[I].SampleWords * 2;
         if (Header->Samples[I].SampleWords >= 1) Header->Samples[I].SampleWords--;
         if (Header->Samples[I].LoopBeg >= 1) Header->Samples[I].LoopBeg--;
         if (Header->Samples[I].LoopLen >= 1) Header->Samples[I].LoopLen--;

         if (Header->Samples[I].LoopBeg > Header->Samples[I].SampleWords)
         {
            Header->Samples[I].LoopBeg = 0;
            Header->Samples[I].LoopLen = 0;
         }

         // Corrupted
         if (CurrentPos >= End)
         {
            if (Header->Samples[I].SampleWords <= (CurrentPos - Region - Size)/2 + 1)
            {
               Header->Samples[I].SampleWords = 0;
               Header->Samples[I].LoopBeg = 0;
               Header->Samples[I].LoopLen = 0;
               Samples[I] = 0;
               continue;
            }
            else
               Header->Samples[I].SampleWords -= (CurrentPos - Region - Size)/2 + 1;
         }
      }
      else
         Samples[I] = 0;
   }
   
   return LOADOK;
}
									/*}}}*/
// MODFormat::GetRowElements - Return the info about a single row.	/*{{{*/
// ---------------------------------------------------------------------
/* This supports the strange FLT8 format which is 8 channels, the first 4 
   are in 1 pattern and the next 4 are in the following pattern. Don't have
   a song like this so I cannot test it. */
long museMODFormat::GetRowElements(SequenceMODElement *Elements,
				   unsigned long Row,unsigned long Pattern)
{
   if (Header == 0 || Row >= 64)
      return 1;
   
   // FLT8 paired structure
   if (!memcmp (Header->ID, "FLT8", 4))
   {
      // Go through each channel
      unsigned char *CurrentByte = Patterns[Pattern].Pattern + 4*4*Row;
      unsigned int I;
      for (I = 0; I != 4; I++, CurrentByte += 4)
      {
         (*Elements)[I].Period = (CurrentByte[1] + 256 * (CurrentByte[0] & 15)) * 16;
         (*Elements)[I].Sample = (CurrentByte[0] & ~15) + (CurrentByte[2] >> 4);
         (*Elements)[I].Effect = CurrentByte[2] & 15;
         (*Elements)[I].Volume = 0xFF;
         (*Elements)[I].EffectParam = (unsigned char) CurrentByte[3];
      };

      CurrentByte = Patterns[Pattern + 1].Pattern + 4*4*Row;
      for (; I != 8; I++, CurrentByte += 4)
      {
         (*Elements)[I].Period = (CurrentByte[1] + 256*(CurrentByte[0] & 15))*16;
         (*Elements)[I].Sample = (CurrentByte[0] & ~15) + (CurrentByte[2] >> 4);
         (*Elements)[I].Effect = CurrentByte[2] & 15;
         (*Elements)[I].Volume = 0xFF;
         (*Elements)[I].EffectParam = (unsigned char) CurrentByte[3];
	 Round((*Elements)[I].Period);      // Kill Compiler Warning
      };
   }
   else
   {
      // Go through each channel
      unsigned char *CurrentByte = Patterns[Pattern].Pattern + 4*MODChans*Row;
      for (unsigned int I = 0; I != MODChans; I++, CurrentByte += 4)
      {
         (*Elements)[I].Period = (CurrentByte[1] + 256 * (CurrentByte[0] & 15)) * 16;
         (*Elements)[I].Sample = (CurrentByte[0] & ~15) + (CurrentByte[2] >> 4);
         (*Elements)[I].Effect = CurrentByte[2] & 15;
         (*Elements)[I].Volume = 0xFF;
         (*Elements)[I].EffectParam = (unsigned char) CurrentByte[3];
      };
   }

   return 0;
}
									/*}}}*/
// MODFormat::GetOrderList - Return the list of orders for the song	/*{{{*/
// ---------------------------------------------------------------------
/* Called by mod generic to get the songs order list */
void museMODFormat::GetOrderList(unsigned char **List,unsigned long *Count)
{
   if (Header == 0)
      return;

   *List = &Header->Patterns[0];
   *Count = Header->OrderNum;
}
									/*}}}*/
// MODFormat::GetInitValue - Return the initial conditions for the song	/*{{{*/
// ---------------------------------------------------------------------
/* In mod these are fixed */
octet museMODFormat::GetInitValues(unsigned char *Speed, unsigned char *Tempo, unsigned char *GlobalVol)
{
   *Speed = 6;
   *Tempo = 125;
   *GlobalVol = 64;
   return 1;
}
									/*}}}*/
// MODFormat::GetMODPan - Return the pan values for the song.		/*{{{*/
// ---------------------------------------------------------------------
/* Notice we use the non-traditional GUSian pan values. */
void museMODFormat::GetMODPan(unsigned char *Pan)
{
   // Set default pan positions
   for (unsigned int I = 0; I != MODChans; I++)
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
// MODFormat::GetSongSamples - Return the sample description 		/*{{{*/
// ---------------------------------------------------------------------
/* This just decodes the sample description performing extra range checks
   and such. */
void museMODFormat::GetSongSamples(SequenceSample &Samps)
{
   if (Header == 0)
      return;

   Samps.construct();
   Samps.reserve(31);
   for (unsigned int I = 0; I < 31; I++)
   {
      museSample &Samp = Samps[I];
      
      if ((Header->Samples[I].SampleWords) * 2 == 0)
         Samp.Sample = 0;
      else
         Samp.Sample = Samples[I];
      
      // Looping is enabled
      if (Header->Samples[I].LoopLen > 1)
      {
         Samp.LoopBegin = Header->Samples[I].LoopBeg * 2;
         Samp.LoopEnd = (Header->Samples[I].LoopLen + Header->Samples[I].LoopBeg)*2 + 2;
         Samp.SampleEnd = (Header->Samples[I].SampleWords) * 2 + 2;

         if (Samp.LoopEnd > Samp.SampleEnd)
            Samp.LoopEnd = Samp.SampleEnd;
         Samp.Flags = INST_Loop | INST_Signed;
      }
      else
      {
         Samp.LoopBegin = 0;
         Samp.LoopEnd = Header->Samples[I].SampleWords * 2 + 2;
         Samp.SampleEnd = Header->Samples[I].SampleWords * 2 + 2;
         Samp.Flags = INST_Signed;
      }

      Samp.Name = Header->Samples[I].FName;
      Samp.MaxNameLen = 21;
      Samp.SampleName = 0;
      Samp.MaxSNameLen = 0;
      Samp.InstNo = I;
      
      double LocalFT;
      LocalFT = Header->Samples[I].FineTune;
      Samp.FineTune = Header->Samples[I].FineTune;

      if (LocalFT > 7)
         LocalFT -= 16;
      
      Samp.Center = (int)(14317056/4/(428 * (1 - LocalFT * 0.0037164 * 2)));
      Samp.Volume = Header->Samples[I].Volume*0xFF/64;
   }
}
									/*}}}*/
// MODFormat::Keep - Make a private copy of the pattern data		/*{{{*/
// ---------------------------------------------------------------------
/* This simply clones the pattern data */
long museMODFormat::Keep()
{
   if (Header == 0 || Private == true)
      return FAIL_NotLoaded;
   
   Private = true;
   
   // Clone the pattern data
   int I;
   for (I = 0; I != PatNum; I++)
   {
      unsigned char *OldPat = Patterns[I].Pattern;
      Patterns[I].Pattern = new unsigned char[Patterns[I].Length];
      memcpy(Patterns[I].Pattern,OldPat,Patterns[I].Length);
   }   
      
   return LOADOK;
}
									/*}}}*/
// MODFormat::GetSongInfo - Returns information about the song		/*{{{*/
// ---------------------------------------------------------------------
/* Simply fills the info structure. */
long museMODFormat::GetSongInfo(museSongInfo &Info)
{
   Info.Free();
   
   if (Header == 0)
      return FAIL_NotLoaded;
   
   Info.Patterns = PatNum;
   Info.Orders = Header->OrderNum;

   // Ennumerate the # of channels
   Info.Channels = MODChans;

   Info.ClassName = ((museFormatClass *)GetMetaClass())->GetTypeName();
   Info.Title = _strndupcpp(Header->Name,sizeof(Header->Name));

   Info.TypeName = _strdupcpp("Amiga Module");
   
   // Determine the TypeName
   char *C = 0;
   if (!memcmp (Header->ID, "M.K.", 4))
      C = "ProTracker Module";
   if (!memcmp (Header->ID, "N.T.", 4))
      C = "NewTracker Module";
   if (!memcmp (Header->ID+2, "CH", 2))
      C = "%uChn FastTracker II";
   if (!memcmp (Header->ID+1,"CHN", 3))
      C = "%uChn FastTracker Module";
   if (!memcmp (Header->ID,  "FLT", 3))
      C = "%uChn StarTrekker Module";
   if (!memcmp (Header->ID, "CD81", 4))
      C = "Atari Octalyser";
   if (!memcmp (Header->ID, "OCTA", 4))
      C = "Unknown Type Name (8 chan)";
   if (!memcmp (Header->ID, "WOW",  3))
      C = "WOW Module";

   if (Samp15 == true)
      C = "15 Sample Amiga Module";

   if (C == 0)
   {
      C = "31 Sample Module (%c%c%c%c)";
      Info.TypeName = new char[strlen(C)+50];
      sprintf(Info.TypeName,C,Header->ID[0],Header->ID[1],Header->ID[2],
	      Header->ID[3]);
   }
   else
   {
      Info.TypeName = new char[strlen(C)+50];
      sprintf(Info.TypeName,C,(unsigned long)MODChans);
   }

   return FAIL_None;
}
									/*}}}*/

// MODFormatClass::museMODFormatClass - Constructor			/*{{{*/
// ---------------------------------------------------------------------
/* Sets the version numbers */
museMODFormatClass::museMODFormatClass()
{
   MajorVersion = 1;
   MinorVersion = 2;
}
									/*}}}*/
// MODFormatClass::GetTypeName - Returns the Base type name		/*{{{*/
// ---------------------------------------------------------------------
/* Returns Amiga Module */
const char *museMODFormatClass::GetTypeName()
{
   return "Amiga Module";
}
									/*}}}*/
// MODFormatClass::GetClassForFile - Find the metaclass for the fname	/*{{{*/
// ---------------------------------------------------------------------
/* mod.*, *.nst and *.mod are all cosidered to be mod files. */
museFormatClass *museMODFormatClass::GetClassForFile(const char *FName)
{
   const char *End = FName + strlen(FName);
   for(;*End != '.' && End != FName;End --);
   
   if (End != FName)
      End++;

   const char *Start;
   for(Start = End;*Start != ':' && *Start != '/' && *Start != '\\' && Start != FName; Start--);
   if (Start != FName)
      Start++;
   
   while (1)
   {
      if (_stricmp(End,"MOD") == 0)
         break;
      if (_stricmp(End,"NST") == 0)
         break;
      if (_strnicmp("MOD.",Start,4) == 0)
         break;
      return 0;
   }
   return this;
}
									/*}}}*/
// MODFormatClass::GetScanSize - Returns the header size		/*{{{*/
// ---------------------------------------------------------------------
/* Returns the header size for file scanning */
unsigned long museMODFormatClass::GetScanSize()
{
   return sizeof(MODHeader);
}
									/*}}}*/
// MODFormatClass::Scan - Scans the header of a file.			/*{{{*/
// ---------------------------------------------------------------------
/* This checks the header signature, song title and pattern/order count. */
unsigned long museMODFormatClass::Scan(octet *Region, museSongInfo &Info)
{
   bool Samp15 = false;
   MODHeader *Header = (MODHeader *)Region;
   
   // Check the header for the Signature
   long MODChans = 0;
   unsigned int MODInsts = 31;
   if (!memcmp (Header->ID, "M.K.", 4)||!memcmp (Header->ID, "N.T.", 4))
       MODChans = 4;
   
   if (!memcmp (Header->ID, "CD81", 4)||!memcmp (Header->ID, "OCTA", 4))
       MODChans = 8;
   
   if (!memcmp (Header->ID+1,"CHN", 3)||!memcmp (Header->ID+1,"FLT", 3))
       MODChans = (Header->ID[0] - '0');
   
   if (!memcmp (Header->ID+2, "CH", 2))
       MODChans = 10 * (Header->ID[0] - '0') + (Header->ID[1] - '0');
   
   if (!memcmp (Header->ID, "WOW",  3))
       MODChans = 8;

   if (MODChans == 0)
   {
      MODChans = 4;
   
      /* Scan the header to see if it makes any sense in terms of a 31 sample
       song */
      for (unsigned int I = 15; I < 31; I++)
      {
	 if (SWAPWORD(Header->Samples[I].LoopLen) +
	     SWAPWORD(Header->Samples[I].LoopBeg) -
	     SWAPWORD(Header->Samples[I].SampleWords) > 4 ||
	     Header->Samples[I].Volume > 64 ||
	     Header->Samples[I].FineTune & 0xF0 != 0)
	 {
	    MODInsts = 15;
	    break;
	 }
      }
      
      // See if there are ascii letters in the 4char sig
      if (!(IsAscii(Header->ID[0]) && IsAscii(Header->ID[1]) &&
          IsAscii(Header->ID[2]) && IsAscii(Header->ID[3])))
         MODInsts = 15;
   }
   
   // Nope, must be 15
   if (MODInsts == 15)
   {
      MODChans = 4;
      MODInsts = 15;
      Samp15 = true;
      Header = new MODHeader;
      memset(Header,0,sizeof(MODHeader));
      memcpy(Header,Region,15*30+20);
      memcpy(&Header->OrderNum,Region + 15*30 + 20,134);
   }
   else
   {
      // Since we change values in the header we must have a private copy
      Header = new MODHeader;
      memcpy(Header,Region,sizeof(MODHeader));
   }

   if (MODChans > 32 || MODChans < 4)
   {
      delete Header;
      return 1;
   }

   // Count the number of patterns.
   Info.Patterns = 0;
   int I;
   for (I = 0; I != 128; I++)
      if (Header->Patterns[I] > Info.Patterns)
         Info.Patterns = Header->Patterns[I];
   Info.Patterns++;
   Info.Channels = MODChans;
   Info.Orders = Header->OrderNum;
   Info.ClassName = GetTypeName();

   char *C = 0;
   if (!memcmp (Header->ID, "M.K.", 4))
       C = "ProTracker Module";
   if (!memcmp (Header->ID, "N.T.", 4))
       C = "NewTracker Module";
   if (!memcmp (Header->ID+2, "CH", 2))
       C = "%uChn FastTracker II";
   if (!memcmp (Header->ID+1,"CHN", 3))
       C = "%uChn FastTracker Module";
   if (!memcmp (Header->ID,  "FLT", 3))
       C = "%uChn StarTrekker Module";
   if (!memcmp (Header->ID, "CD81", 4))
       C = "Atari Octalyser";
   if (!memcmp (Header->ID, "OCTA", 4))
       C = "Unknown Type Name (8 chan)";
   if (!memcmp (Header->ID, "WOW",  3))
       C = "WOW Module";

   if (Samp15 == true)
      C = "15 Sample Amiga Module";

   if (C == 0)
   {
      C = "31 Sample Module (%c%c%c%c)";
      Info.TypeName = new char[strlen(C)+10];
      sprintf(Info.TypeName,C,Header->ID[0],Header->ID[1],Header->ID[2],Header->ID[3]);
   }
   else
   {
      Info.TypeName = new char[strlen(C)+10];
      sprintf(Info.TypeName,C,(unsigned long)MODChans,Header->ID[0],Header->ID[1],Header->ID[2],Header->ID[3]);
   }

   Info.Title = _strndupcpp(Header->Name,sizeof(Header->Name));
   return 0;
}
									/*}}}*/

// WOWFormat::LoadMemModule - Loads the song from a memory region	/*{{{*/
// ---------------------------------------------------------------------
/* We rewrite the header so it has WOW for the tag and the mod loader
   will consider it a WOW file. */
long museWOWFormat::LoadMemModule(unsigned char *Region,unsigned long Size)
{
   MODHeader *Header = (MODHeader *)Region;
   if (!(Header->ID[0] == 'M' && Header->ID[1] == '.' &&
       Header->ID[2] == 'K' && Header->ID[3] == '.'))
   {
      return 1;
   }
   Header->ID[0] = 'W';
   Header->ID[1] = 'O';
   Header->ID[2] = 'W';
   return museMODFormat::LoadMemModule(Region,Size);
}
									/*}}}*/
// WOWFormatClass::GetTypeName - Return the name of a wow module	/*{{{*/
// ---------------------------------------------------------------------
/* */
const char *museWOWFormatClass::GetTypeName()
{
   return "WOW Module";
}
									/*}}}*/
// WOWFormatClass::GetClassForFile - Find the metaclass for the fname	/*{{{*/
// ---------------------------------------------------------------------
/* Matches *.wow */
museFormatClass *museWOWFormatClass::GetClassForFile(const char *FName)
{
   const char *End = FName + strlen(FName);
   for(;*End != '.' && End != FName;End --);

   if (End != FName)
      End++;

   while (1)
   {
      if (_stricmp(End,"WOW") == 0)
         break;
      return 0;
   }
   return this;
}
									/*}}}*/
// WOWFormatClass::Scan - Scans the header of a file.			/*{{{*/
// ---------------------------------------------------------------------
/* We rewrite the id tag to have WOW in it, the mod code will then 
   consider it a wow file. */
unsigned long museWOWFormatClass::Scan(octet *Region, museSongInfo &Info)
{
   museFormatClass::Scan(Region,Info);

   MODHeader *Header = (MODHeader *)Region;
   if (!(Header->ID[0] == 'M' && Header->ID[1] == '.' &&
       Header->ID[2] == 'K' && Header->ID[3] == '.'))
      return 1;

   Header->ID[0] = 'W';
   Header->ID[1] = 'O';
   Header->ID[2] = 'W';
   return museMODFormatClass::Scan(Region,Info);
}
									/*}}}*/
// WOWFormatClass::museWOWFormatClass - Constructor			/*{{{*/
// ---------------------------------------------------------------------
/* Sets the version numbers */
museWOWFormatClass::museWOWFormatClass() : museMODFormatClass(0)
{
   MajorVersion = 1;
   MinorVersion = 1;
}
									/*}}}*/
