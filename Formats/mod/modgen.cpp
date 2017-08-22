// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   MODGeneral - This is the protracker player. 

   This was essentially derived from the S3M code and modifed to handle
   the protracker quirks.
   
   ##################################################################### */
									/*}}}*/
// Includes								/*{{{*/
#include <muse.h>
#include <modgen.h>

#include <stdlib.h>
#include <math.h>
   									/*}}}*/

#define MAXNOCHANS 32
#define MAXEFFECTS 32

template class Sequence<museMODElement>;

struct CmdInfoMOD 
{
   int Period;
   int DelayFlags;
   int PortTarget;
      
   int VibratoType;
   int VibratoPoint;
   int VibratoSpeed;
   int VibratoSize;
   int TremoloType;
   int TremoloPoint;
   int TremoloSpeed;
      
   int BasePeriod;
   int BaseVolume;
   int MainVol;

   int RowDelay;
   int TremorFlipFlop;
   int TremorCounter;
   int RetrigCounter;
   float ArpeggioCounter;
};

// MODGeneric::Play - Play ProTracker data
// ---------------------------------------------------------------------
/* */
long museMODGeneric::Play(museOutputBase *Device,musePlayerControl *Control)
{   
   signed short VSineTable[256];
   signed short VSquare[256];
   signed short VRamp[256];
   signed short VRandom[256];

   signed short *VibratoTypes[4] = { VSineTable, VRamp, VSquare, VRandom };

   CmdInfoMOD *CommandInfo = new CmdInfoMOD[MAXNOCHANS];

   museSongInfo SngInfo;
   GetSongInfo(SngInfo);
   
   int x;
   
   int CurOrder = 0;
   int RowSkip = -1;
   unsigned int I;
   unsigned char GlobalVolume = 64;
   unsigned short MODChans = SngInfo.Channels;
   unsigned char *Patterns;
   unsigned long OrderNum;
   unsigned char RowsAPat = GetRowsAPattern();
   GetOrderList(&Patterns,&OrderNum);

   Command Cmds[MAXNOCHANS];
   unsigned char *CmdInfos;
   CmdInfos = new unsigned char[MAXNOCHANS*MAXEFFECTS];
   
   memset(CommandInfo,0,MAXNOCHANS*sizeof(CmdInfoMOD));

   SngInfo.Free();
   
   // Generate the volume tables for vibrato, etc
   for (x = 0; x < 256; x++) 
      VSineTable[x] = (short)(256*sin((x*3.14159265)/128));

   for (x = 0; x < 256; x++) 
      VSquare[x] = (x < 128?128:-128);
   
   for (x = 0; x < 256; x++) 
      VRamp[x] = ( x - 128 ) * 2;
   
   for (x = 0; x < 256; x++) 
      VRandom[x] = (rand() % 510) - 255;

   // Definintions of tracking variables
   unsigned char FramesARow;
   unsigned long FrameLength;

   museFrameInfo FInf;
   FInf.SongTime = 0;

   {
       unsigned char Tempo;
       GetInitValues ( &FramesARow, &Tempo, &GlobalVolume );
       FrameLength = Device->SecondsToLen(60.0/(24.0*((float)Tempo)));
       FInf.GlobalVolume = GlobalVolume * VolMax / 64;
       FInf.Tempo = Tempo;
       FInf.Speed = FramesARow;
   }

   // Ennumerate the # of channels
   unsigned char Channels = MODChans;
   Sequencebool BoolSeq;
   BoolSeq.construct();
   BoolSeq.reserve(Channels);
   for (I = 0; I != Channels; I++)
      BoolSeq[I] = true;
   Device->SetChannelMask(&BoolSeq);
   Device->SetVolumeScale(0);
   BoolSeq.free();

   // Get the samples from the file format
   SequenceSample MODSamples;
   GetSongSamples(MODSamples);

   Device->LoadSamples(&MODSamples);

   for (I = 0; I != MODSamples.size(); I++)
      MODSamples[I].Volume = (MODSamples[I].Volume*64)/0xFF;

   SequenceChannel ChanDat;
   ChanDat.construct();
   ChanDat[Channels-1].Flags = 0;
   for (I = 0; I != Channels; I++)
   {
      ChanDat[I].Sample = 0xFFFF;
      ChanDat[I].ModuleChannel = I;
   }
   unsigned char Pan[MAXNOCHANS];
   GetMODPan(Pan);

   // Convert to more usable panning values
   for (I = 0; I != Channels; I++)
      ChanDat[I].Pan = (2*Pan[I] - 0xF)*(PanSpan/2)/0xF;

   memset(Cmds,0xFF,MAXNOCHANS*sizeof(Command));
   memset(CmdInfos,0,MAXEFFECTS*MAXNOCHANS);

   SequenceMODElement RowData;
   RowData.construct();
   RowData.reserve(MODChans);

   PlayedRec *OrdPlayList = new PlayedRec[OrderNum + 1];
   memset(OrdPlayList,0,OrderNum*sizeof(PlayedRec));

   unsigned long SkipOrds = 0;
   unsigned long SkipRows = 0;

   while (CurOrder < (int)OrderNum)
   {
      int LoopRow = -1;
      int CurrentLoop = 0;
      char NextOrder = 0;
      char Jump = 0;

      FInf.Order = CurOrder;
      FInf.Pattern = Patterns[CurOrder];
      if (SkipOrds != 0)
         SkipOrds--;
      for (unsigned char Row = 0;Row != RowsAPat || (RowSkip != -1 && NextOrder == 0); Row++)
      {
         unsigned char RowRepeat = 1;

         if ( RowSkip != -1)
         {
            Row = RowSkip;
            RowSkip = -1;
         }

         memset(Cmds,0xFF,MAXNOCHANS*sizeof(Command));

         // Get this row
         long Rc;
         
         if ((Rc=GetRowElements(&RowData,Row,Patterns[CurOrder])) != 0)
         {
             delete [] CommandInfo;
             return Rc;
         }

         OrdPlayList[CurOrder].Rows[Row/32] |= (1 << (Row % 32));
         FInf.Row = Row;

         // Decrease skip counter
         if (SkipRows != 0)
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
               CurOrder = NOrd - 1 + NRow/RowsAPat;
               RowSkip = NRow % RowsAPat;
               if (NRow < 0)
               {
                  RowSkip = RowsAPat + RowSkip;
                  CurOrder--;
               }
               CurOrder = max(CurOrder,-1);

               // Cut all the channels notes
               for (museChannel *Iter = ChanDat.begin(); Iter != ChanDat.end(); Iter++)
               {
                  Iter->Flags &= ~CHAN_Retrig;
                  Iter->Flags |= CHAN_Cut;
               }

               // Jump to start, reset all vars
               if ((CurOrder == -1) && (RowSkip == 0))
               {
                  unsigned char Tempo;
                  GetInitValues ( &FramesARow, &Tempo, &GlobalVolume );
                  FrameLength = Device->SecondsToLen(60.0/(24.0*((float)Tempo)));
                  FInf.GlobalVolume = GlobalVolume * VolMax / 64;
                  FInf.Tempo = Tempo;
                  FInf.Speed = FramesARow;
                  FInf.SongTime = 0;

                  unsigned char Pan[MAXNOCHANS];
                  GetMODPan(Pan);

                  // Convert to more usable panning values
                  for (I = 0; I != Channels; I++)
                     ChanDat[I].Pan = (2*Pan[I] - 0xF)*(PanSpan/2)/0xF;

                  memset(Cmds,0xFF,MAXNOCHANS*sizeof(Command));
                  memset(CmdInfos,0,MAXEFFECTS*MAXNOCHANS);

                  memset(CommandInfo,0,MAXNOCHANS*sizeof(CmdInfoMOD));
               }
               break;
            }
         }

         // Decode each channel
         for ( unsigned char Channel = 0; Channel != MODChans; Channel++ )
         {
            museMODElement &Element = RowData[Channel];
            museChannel *Data = &ChanDat[Channel];
            CmdInfoMOD *CData = &CommandInfo[Channel];
            unsigned long OldNote = 0;
            unsigned long OldVol = 0;
            unsigned char Inst = Element.Sample;

            // Change instruments
            if (Inst != 0)
            {
               Inst--;

               if (Inst >= MODSamples.size())
                  Inst = MODSamples.size() - 1;

               // ST3 does not rehit the note on a inst change
               Data->Flags |= CHAN_Instrument;    // Change instrument

               Data->Sample = Inst;

               if ( MODSamples[Data->Sample].Sample == 0 )
               {
                   printf ( "%ld %p\n", Data->Sample, MODSamples[Data->Sample].Sample );
               }
               
               if ((Data->Sample == 0xFFFF))// || (MODSamples[Data->Sample].Sample == 0))
               {
                  Data->Flags &= ~CHAN_Retrig;
                  Data->Flags |= CHAN_Cut;     // Disable Note (Error Check)
               }
               else
               {
                  if ( CData->Period != -1 )
                  {
                     CData->MainVol = MODSamples[Data->Sample].Volume;
                     CData->BaseVolume = CData->MainVol;
                     Data->Flags |= CHAN_Volume;  // Change volume
                  }
               }
            }

            // Note Change
            unsigned int Note = Element.Period;

            if ( Note != 0 )
            {
               // This is not a null note, get Hz
               OldNote = CData->Period;
               if ( Data->Sample < 31 )
               {
                  if ( MODSamples[Data->Sample].FineTune <= 16 )
                  {
                     float LocalFT;
                     LocalFT = MODSamples[Data->Sample].FineTune & 0xF;
                     if ( LocalFT > 7 ) LocalFT -= 16;
                     CData->Period = (int)(((float)Note)*4.0*(float)(1.0 - LocalFT*0.0037164*2.0));
                  }
                  else
                     CData->Period = (int)(Note*4*8363/MODSamples[Data->Sample].FineTune);

                  OldVol = CData->BaseVolume;

                  Data->Flags &= ~CHAN_Cut;
                  Data->Flags |= CHAN_Retrig;  // Start Note
                  Data->SampleOffset = 0;
               }
               else
                  CData->Period = Note;

               CData->BasePeriod = CData->Period;
               Data->Flags |= CHAN_Pitch;    // New Pitch
            }

            if ( Element.Volume <= 64 )
            {
               if (CData->MainVol != Element.Volume)
                  Data->Flags |= CHAN_Volume;  // Change volume
               CData->MainVol = Element.Volume;
               CData->BaseVolume = CData->MainVol;
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
            if (Element.Effect != 0 || Element.EffectParam != 0)
            {
               int Command = Element.Effect;
               if ( Command > MAXEFFECTS ) Command = MAXEFFECTS-1;
               int Info = Element.EffectParam;
               switch (Command)
               {
                  // Portamento (Command 3)
                  case 3:
                     if (OldNote != 0)
                     {
                        CData->PortTarget = CData->Period;
                        CData->Period= OldNote;
                        CData->BasePeriod = CData->Period;
                        Data->Flags &= ~CHAN_Retrig;   // Do not start note
                        Data->Flags |= CHAN_Volume;  // Change volume
                        CData->MainVol = OldVol;
                        CData->BaseVolume = OldVol;
                     }

                     if (Info != 0)
                        CmdInfos[Channel*MAXEFFECTS+3] = Info;
                     Cmds[Channel].Command = Command;
                     break;

                  // Vibrato (Command 4)
                  case 4:
                     if ((Info >> 4) != 0)
                     {
                        CData->VibratoSpeed = Info >> 4;// & 15;
                        CData->VibratoPoint = 0;
                     }

                     if ((Info & 15) != 0) //>> 4 != 0)
                        CData->VibratoSize = (Info & 15);
                     Cmds[Channel].Command = Command;
                     break;

                  // Dual Command G00 and Dxy (Command 5)
                  case 5:
                     if (Info == 0) break;
                     CmdInfos[Channel*MAXEFFECTS+10] = Info; //5
                     Cmds[Channel].Command = 10;
                     Cmds[Channel].Command2 = 3;

                     if (OldNote != 0)
                     {
                        CData->PortTarget = CData->Period;
                        CData->Period= OldNote;
                        CData->BasePeriod = CData->Period;
                        Data->Flags &= ~CHAN_Retrig;   // Do not start note
                        Data->Flags |= CHAN_Volume;  // Change volume
                        CData->MainVol = OldVol;
                        CData->BaseVolume = OldVol;
                     }
                     break;

                  // Dual Command H00 and Dxy (Command 6)
                  case 6:
                     if (Info == 0) break;
                     CmdInfos[Channel*MAXEFFECTS+10] = Info; //6
                     Cmds[Channel].Command = 10;
                     Cmds[Channel].Command2 = 4;
                     break;

                  // Tremolo (Command 7)
                  case 7:
                     if ((Info >> 4) != 0)
                     {
                        CData->TremoloSpeed = Info >> 4;
                        CData->TremoloPoint = 0;
                     }

                     CmdInfos[Channel*MAXEFFECTS+7] = Info;
                     Cmds[Channel].Command = Command;
                     break;

                  // DMP Panning (Command 8)
                  case 8:
                     if (Info == 0xA4)
                        Data->Pan = PanSurround;
                     else
                     {
                        Data->Pan = min(128,Info);
                        Data->Pan = ((Data->Pan - 64)*PanSpan)/128;
                     }
                     Data->Flags |= CHAN_Pan;
                     break;

                  // Sample Offset (Command 9)
                  case 9:
                     // Only works if a note has been started.
                     if ( Info != 0)
                     {
                        if (//(MODSamples[Data->Sample].Sample == 0) ||
                           (MODSamples[Data->Sample].SampleEnd == 0))
                           break;

                           Data->Flags |= CHAN_Retrig;  // Start Note
                           Data->SampleOffset =  ((Info << 8) %
                                       (MODSamples[Data->Sample].SampleEnd));
                     }
                     break;

                  // Jump to order (Command 11)
                  case 11:
                     CurOrder = Info;
                     CurOrder--;
                     RowSkip = 0;
                     Row = RowsAPat - 1;
                     NextOrder = 1;
                     Jump = 1;
                     break;

                  // Change Volume (Command 12)
                  case 12:
                     if (Info <= 64)
                     {
                        CData->MainVol = Info;
                        CData->BaseVolume = CData->MainVol;
                        Data->Flags |= CHAN_Volume;    // Change volume
                     }
                     break;

                  // Skip Pattern to row (Command 13)
                  case 13:
                     RowSkip = (Info & 0xF) + 10*(Info >> 4);
                     if ((RowSkip > RowsAPat - 1) || ((Info & 0xF) > 9) || ((Info >> 4) > 9))
                        RowSkip = RowsAPat - 1 ;
                     Row = 63;
                     Jump = 1;
                     NextOrder = 1;
                     break;

                  // Command Group 14
                  case 14:
                     switch (Info >> 4)
                     {
                        // Pitch slide up (Command 1)
                        case 1:
                           // Slide Up
                           CData->Period -= ( Info & 0xF ) * 64;
                           CData->Period = max(113*64,CData->Period);
                           Data->Flags |= CHAN_Pitch;    // New Pitch
                           CData->BasePeriod = CData->Period;
                           break;

                        // Pitch slide down (Command 2)
                        case 2:
                           // Slide Down
                           CData->Period += ( Info & 0xF ) * 64;
                           CData->Period = min(856*64,CData->Period);
                           Data->Flags |= CHAN_Pitch;    // New Pitch
                           CData->BasePeriod = CData->Period;
                           break;

                        // Set vibrato waveform
                        case 4:

                           CData->VibratoType = (Info & 0xF);
                           break;

                        //Set finetune
                        case 5:
                           MODSamples[Data->Sample].FineTune = Info & 15;
                           break;

                        // Pattern loop
                        case 6:
                           if ((Info & 0x0F) == 0)
                           {
                              LoopRow = Row;
                              break;
                           }

                           /* MUST set the loop point before, lame song that
                              uses this command but doesn't mean it :< (dmtrip) */
                           if (LoopRow == -1)
                              break;

                           // Finished the loop
                           if (CurrentLoop == 1)
                           {
                              CurrentLoop = 0;
                              break;
                           }

                           if (CurrentLoop == 0)
                              CurrentLoop = Info & 0x0F;
                           else
                              CurrentLoop--;
                           RowSkip = LoopRow;
                           break;

                        // Set tremolo waveform
                        case 7:

                           CData->TremoloType = (Info & 0xF);
                           break;

                        // Set pan position
                        case 8:
                           Data->Pan = Info & 0x0F;
                           Data->Pan = (2*Data->Pan - 0xF)*(PanSpan/2/0xF);
                           Data->Flags |= CHAN_Pan;
                           break;

                        case 0xA:
                           CData->MainVol += Info & 15;
                           CData->BaseVolume = CData->MainVol;
                           Data->Flags |= CHAN_Volume;    // Change volume
                           break;

                        case 0xB:
                           CData->MainVol -= Info & 15;
                           CData->BaseVolume = CData->MainVol;
                           Data->Flags |= CHAN_Volume;    // Change volume
                           break;

                        // Note Cut in X frames
                        case 0xC:
                           CData->RowDelay = (Info & 0X0F);
                           CData->DelayFlags = CHAN_Cut;     // Disable Note
                           break;

                        // Note Delay for X frames
                        case 0xD:
                           CData->RowDelay = (Info & 0X0F);
                           CData->DelayFlags = Data->Flags;
                           Data->Flags = 0;
                           break;

                        // Row Repeat
                        case 0xE:
                           RowRepeat = (Info & 0x0F);
                           break;

                        default:
                           if (Info != 0)
                           {
                              CmdInfos[Channel*MAXEFFECTS+Command] = Info;
                              Cmds[Channel].Command = Command;
                           }
                           break;
                     }
                     break;

                  // Speed Change (Command 15)
                  case 15:
                     if ( Info != 0 ) if ( Info < 32 )
                     {
                        FramesARow = Info;
                        FInf.Speed = FramesARow;
                     }
                     else
                     {
                        FrameLength = Device->SecondsToLen(60/(24*((float)Info)));
                        FInf.Tempo = Info;
                     }
                     break;

                  default:
                     if (Info != 0)
                     {
                        CmdInfos[Channel*MAXEFFECTS+Command] = Info;
                        Cmds[Channel].Command = Command;
                     }
                     break;
               };
            }
         }

         if (Jump == 1)
         {
            Jump = 0;
            // Check to see if the first row has been played,
            if (AllowLoop == false && (CurOrder + 1) < (int)OrderNum)
            {
               if ((OrdPlayList[CurOrder + 1].Rows[RowSkip/32] &
                   (1 << (RowSkip % 32))) != 0)
               {
                  CurOrder = OrderNum - 1;
		  Trace(TRACE_INFO,"Aborting");
               }
            }
         }

         // Let an output filter know of the current location.
         for (;RowRepeat != 0; RowRepeat--)
         {
            for (I = 0; I != FramesARow; I++)
            {
               FInf.Frame = I;

               char FlipFlop = 0;
               for (unsigned char Channel = 0; Channel != MODChans;)
               {
                  museChannel *Data = &ChanDat[Channel];
                  CmdInfoMOD *CData = &CommandInfo[Channel];
                  unsigned char Info;
                  unsigned char Command;
                  if (FlipFlop == 0)
                  {
                     Info = CmdInfos[Channel*MAXEFFECTS+Cmds[Channel].Command];
                     Command = Cmds[Channel].Command;
                  }
                  else
                  {
                     Info = CmdInfos[Channel*MAXEFFECTS+Cmds[Channel].Command2];
                     Command = Cmds[Channel].Command2;
                  }

                  switch (Command)
                  {
                     // Arpeggio (Command 0 with non-0 arguments)
                     case 0:
                        if (Info != 0 )
                        {
                           switch (((long)CData->ArpeggioCounter) % 3)
                           {
                              case 0:
                                 CData->Period = CData->BasePeriod;
                                 break;
                              case 1:
                                 CData->Period = (int)(CData->BasePeriod/pow(1.059463094,Info & 15));
                                 break;
                              case 2:
                                 CData->Period = (int)(CData->BasePeriod/pow(1.059463094,Info >> 4));
                                 break;
                           }

                           CData->ArpeggioCounter += ((float)FrameLength)/((float)Device->SecondsToLen(1/50.0));
                           Data->Flags |= CHAN_Pitch;    // Change pitch
                           break;
                        }

                     // Pitch slide up (Command 1)
                     case 1:
                        // Slide Up
                        if (I == 0) break;

                        CData->Period -= Info * 64;
                        CData->Period = max(113*64,CData->Period);
                        Data->Flags |= CHAN_Pitch;    // New Pitch
                        CData->BasePeriod = CData->Period;
                        break;

                     // Pitch slide down (Command 2)
                     case 2:
                        // Slide Down
                        if (I == 0) break;

                        CData->Period += Info * 64;
                        CData->Period = min(856*64,CData->Period);
                        Data->Flags |= CHAN_Pitch;    // New Pitch
                        CData->BasePeriod = CData->Period;
                        break;

                     // Portamento (Command 3)
                     case 3:
                     case 5:
                        if ( I == 0 ) break;
                        if (CData->Period == CData->PortTarget)
                           break;

                        if (CData->Period > CData->PortTarget)
                        {
                           CData->Period -= Info*64;
                           if (CData->Period < CData->PortTarget)
                              CData->Period = CData->PortTarget;
                        }

                        if (CData->Period < CData->PortTarget)
                        {
                           CData->Period += Info*64;
                           if (CData->Period > CData->PortTarget)
                              CData->Period = CData->PortTarget;
                        }

                        Data->Flags |= CHAN_Pitch;    // New Pitch
                        CData->BasePeriod = CData->Period;
                        break;

                     // Vibrato (Command 4 & 6)
                     case 4:
                     case 6:
                     {
                        double P;
                        P = VibratoTypes[CData->VibratoType%4][(CData->VibratoPoint*4)%256];

                        // Scale the value
                        P *= CData->VibratoSize;
                        P /= 256 * 16 * 8;
                        // P *= 0.059463094;

                        CData->Period = (int)(CData->BasePeriod*(1 + P));

                        if ( I )
                        {
                            CData->VibratoPoint += CData->VibratoSpeed;
                            Data->Flags |= CHAN_Pitch;    // New Pitch
                        }
                        break;
                     }

                     // Tremolo (Command 7)
                     case 7:
                     {
                        signed short P;

                        // Compute the correct table index
                        P = VibratoTypes[CData->TremoloType%4][(CData->TremoloPoint*4)%256];

                        // Scale the value
                        P *= Info & 0xF;
                        P /= 128;

                        CData->MainVol = CData->BaseVolume + P;

                        if ( I ) CData->TremoloPoint += CData->TremoloSpeed * 4;

                        Data->Flags |= CHAN_Volume;   // New Volume
                        break;
                     }

                     // Volume Slides (Command 10)
                     case 10:
                        if (I == 0 || Info == 0) break;

                        // Volume slide Down
                        if ((Info & 0xF0) == 0x00)
                        {
                           CData->MainVol -= Info & 0xF;
                           if ( CData->MainVol < 0 ) CData->MainVol = 0;
                           CData->BaseVolume = CData->MainVol;
                           Data->Flags |= CHAN_Volume;    // Change volume
                           break;
                        }
                        else
                        {
                           CData->MainVol += Info >> 4;
                           if ( CData->MainVol > 64 ) CData->MainVol = 64;
                           CData->BaseVolume = CData->MainVol;
                           Data->Flags |= CHAN_Volume;    // Change volume
                           break;
                        }
                        break;

                     // Command Group E
                     case 0xE:
                        switch (Info >> 4)
                        {
                           case 9:
                              if (I%(Info & 0xF) == 0)
                              {
                                 Data->Flags &= ~CHAN_Cut;
                                 Data->Flags |= CHAN_Retrig;
                                 Data->SampleOffset = 0;
                              }
                              break;
                        }
                        break;

                     // Tremor (Command 250, STM special command)
                     case 250:
                        if (CData->TremorCounter != 0) CData->TremorCounter--;

                        if (CData->TremorFlipFlop == 0)
                        {
                           if (CData->TremorCounter == 0)
                           {
                              CData->TremorFlipFlop = 1;
                              CData->TremorCounter = (Info & 0xF);
                           }
                           CData->MainVol = 0;
                           Data->Flags |= CHAN_Volume;   // New Volume
                        }
                        else
                        {
                           if (CData->TremorCounter == 0)
                           {
                              CData->TremorFlipFlop = 0;
                              CData->TremorCounter = Info >> 4;
                           }
                           CData->MainVol = CData->BaseVolume;
                           Data->Flags |= CHAN_Volume;   // New Volume
                        }
                        break;

                     default:
                        break;
                  }

                  // Notedelay and note stop
                  if ((CData->RowDelay != 0) && (CData->RowDelay == (int)I))
                  {
                     Data->Flags = CData->DelayFlags;
                     CData->RowDelay = 0;
                  }

                  if ((Cmds[Channel].Command2 != 0xFF) && (FlipFlop == 0))
                  {
                     FlipFlop = 1;
                     continue;
                  }
                  else
		     FlipFlop = 0;

		  /* Don't bother doing this when we are skipping or there
		     is nothing to update */
                  if (Data->Flags == 0 || SkipOrds != 0 || SkipRows != 0)
		  {
		     Channel++;
		     continue;
		  }

                  // Fix pitch
                  if ((Data->Flags & CHAN_Pitch) != 0)
                  {
                     boundv(37*64,CData->Period,0x7FFF*64);
                     Data->Pitch = (int)(14317056.0/4.0*64.0/(float)CData->Period);
                  }

                  // Fix Volume
                  if ((Data->Flags & CHAN_Volume) != 0)
                  {
                     Data->MainVol = bound(0,CData->MainVol,64);
                     boundv(0,CData->BaseVolume,64);
                     Data->MainVol = (Data->MainVol*VolMax*GlobalVolume)/64/64;

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

                  Channel++;
               }

               Device->SetFrameInfo(&FInf);
               FInf.SongTime += FrameLength;
               if (SkipOrds == 0 && SkipRows == 0)
               {
                  long Rc = 0;
                  if ((Rc = Device->Compute(&ChanDat,FrameLength)) != 0)
                  {
                     ChanDat.free();
                     RowData.free();
                     delete [] OrdPlayList;
                     MODSamples.free();
                     delete [] CmdInfos;
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
   RowData.free();
   delete [] OrdPlayList;
   MODSamples.free();
   delete [] CmdInfos;
   delete [] CommandInfo;
   return PLAYOK;
}
