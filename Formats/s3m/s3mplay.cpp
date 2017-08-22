// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   S3MFormat - Muse file format to handle S3M files.

   This bit contains the player portion. Once the song is loaded and
   verified the player decodes and converts it into Muse Pattern Data.
   This is done in one function because most of it is the two large
   switch statements.
   
   Check G00 and K and L more
   Check Porta OldNote
   3rd bit is 16 bit flag
   Check Multi-Retrig

   ##################################################################### */
									/*}}}*/
// Includes								/*{{{*/
#include <muse.h>
#include <s3mform.h>

#include "s3mstruc.h"
#include <stdlib.h>
#include <math.h>
   									/*}}}*/

// S3MFormat::Play - Plays the song				
// ---------------------------------------------------------------------
/* */
long museS3MFormat::Play(museOutputBase *Device,musePlayerControl *Control)
{
   const signed char RetrigAdd[16] = { 0,-1,-2,-4,-8,-16, 0,0, 0, 1, 2, 4, 8,16, 0, 0};
   const signed char RetrigMul[16] = {16,16,16,16,16, 16,10,8,16,16,16,16,16,16,24,32};
   const unsigned short FineTunes[16] = {7895,7941,7985,8046,8107,8169,8232,8280,8363,8413,8463,8529,8581,8651,8723,8757};
   
   if ((Orders == 0) || (Patterns == 0) || (Instruments == 0))
      return PLAYFAIL_NotLoaded;

   // Table of note periods. (Should be 12, but 17 prevents bad notes from crashing)
   unsigned int NoteTable[17] = {1712,1616,1524,1440,1356,1280,1208,1140,
                                 1076,1016,960, 907, 907, 907, 907, 907, 907};
   signed short VSineTable[32] = {0,24,49,74,97,120,141,161,180,197,212,224,
                                 235,244,250,253,255,253,250,244,235,224,
                                 212,197,180,161,141,120,97,74,49,24};
   signed short VSquare[64] = {+255,+255,+255,+255,+255,+255,+255,+255,+255,+255,+255,
                               +255,+255,+255,+255,+255,+255,+255,+255,+255,+255,+255,
                               +255,+255,+255,+255,+255,+255,+255,+255,+255,+255,
                               0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                               0,0,0,0,0,0,0,0,0,0};
   signed short VRamp[64] = {255,247,239,231,223,215,207,199,191,183,175,167,
                             159,151,143,135,127,119,111,103,95,87,
                             79,71,63,55,47,39,31,23,15,7,
                             -7,-15,-23,-31,-39,-47,-55,-63,-71,-79,
                             -87,-95,-103,-111,-119,-127,-135,-143,-151,-159,
                             -167,-175,-183,-191,-199,-207,-215,-223,-231,-239,-247,-255};
   signed short VRandom[64];

   CmdInfoS3M *CommandInfo = new CmdInfoS3M[32];

   unsigned int I;
   for (I = 0; I != 64; I++)
      VRandom[I] = (rand() % 510) - 255;

   signed short CurOrder = 0;
   unsigned char RowSkip = 0;
   unsigned char Play = 0;

   memset(CommandInfo,0,32*sizeof(CmdInfoS3M));
   for (I = 0; I != 32; I++)
   {
      CommandInfo[I].VibTable = VSineTable;
      CommandInfo[I].VibSize = 32;
      CommandInfo[I].TrmoTable = VSineTable;
      CommandInfo[I].TrmoSize = 32;
   }

   // Definintions of tracking variables
   unsigned short FramesARow = Header->InitialSpeed;
   unsigned long FrameLength = Device->SecondsToLen(60.0/(24.0*((float)Header->InitialTempo)));
   unsigned char GlobalVolume = GlobalVol;

   // Ennumerate the # of channels
   Sequencebool BoolSeq;
   BoolSeq.construct();
   BoolSeq.reserve(32);

   SequenceChannel ChanDat;
   ChanDat.construct();
   ChanDat[31].Flags = 0;
   for (I = 0; I != 32; I++)
   {
      ChanDat[I].Sample = 0xFFFF;
      ChanDat[I].ModuleChannel = I;
   }

   for (I = 0; I != 32; I++)
   {
      BoolSeq[I] = ChannelMask[I];
      if (BoolSeq[I] == true)
         ChanDat._length = I + 1;
   }
   Device->SetChannelMask(&BoolSeq);
   Device->SetVolumeScale(Header->MasterVolume & 127);
   BoolSeq.free();

   // Set default pan positions
   for (I = 0; I != ChanDat.size(); I++)
   {
      if ((Header->ChannelF[I] & 127) > 7)
         ChanDat[I].Pan = 0xC;
      else
         ChanDat[I].Pan = 0x3;
   }

   // Pan data is included, use it.
   if (Header->DefaultPan == 252)
      for (I = 0; I < ChanDat.size(); I++)
         if ((ChannelPan[I] & 32) == 32)
            ChanDat[I].Pan = ChannelPan[I] & 15;

   // Convert to more usable panning values
   for (I = 0; I < ChanDat.size(); I++)
      ChanDat[I].Pan = (2*ChanDat[I].Pan - 0xF)*(PanSpan/2)/0xF;

   // Load the samples into the mixer.
   unsigned short *Centre = new unsigned short[Header->InstNum];
   for (I = 0; I < Header->InstNum; I++)
      Centre[I] = Instruments[I].Inst.C2Speed;
   SequenceSample Samples;
   GetSongSamples(Samples);
   Device->LoadSamples(&Samples);
   Samples.free();

   Command Cmds[32];
   memset(Cmds,0xFF,32*sizeof(Command));

   unsigned char PatLoopRow = 0;
   unsigned char PatLoopCount = 0;
   unsigned long SkipOrds = 0;
   unsigned long SkipRows = 0;

   PlayedRec *OrdPlayList = new PlayedRec[Header->OrderNum];
   memset(OrdPlayList,0,Header->OrderNum*sizeof(PlayedRec));
   museFrameInfo FInf;
   FInf.GlobalVolume = GlobalVolume * VolMax / 64;
   FInf.Tempo = Header->InitialTempo;
   FInf.Speed = Header->InitialSpeed;
   FInf.SongTime = 0;
   while (CurOrder < Header->OrderNum)
   {
      // Smart Order Skipping
      if (SkipOrds != 0)
         SkipOrds--;

      FInf.Order = CurOrder;
      // End of song mark
      if (Orders[CurOrder] == 255)
         break;

      // Position Marker Pattern
      if (Orders[CurOrder] == 254)
      {
         CurOrder++;
         continue;
      };

      // Be sure we are still in a safe range
      if ((CurOrder >= Header->OrderNum) || (Orders[CurOrder] >=
           Header->PatNum))
         break;
      
      FInf.Pattern = Orders[CurOrder];

      /* Pattern Decoder
         This goes through each of the 64 rows in the pattern. From the
         ST3 tech docs:
           Unpacked pattern is always 32 channels by 64 rows. Below
           is the unpacked format st uses for reference:
             Unpacked Internal memoryformat for patterns (not used in files):
             NOTE: each channel takes 320 bytes, rows for each channel are
                   sequential, so one unpacked pattern takes 10K.
             byte 0 - Note; hi=oct, lo=note, 255=empty note,
                      254=key off (used with adlib, with samples stops smp)
             byte 1 - Instrument      ;0=..
             byte 2 - Volume          ;255=..
             byte 3 - Special command ;255=..
             byte 4 - Command info    ;

           Packed data consits of following entries:
           BYTE:what  0=end of row
                      &31=channel
                      &32=follows;  BYTE:note, BYTE:instrument
                      &64=follows;  BYTE:volume
                      &128=follows; BYTE:command, BYTE:info
              -- Called Flags
           So to unpack, first read one byte. If it's zero, this row is
           done (64 rows in entire pattern). If nonzero, the channel
           this entry belongs to is in BYTE AND 31. Then if bit 32
           is set, read NOTE and INSTRUMENT (2 bytes). Then if bit
           64 is set read VOLUME (1 byte). Then if bit 128 is set
           read COMMAND and INFO (2 bytes).
      */
      unsigned char *CurByte = Patterns[Orders[CurOrder]].Pattern;
      char Jump = 0;

      if (PatLoopCount == 0)
         PatLoopRow = 0;

      for (unsigned char Row = 0;Row != 64; Row++)
      {
         FInf.Row = Row;
         unsigned char RowRepeat = 1;
	 
         memset(Cmds,0xFF,32*sizeof(Command));
	 
         // Check that this row has never been played before
         if (AllowLoop == false && (CurOrder) < Header->OrderNum)
         {
	    if ((OrdPlayList[CurOrder].Rows[Row/32] &
		 (1 << (Row % 32))) != 0)
	    {
	       CurOrder = Header->OrderNum;
	       Trace(TRACE_INFO,"Aborting");
	       break;
	    }
         }
         
         // Mark the row as played.
         OrdPlayList[CurOrder].Rows[Row/32] |= (1 << (Row % 32));
         
         // Mark as played
         if (!((Play == 0) && (RowSkip != 0)))
         {
            // Decrease skip counter
            if (SkipOrds == 0 && SkipRows != 0)
               SkipRows--;

            // Get next skip target
            if (Control != 0 && SkipRows == 0 && SkipOrds == 0)
            {
               // Query the player control, maintains a queue of commands
               musePlayRec Rec;
               Control->Get(&Rec);
               long NRow = Rec.Row;
               long NOrd = Rec.Order;

               // Smart advance
               if (Rec.Type == PlayAdvance)
               {
                  // Negative advances are the same as Position jumps relative to here..
                  if (NRow < 0 || NOrd < 0)
                  {
                     Rec.Type = PlayJump;
                     if (NOrd == 0)
                       NRow += Row;
                     else
                       NRow = 0;
                     NOrd += CurOrder;
                  }
                  else
                  {
                     // Skip Ahead
                     SkipRows = NRow;
                     SkipOrds = NOrd;
                  }
               }

               // Just Jump
               if (Rec.Type == PlayJump)
               {
                  // Compute a new order point, considering negative row jumps
                  CurOrder = max(NOrd - 1 + NRow/64,-1);
                  RowSkip = NRow % 64;
                  if (NRow < 0)
                     RowSkip = 64 - RowSkip;

		  // Cut all the channels notes
		  museChannel *Iter;
                  for (Iter = ChanDat.begin(); Iter != ChanDat.end(); Iter++)
                  {
                     Iter->Flags &= ~CHAN_Retrig;
                     Iter->Flags |= CHAN_Cut;
                  }

                  // Jump to start, reset all vars
                  if ((CurOrder == -1) && (RowSkip == 0))
                  {
                     FramesARow = Header->InitialSpeed;
                     FrameLength = Device->SecondsToLen(60.0/(24.0*((float)Header->InitialTempo)));
                     GlobalVolume = GlobalVol;

                     memset(CommandInfo,0,32*sizeof(CmdInfoS3M));
                     for (I = 0; I != 32; I++)
                     {
                        CommandInfo[I].VibTable = VSineTable;
                        CommandInfo[I].VibSize = 32;
                        CommandInfo[I].TrmoTable = VSineTable;
                        CommandInfo[I].TrmoSize = 32;
                     }

                     for (Iter = ChanDat.begin(); Iter != ChanDat.end(); Iter++)
                        Iter->Sample = 0xFFFF;

                     // Set default pan positions
                     for (I = 0; I != ChanDat.size(); I++)
                     {
                        if ((Header->ChannelF[I] & 127) > 7)
                           ChanDat[I].Pan = 0xC;
                        else
                           ChanDat[I].Pan = 0x3;
                     }

                     // Pan data is included, use it.
                     if (Header->DefaultPan == 252)
                        for (I = 0; I < ChanDat.size(); I++)
                           if ((ChannelPan[I] & 32) == 32)
                              ChanDat[I].Pan = ChannelPan[I] & 15;

                     // Convert to more usable panning values
                     for (I = 0; I < ChanDat.size(); I++)
                        ChanDat[I].Pan = (2*ChanDat[I].Pan - 0xF)*(PanSpan/2)/0xF;

                     PatLoopRow = 0;
                     PatLoopCount = 0;
                     FInf.GlobalVolume = GlobalVolume * VolMax / 64;
                     FInf.Tempo = Header->InitialTempo;
                     FInf.Speed = Header->InitialSpeed;
                     FInf.SongTime = 0;
                  }
                  break;
               }
            }
         }

         // Decode each row
         while ((CurByte != 0) && (*CurByte != 0))
         {
            unsigned char Flags = *CurByte;
            unsigned char Channel = Flags & 31;
            unsigned char ChanFlag = Header->ChannelF[Channel];
            museChannel *Data = ChanDat.begin() + Channel;
            CmdInfoS3M *CData = &CommandInfo[Channel];
            unsigned long OldNote = 0;

            CurByte++;      // We have decoded the flags byte, move on

            if ((Play == 0) && (RowSkip != 0))
            {
               if ((Flags & 32) == 32)
                  CurByte += 2;
               if ((Flags & 64) == 64)
                  CurByte++;
               if ((Flags & 128) == 128)
                  CurByte += 2;

               // Continue skipping till this row is complete.
               continue;
            }

            // Note Change
            if ((Flags & 32) == 32)
            {
               unsigned char Note = *(CurByte++);   // Hi=Octave Lo=note
               unsigned char Inst = *(CurByte++);
	       unsigned char OldInst = Inst;
	       
               // Change instruments
               if (Inst != 0)
               {
                  Inst--;

                  // Set to the last inst, which is a null inst.
                  if (Inst >= Header->InstNum)
                     Inst = Header->InstNum;

                  Data->Sample = Inst;
               }

               S3MBoundInst &IData = Instruments[Data->Sample];

               // Release the note
               if ((Note == 254) || (Data->Sample == 0xFFFF) || (IData.Sample == 0) || (IData.Inst.C2Speed == 0))
               {
                  Data->Flags &= ~CHAN_Retrig;
                  Data->Flags |= CHAN_Cut;     // Disable Note
               }
               else
               {
                  Data->SampleOffset = 0;

                  /* St3 does a volume reset on porta targets. But it doesn't
		     if the inst is 0 */
		  if (OldInst != 0)
		  {
		     CData->MainVol = IData.Inst.Volume;
		     CData->BaseVolume = CData->MainVol;
		     Data->Flags |= CHAN_Volume;  // Change volume
		  }
		  
                  // This is not a null note, get Hz
                  if (Note != 255)
                  {
                     Data->Flags &= ~CHAN_Cut;
                     Data->Flags |= CHAN_Retrig;  // Start Note
                     CData->Period = (long)(((float)8363.0*16.0*((float)NoteTable[Note & 0xF])/pow(2,Note >> 4))/((float)Centre[Data->Sample]));

                     OldNote = CData->BasePeriod;
                     CData->BasePeriod = (long)CData->Period;
                     Data->Flags |= CHAN_Pitch;    // New Pitch
                     Data->Flags |= CHAN_Instrument; // Change instrument
                  }
               }
            }

            // Volume change
            if ((Flags & 64) == 64)
            {
               unsigned char Volume = *(CurByte++);
               CData->MainVol = Volume;
               CData->BaseVolume = CData->MainVol;
               Data->Flags |= CHAN_Volume;    // Change volume
            }

            // Cancel the effects of any commands like vibrato
            if (CData->Period != CData->BasePeriod)
            {
               CData->Period = CData->BasePeriod;
               Data->Flags |= CHAN_Pitch; // Change pitch
            }

            if (CData->MainVol != CData->BaseVolume)
            {
               CData->MainVol = CData->BaseVolume;
               Data->Flags |= CHAN_Volume; // Change volume
            }

	    // Command Byte
            if ((Flags & 128) == 128)
            {
               unsigned char Command = *(CurByte++);
               unsigned char Info = *(CurByte++);

	       if (!(((ChanFlag & 128) == 0) && ((ChanFlag & 127) < 16)))
                  Command = 0;

               switch (Command)
               {
                  case 0:
                     break;

                  // Speed Change (Command A)
                  case 1:
                     if (Info == 0)
                        break;

                     FramesARow = Info;
                     FInf.Speed = Info;
                     break;

                  // Jump to order (Command B)
                  case 2:
                     CurOrder = Info;
                     CurOrder--;
                     Row = 63;
                     Play = 1;
                     Jump = 1;
                     break;

                  // Skip Pattern to row (Command C)
                  case 3:
                     RowSkip = (Info & 0xF) + 10*(Info >> 4);
                     if ((RowSkip > 63) || ((Info & 0xF) > 9) || ((Info >> 16) > 9))
                        RowSkip = 63;
                     Row = 63;
                     Play = 1;
                     Jump = 1;
                     break;

                  // Portamento (Command G)
                  case 7:
                     if (OldNote != 0)
                     {
                        CData->PortTarget = CData->Period;
                        CData->Period= OldNote;
                        CData->BasePeriod = CData->Period;
                        Data->Flags &= ~CHAN_Retrig;   // Do not start note
                     } 
		     else 
  		     {
//                        CData->PortTarget = CData->Period;
                     }

                     if (Info != 0)
                        CData->GInfo = Info;

                     Cmds[Channel].Command = Command;
                     break;

                  // Vibrato (Command H & U)
                  case 21:
                  case 8:
                     if ((Info >> 4) != 0)
                     {
                        CData->VibratoSpeed = Info >> 4;
                        CData->VibratoPoint = 0;
                     }

                     // Store last vibrato info
                     if (Info != 0)
                        CData->HInfo = Info;

                     Cmds[Channel].Command = Command;
                     break;

                  // Dual Command H00 and Dxy (Command K)
                  case 11:
                     Cmds[Channel].Command = 254;
                     Cmds[Channel].Command2 = 8;
                     break;

                  // Dual Command G00 and Dxy (Command L)
                  case 12:
                     Cmds[Channel].Command = 254;
                     Cmds[Channel].Command2 = 7;

                     if (OldNote != 0)
                     {
                        CData->PortTarget = CData->Period;
                        CData->Period= OldNote;
                        CData->BasePeriod = CData->Period;
                        Data->Flags &= ~CHAN_Retrig;   // Do not start note
                     } else {
                         CData->PortTarget = CData->Period;
                     }
                     break;

                  // Sample Offset (Command O)
                  case 15:
                     // Only works if a note has been started.
                     if ((Flags & 32) == 32)
                     {
                        if ((Instruments[Data->Sample].Sample == 0) ||
                           (Instruments[Data->Sample].Inst.Length == 0))
                           break;

                        Data->SampleOffset =  ((Info << 8) %
                                       (Instruments[Data->Sample].Inst.Length));
                     }
                     break;

                  // Tremolo (Command R)
                  case 18:
                     if ((Info >> 4) != 0)
                     {
                        CData->TremoloSpeed = Info >> 4;
                        CData->TremoloPoint = 0;
                     }

                     Cmds[Channel].Command = Command;
                     break;

                  // Command Group S
                  case 19:
                     switch (Info >> 4)
                     {
                        // Set fine tune
                        case 2:
                           Centre[Data->Sample] = FineTunes[Info & 0x0F];
                           break;

                        // Set vibrato waveform
                        case 3:
                           if ((Info & 0xF) == 0)
                           {
                              CData->VibTable = VSineTable;
                              CData->VibSize = 32;
                           }

                           if ((Info & 0xF) == 1)
                           {
                              CData->VibTable = VRamp;
                              CData->VibSize = 64;
                           }

                           if ((Info & 0xF) == 2)
                           {
                              CData->VibTable = VSquare;
                              CData->VibSize = 64;
                           }

                           if ((Info & 0xF) == 3)
                           {
                              CData->VibTable = VRandom;
                              CData->VibSize = 64;
                           }
                           break;

                        // Set tremolo waveform
                        case 4:
                           if ((Info & 0xF) == 0)
                           {
                              CData->TrmoTable = VSineTable;
                              CData->TrmoSize = 32;
                           }

                           if ((Info & 0xF) == 1)
                           {
                              CData->TrmoTable = VRamp;
                              CData->TrmoSize = 64;
                           }

                           if ((Info & 0xF) == 2)
                           {
                              CData->TrmoTable = VSquare;
                              CData->TrmoSize = 64;
                           }

                           if ((Info & 0xF) == 3)
                           {
                              CData->TrmoTable = VRandom;
                              CData->TrmoSize = 64;
                           }
                           break;

                        // Set pan position
                        case 8:
                           Data->Pan = Info & 0x0F;
                           Data->Pan = (2*Data->Pan - 0xF)*(PanSpan/2)/0xF;
                           Data->Flags |= CHAN_Pan;
                           break;

                        // Pattern Loop
                        case 0xB:
                           if ((Info & 0x0F) == 0)
                           {
                              PatLoopRow = Row;
                              break;
                           }

                           // Finished the loop
                           if (PatLoopCount == 1)
                           {
                              PatLoopCount = 0;
                              break;
                           }

                           if (PatLoopCount == 0)
                              PatLoopCount = Info & 0x0F;
                           else
                              PatLoopCount--;
                           RowSkip = PatLoopRow;
                           Row = 63;
                           Play = 1;
                           CurOrder--;
                           break;

                        // Note Cut in X frames
                        case 0xC:
                           CData->RowDelay = (Info & 0X0F);
                           CData->DelayFlags = (Data->Flags & ~CHAN_Retrig) | CHAN_Cut;     // Disable Note
                           break;

                        // Note Delay for X frames
                        case 0xD:
                           CData->RowDelay = (Info & 0X0F);
                           CData->DelayFlags = Data->Flags;
                           Data->Flags = 0;
                           break;

                        // Row Repeat
                        case 0xE:
                           RowRepeat = (Info & 0x0F) + 1;
                           break;
                     }
                     break;

                  // Tempo change (Command T)
                  case 20:
                     if (Info == 0 || Info <= 0x20)
                        break;

                     FrameLength = Device->SecondsToLen(60/(24*((float)Info)));
                     FInf.Tempo = Info;
                     break;

                  // Change Global Volume (Command V)
                  case 22:
                     if (Info <= 0x40)
                     {
                        GlobalVolume = Info;
                        FInf.GlobalVolume = Info * VolMax / 64;
                     /* Really should be in here, but st3 doesn't so we won't
                        for (unsigned char Channel = 0; Channel != 32;)
                           ChanDat[Channel].Flags |= 1 << 1;*/
                     }
                     break;

                  // DMP Panning (Command X)
                  case 24:
                     if (Info == 0xA4)
                        Data->Pan = PanSurround;
                     else
                     {
                        Data->Pan = min(128,Info);
                        Data->Pan = ((Data->Pan - 64)*PanSpan)/128;
                     }
                     Data->Flags |= CHAN_Pan;
                     break;

                  // Pix Play Panning (Command Z)
                  case 26:
                     Data->Pan = Info & 0x0F;
                     Data->Pan = (2*Data->Pan - 0xF)*(PanSpan/2/0xF);
                     Data->Flags |= CHAN_Pan;
                     break;

                  default:
                     Cmds[Channel].Command = Command;
                     break;
               };

               // Store the command info
               if (Info != 0)
                  CData->Info = Info;
            }
         }

         if (CurByte != 0)
            CurByte++;
         if (RowSkip != 0)
         {
            if (Play == 0)
            {
               RowSkip--;
               continue;
            }
            else
               Play = 0;
         }
         else
            Play = 1;

         for (;RowRepeat != 0; RowRepeat--)
         {
            for (I = 0; I != FramesARow; I++)
            {
               FInf.Frame = I;
                
               char FlipFlop = 0;
               for (unsigned char Channel = 0; Channel < ChanDat.size();)
               {
                  museChannel *Data = &ChanDat[Channel];
                  CmdInfoS3M *CData = &CommandInfo[Channel];
                  unsigned char Info;
                  unsigned char Command;

                  Info = CData->Info;

                  // Only happens in H/G dual so no need to flip info
                  if (FlipFlop == 0)
                     Command = Cmds[Channel].Command;
                  else
                     Command = Cmds[Channel].Command2;

                  switch (Command)
                  {
                     // Volume Slides (Command D)
                     case 254:          // Dual command slide
                     case 4:
                     {
                        unsigned char U = Info >> 4;
                        unsigned char L = Info & 0xF;

                        // Slide up
                        if (L == 0)
                        {
                           if ((I == 0) && ((Header->Flags & 64) == 0)) break;
                           CData->MainVol += U;
                           CData->BaseVolume = CData->MainVol;
                           Data->Flags |= CHAN_Volume;    // Change volume
                           break;
                        }

                        if (!((L == 0xF && U != 0) || U == 0xF))
                        {
                           if ((I == 0) && ((Header->Flags & 64) == 0)) break;
                           CData->MainVol -= Info & 0xF;
                           CData->BaseVolume = CData->MainVol;
                           Data->Flags |= CHAN_Volume;    // Change volume
                           break;
                        }

                        // Skip fine slides
                        if (I != 0) break;
                        if (Command == 254) break;

                        // Fine Volume up
                        if ((Info & 0x0F) == 0x0F)
                        {
                           CData->MainVol += Info >> 4;
                           CData->BaseVolume = CData->MainVol;
                           Data->Flags |= CHAN_Volume;    // Change volume
                           break;
                        }

                        // Fine Volume down
                        if ((Info & 0xF0) == 0xF0)
                        {
                           CData->MainVol -= Info & 0xF;
                           CData->BaseVolume = CData->MainVol;
                           Data->Flags |= CHAN_Volume;    // Change volume
                           break;
                        }
                        break;
                     }

                     // Pitch slide down (Command E)
                     case 5:
 		        // Fine Slide Down
                        if ((Info & 0xF0) == 0xF0)
                        {
                           if (I != 0) break;
                           CData->Period += (Info & 0xF) * 4;
                           Data->Flags |= CHAN_Pitch;    // New Pitch
                           CData->BasePeriod = CData->Period;
                           break;
                        }

                        // Extra Fine Slide Down
                        if ((Info & 0xE0) == 0xE0)
                        {
                           if (I != 0) break;
                           CData->Period += (Info & 0xF);
                           Data->Flags |= CHAN_Pitch;    // New Pitch
                           CData->BasePeriod = CData->Period;
                           break;
                        }

                        // Slide Down
                        if (I == 0) break;
 
                        CData->Period += Info * 4;
                        Data->Flags |= CHAN_Pitch;    // New Pitch
                        CData->BasePeriod = CData->Period;
                        break;

                     // Pitch slide up (Command F)
                     case 6:
                        // Fine Slide Up
                        if ((Info & 0xF0) == 0xF0)
                        {
                           if (I != 0) break;
                           CData->Period -= (Info & 0xF) * 4;
                           Data->Flags |= CHAN_Pitch;    // New Pitch
                           CData->BasePeriod = CData->Period;
                           break;
                        }

                        // Extra Fine Slide Up
                        if ((Info & 0xE0) == 0xE0)
                        {
                           if (I != 0) break;

                           CData->Period -= (Info & 0xF);
                           Data->Flags |= CHAN_Pitch;    // New Pitch
                           CData->BasePeriod = CData->Period;
                           break;
                        }

                        // Slide Up
                        if (I == 0) break;

                        CData->Period -= Info * 4;
                        Data->Flags |= CHAN_Pitch;    // New Pitch
                        CData->BasePeriod = CData->Period;
                        break;

                     // Portamento (Command G)
                     case 7:
                        if (CData->Period == CData->PortTarget)
                           break;

                        if (I == 0) break;

                        if (CData->Period > CData->PortTarget)
                        {
                           CData->Period -= CData->GInfo * 4;
                           if (CData->Period < CData->PortTarget)
                              CData->Period = CData->PortTarget;
                        }

                        if (CData->Period < CData->PortTarget)
                        {
                           CData->Period += CData->GInfo * 4;
                           if (CData->Period > CData->PortTarget)
                              CData->Period = CData->PortTarget;
                        }

                        Data->Flags |= CHAN_Pitch;    // New Pitch
                        CData->BasePeriod = CData->Period;
                        break;

                     // Vibrato (Command H & U)
                     case 21:
                     case 8:
                     {
                        if (CData->VibratoPoint == 0 && I == 0) break;

                        signed long P;

                        // Compute the correct table index
                        if (CData->VibSize > 32)
                           P = CData->VibTable[CData->VibratoPoint % CData->VibSize];
                        else
                        {
                           unsigned char Point = CData->VibratoPoint % (2*CData->VibSize);
                           if (Point >= CData->VibSize)
                              P = (-1)*CData->VibTable[Point - CData->VibSize];
                           else
                              P = CData->VibTable[Point];
                        }

                        // Scale the value
                        P *= (CData->HInfo & 0xF);
                        if (Command == 21)
                           P /= 128;
                        else
                        {
                           P /= 16;
                           if ((Header->Flags & 1) == 0)
                              P /= 2;
                        }
                        CData->Period = CData->BasePeriod + P;
                        Data->Flags |= CHAN_Pitch;    // New Pitch

                        // Reset pitch, but do not advance
                        if (I != 0)
                           CData->VibratoPoint += CData->VibratoSpeed;
                        break;
                     }

                     // Tremor (Command I)
                     case 9:
                        if (CData->TremorCounter != 0) CData->TremorCounter--;

                        if (CData->TremorCounter == 0)
                        {
                           if (CData->TremorFlipFlop == 0)
                           {
                              CData->TremorFlipFlop = 1;
                              CData->TremorCounter = (Info & 0xF);
                              CData->MainVol = 0;
                              Data->Flags |= CHAN_Volume;   // New Volume
                           }
                           else
                           {
                              CData->TremorFlipFlop = 0;
                              CData->TremorCounter = Info >> 4;
                              CData->MainVol = CData->BaseVolume;
                              Data->Flags |= CHAN_Volume;   // New Volume
                           }
                        }
                        break;

                     // Arpeggio (Command J)
                     case 10:
                        if (Info != 0)
                        {
                           switch (((long)CData->ArpeggioCounter) % 3)
                           {
                              case 0:
                                 CData->Period = CData->BasePeriod;
                                 break;
                              case 1:
                                 CData->Period = (long)(CData->BasePeriod / pow ( 1.059463094, Info & 15 ));
                                 break;
                              case 2:
                                 CData->Period = (long)(CData->BasePeriod / pow ( 1.059463094, Info >> 4 ));
                                 break;
                           }

                           CData->ArpeggioCounter += ((float)FrameLength)/((float)Device->SecondsToLen(1/50.0));

                           Data->Flags |= CHAN_Pitch;    // Change pitch
                           break;
                         }

                     // Retrig (Command Q)
                     case 17:
                        if (CData->RetrigCounter != 0) CData->RetrigCounter--;

                        if (CData->RetrigCounter == 0)
                        {
                           CData->RetrigCounter = Info & 0xF;
                           CData->MainVol *= RetrigMul[Info >> 4];
                           CData->MainVol /= 16;
                           CData->MainVol += RetrigAdd[Info >> 4];
                           boundv(0,CData->MainVol,64);
                           CData->BaseVolume = CData->MainVol;
                           Data->Flags |= CHAN_Volume; // New Volume
                           Data->Flags &= ~CHAN_Cut;
                           Data->Flags |= CHAN_Retrig;  // Start Note
                           Data->SampleOffset = 0;
                        }
                        break;

                     // Tremolo (Command R)
                     case 18:
                     {
                        signed short I;

                        // Compute the correct table index
                        if (CData->TrmoSize > 32)
                           I = CData->TrmoTable[CData->TremoloPoint % CData->TrmoSize];
                        else
                        {
                           unsigned char Point = CData->TremoloPoint % (2*CData->TrmoSize);
                           if (Point >= CData->TrmoSize)
                              I = (-1)*CData->TrmoTable[Point - CData->TrmoSize];
                           else
                              I = CData->TrmoTable[Point];
                        }

                        // Scale the value
                        I *= Info & 0xF;
                        I /= 128;

                        CData->MainVol = CData->BaseVolume + I;

                        if ( I != 0 ) CData->TremoloPoint += CData->TremoloSpeed;

                        Data->Flags |= CHAN_Volume;   // New Volume
                        break;
                     }

                     default:
                        break;
                  }

                  // Notedelay and note stop
                  if ((CData->RowDelay != 0) && (CData->RowDelay == I))
                  {
                     Data->Flags |= CData->DelayFlags;
                     CData->RowDelay = 0;
                  }

                  if ((Cmds[Channel].Command2 != 0xFF) && (FlipFlop == 0))
                  {
                     FlipFlop = 1;
                     continue;
                  }
                  else
                  {
                     FlipFlop = 0;
                     Channel++;
                  }

                  if (Data->Flags == 0) continue;
                  if (SkipOrds != 0 || SkipRows != 0) continue;

                  // Fix pitch
                  if ((Data->Flags & CHAN_Pitch) != 0)
                  {
                     CData->Period = min(0x7FFF,CData->Period);
                     CData->Period = max(64,CData->Period);
                     Data->Pitch = 14317056/CData->Period;
                  }

                  // Fix Volume
                  if ((Data->Flags & CHAN_Volume) != 0)
                  {
                     Data->MainVol = bound(0,CData->MainVol,64);
                     Data->MainVol = (Data->MainVol*VolMax*GlobalVolume)/64/64;
                     CData->BaseVolume = min(64,max(0,CData->BaseVolume));

                     if (Data->Pan == PanSurround)
                     {
                        Data->LeftVol = Data->MainVol/1;
                        Data->RightVol = Data->MainVol/(-1);
                     }
                     else
                     {
                        Data->LeftVol = (Data->MainVol*(PanMax - Data->Pan))/PanSpan;
                        Data->RightVol = (Data->MainVol*(PanMax + Data->Pan))/PanSpan;
                     }
                  }
               }

               // Let an output filter know of the current location.
               Device->SetFrameInfo(&FInf);
               FInf.SongTime += FrameLength;
               
               if (SkipOrds == 0 && SkipRows == 0)
               {
                  long Rc = 0;
                  if ((Rc = Device->Compute(&ChanDat,FrameLength)) != 0)
                  {
                     ChanDat.free();
                     delete [] Centre;
                     delete [] OrdPlayList;
                     delete [] CommandInfo;
                     return Rc;
                  }

                  // Zero all of the flags.
                  for (museChannel *Iter = ChanDat.begin(); Iter != ChanDat.end(); Iter++)
                     Iter->Flags = 0;
               }
            };
         }
      };

      CurOrder++;
   };

   ChanDat.free();
   delete [] Centre;
   delete [] OrdPlayList;
   delete [] CommandInfo;
   return PLAYOK;
}
