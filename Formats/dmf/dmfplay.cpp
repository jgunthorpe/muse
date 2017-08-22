
/* ########################################################################

   FormatBase - Base class for all Muse/2 Module file formats (eg DMF's)

   This is the player routine for the DMF files.

   ######################################################################## */

#include <Muse.h>

#ifndef CPPCOMPILE
#include <DMFForm.hh>
#else
#include <DMFForm.hc>
#endif

#include "DMFStruc.h"
#include <math.h>

//#define MULTIPLE 1
#define BPMCONST 8
#define AMIGACONST (1712-64-8)

/* ########################################################################

   Class - museDMFormat (Class to allow Muse/2 to handle DMF files)
   Member - Play (Plays the song)
   IDL - long Play(in museOutputBase Device)

   ######################################################################## */

long museDMFFormat::Glissando ( long Frequency, long BaseNote )
{
float Current = BaseNote;
while ( Current * 2 >= Frequency ) Current *= 2;
while ( Current * 1.059463094 >= Frequency ) Current *= 1.059463094;
if ( Frequency - Current <= Current * 1.059463094 - Frequency ) return ( Current );
   else return ( Current * 1.059463094 );
}

long museDMFFormat::Play(museOutputBase *Device, musePlayerControl *Control)
{
   DBU_FuncTrace("museDMFFormat","Play",TRACE_SIMPLE);
   if ( ( Header == 0 ) || ( Sequence->HeaderSize - 12 <= 0 ) || ( PatternHeader->NoOfPatterns == 0 ) || ( SampleInfoHeader->NoOfSamples == 0 ) )
      return PLAYFAIL_NotLoaded;


   signed short VSineTable[256];
   signed short VSquare[256];
   signed short VRamp[256];
   signed short *VibratoTypes[3] = { VSineTable, VSquare, VRamp };

   struct CmdInfo {
   int Note;
   int Period;
   int PortaTarget;
   int Instrument;
   int Volume;
   int Flags;
   int FineTune;
   int PanPos;

   int NoteParm;
   int InstrumentParm;
   int VolumeParm;
   int NoteEffectParm;
   int NoteEffectDataParm;
   int InstrumentEffectParm;
   int InstrumentEffectDataParm;
   int VolumeEffectParm;
   int VolumeEffectDataParm;

   int VibratoType;
   int VibratoPos;
   int VibratoSpeed;
   int VibratoSize;
   int TremoloType;
   int TremoloPos;
   int TremoloSpeed;
   int TremoloSize;
   int PanbrelloPos;
   int PanbrelloSpeed;
   int PanbrelloSize;
   int TremorPos;
   int NTremorPos;
   int ArpeggioCounter;

   int ReverseFlag;
   
   int PeriodShift;
   int VolumeShift;
   int PanShift;
   } ChannelData[32];

   unsigned short CurOrder;
   unsigned char *LastPos;
   char PatternDelay, DelayTicks;
   int x, y, z;
   int LastRow, LastOrder, NewRow;

   CurOrder = 0;
   PatternDelay = 0;
   DelayTicks = 0;
   LastPos = 0;
   LastRow = -1;
   LastOrder = -1;
   NewRow = -1;

   memset ( ChannelData, 0, 32*sizeof ( CmdInfo ) );

   for ( x = 0; x < 256; x++ ) VSineTable[x] = 256 * sin ( x * 3.14159265 / 128 );
   for ( x = 0; x < 256; x++ ) VSquare[x] = ( x < 128 ? 128 : -128 );
   for ( x = 0; x < 256; x++ ) VRamp[x] = ( x - 128 ) * 2;

   // Definintions of tracking variables
   unsigned short FramesARow, TicksAFrame;
   unsigned long FrameLength;
#ifdef MULTIPLE
   unsigned long Multiple;
#else
#define Multiple 1
#endif

   // Ennumerate the # of channels
   Sequencebool BoolSeq;
   BoolSeq.construct();
   BoolSeq.reserve(32);

   for ( x = 0; x < 32; x++ ) if ( x < PatternHeader->NoOfChannels ) BoolSeq[x] = true; else BoolSeq[x] = false;
   Device->SetChannelMask(&BoolSeq);
   Device->SetVolumeScale (0);
   BoolSeq.free();

   // Load the samples into the mixer.
   SequenceSample MixSamples;
   MixSamples.construct();
   GetSongSamples(MixSamples);
   Device->LoadSamples(&MixSamples);
   MixSamples.free();

   SequenceChannel ChanDat;
   ChanDat.construct();
   ChanDat[32].Flags = 0;
   for ( x = 0; x < 32; x++ ) ChanDat[x].Sample = 0xFFFF;

   for ( x = 0; x < PatternHeader->NoOfChannels; x++ )
   {
       if ( x & 1 ) ChannelData[x].PanPos = (2*0xC - 0xF)*(PanSpan/2)/0xF;
       else ChannelData[x].PanPos = (2*0x3 - 0xF)*(PanSpan/2)/0xF;
       ChannelData[x].Flags |= CHAN_Pan | CHAN_Volume;
   }

   // Construct array to hold row bitmap
   PlayedRec *OrdPlayList = new PlayedRec[Sequence->HeaderSize - 12 + 1];
   memset(OrdPlayList,0,(Sequence->HeaderSize - 12 + 1)*sizeof(PlayedRec));
   museFrameInfo FInf;
   FInf.GlobalVolume = VolMax;
   FInf.SongTime = 0;
   unsigned long SkipOrds = 0;
   unsigned long SkipRows = 0;

   FrameLength = Device->SecondsToLen ( 0.02 );
   FInf.Tempo = 125;
   
   while ( CurOrder < (Sequence->HeaderSize-4)/2 ) {
      int Row = 0;
      if ( NewRow != -1 ) {
         Row = NewRow;
         NewRow = -1;
         }
      int NewOrder = -1;

      FInf.Order = CurOrder;
      FInf.Pattern = (&Sequence->Orders)[CurOrder];
      
      // Smart Order Skipping
      if (SkipOrds != 0)
          SkipOrds--;
      
      char Counters[33];
      while ( Row < Patterns[(&Sequence->Orders)[CurOrder]]->Rows ) {
         int LoopRow = 0;
         int CurrentLoop = 0;

         // Check that this row has never been played before
         if (AllowLoop == false && (CurOrder) < (Sequence->HeaderSize-4)/2)
         {
             if ((OrdPlayList[CurOrder].Rows[Row/32] & (1 << (Row % 32))) != 0)
             {
                 CurOrder = (Sequence->HeaderSize-4)/2;
                 somPrintf("Aborting\n");
                 break;
             }
         }
         
         // Mark the row as played.
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
                 NewOrder = NOrd;
                 if (NRow < 0)
                 {
                     while (1)
                     {
                         NewOrder--;
                         if (NewOrder < 0)
                         {
                             NRow = 0;
                             break;
                         }
                         NRow += Patterns[(&Sequence->Orders)[NewOrder]]->Rows;
                         if (NRow >  0)
                             break;
                     }
                 }
                 else
                 {
                     while (1)
                     {
                         if (NRow < Patterns[(&Sequence->Orders)[NewOrder]]->Rows)
                             break;
                         NRow -= Patterns[(&Sequence->Orders)[NewOrder]]->Rows;
                         NewOrder++;
                         if (NewOrder > (Sequence->HeaderSize-4)/2)
                         {
                             NRow = 0;
                             break;
                         }
                     }
                 }
                 NewRow = NRow;
                 
                 // Cut all the channels notes
                 for ( x = 0; x < PatternHeader->NoOfChannels; x++ )
                 {
                     ChanDat[x].Flags |= CHAN_Cut;
                     ChanDat[x].Flags &= CHAN_Retrig;
                 }
                 
                 // Jump to start, reset all vars
                 if ((NewOrder == 0) && (NewRow == 0))
                 {
                     CurOrder = 0;
                     PatternDelay = 0;
                     DelayTicks = 0;
                     LastPos = 0;
                     LastRow = -1;
                     LastOrder = -1;
                     NewRow = -1;

                     memset ( ChannelData, 0, 32*sizeof ( CmdInfo ) );

                     FrameLength = Device->SecondsToLen ( 0.02 );
                     FInf.Tempo = 125;
                     
                     for ( x = 0; x < PatternHeader->NoOfChannels; x++ )
                     {
                         ChanDat[x].Sample = 0xFFFF;
                         ChanDat[x].Flags = CHAN_Free | CHAN_Cut;

                         if ( x & 1 ) ChannelData[x].PanPos = (2*0xC - 0xF)*(PanSpan/2)/0xF;
                         else ChannelData[x].PanPos = (2*0x3 - 0xF)*(PanSpan/2)/0xF;
                         ChannelData[x].Flags |= CHAN_Pan | CHAN_Volume;
                     }
                     
                     FInf.SongTime = 0;
                     
                     memset(OrdPlayList,0,((Sequence->HeaderSize-4)/2 + 1)*sizeof(PlayedRec));
                     SkipOrds = 0;
                     SkipRows = 0;
                     
                 }
                 break;
             }
         }
         
         //Get the pointer to the current row
         unsigned char *DataPointer;
         int Repeat;

         if ( LastOrder == CurOrder && LastRow == Row - 1 ) {
            DataPointer = LastPos;
            } else {
            memset ( Counters, 0, 33 );

            DataPointer = &Patterns[(&Sequence->Orders)[CurOrder]]->PatternData;

            FramesARow = ( Patterns[(&Sequence->Orders)[CurOrder]]->Beat & 15 );
            if ( Header->Version < 8 ) FramesARow = 1;
            if ( FramesARow == 0 ) FramesARow = 1;
            FInf.Speed = FramesARow;
            
            TicksAFrame = ( Patterns[(&Sequence->Orders)[CurOrder]]->Beat >> 4 );
            if ( TicksAFrame == 0 ) TicksAFrame = 1;
#ifdef MULTIPLE
            z = MULTIPLE;
            Multiple = 1;
            while ( z > 1 )
            {
                if ( ( TicksAFrame / z ) && ( TicksAFrame / z ) * z == TicksAFrame )
                {
                    Multiple = z;
                    break;
                }

                z /= 2;
            }
#endif
            
            x = 0;
            while ( x != Row ) {
               if ( !Counters[0] ) {
                  z = *DataPointer++;
                  if ( z & PATTERN_Counter ) Counters[0] = *DataPointer++;
                  if ( z & ~PATTERN_Counter ) DataPointer++;
                  } else Counters[0]--;
               for ( y = 0; y < Patterns[(&Sequence->Orders)[CurOrder]]->Channels; y++ ) {
                  if ( !Counters[y+1] ) {
                     z = *DataPointer++;
                     if ( z & PATTERN_Counter ) Counters[y+1] = *DataPointer++;
                     if ( z & PATTERN_Instrument ) DataPointer += 1;
                     if ( z & PATTERN_Note ) DataPointer += 1;
                     if ( z & PATTERN_Volume ) DataPointer += 1;
                     if ( z & PATTERN_InstrumentEffect ) DataPointer += 2;
                     if ( z & PATTERN_NoteEffect ) DataPointer += 2;
                     if ( z & PATTERN_VolumeEffect ) DataPointer += 2;
                     } else Counters[y+1]--;
                  }
               x++;
               }
            }

         //For each channel, calculate the parameters
         int GlobalEffect = 0, GlobalEffectData = 0;
         if ( !Counters[0] ) {
            z = *DataPointer++;
            if ( z & PATTERN_Counter ) Counters[0] = *DataPointer++;
            if ( z & ~PATTERN_Counter ) {
               GlobalEffect = ( z & ~PATTERN_Counter );
               GlobalEffectData = *DataPointer++;
               }
            } else Counters[0]--;

         switch ( GlobalEffect ) {
            case EFF_G_None:
               break;
            case EFF_G_SetTickSpeed:
               FrameLength = Device->SecondsToLen ( 1.0 / ( ((float) GlobalEffectData)/4.0+1.0 ) );
               FInf.Tempo = ( ((float) GlobalEffectData)/4.0+1.0 ) * 2.5;
               break;
            case EFF_G_SetBPMSpeed:
               FrameLength = Device->SecondsToLen ( BPMCONST / (float)(GlobalEffectData * FramesARow * TicksAFrame) );
               FInf.Tempo = GlobalEffectData;
               break;
            case EFF_G_SetBeatSpeed:
               FramesARow = ( GlobalEffectData & 15 );
               if ( FramesARow == 0 ) FramesARow = 1;
               TicksAFrame = (GlobalEffectData >> 4);
               if ( TicksAFrame == 0 ) TicksAFrame = 1;

#ifdef MULTIPLE
               z = MULTIPLE;
               Multiple = 1;
               while ( z > 1 )
               {
                   if ( ( TicksAFrame / z ) && ( TicksAFrame / z ) * z == TicksAFrame )
                   {
                       Multiple = z;
                       break;
                   }
                   
                   z /= 2;
               }
#endif
               
               FInf.Speed = FramesARow;
               break;
            case EFF_G_SetTickDelaySpeed:
               DelayTicks = GlobalEffectData;
               break;
            case EFF_G_SetFlag:
               break;
            }

         for ( int Channel = 0; Channel < Patterns[(&Sequence->Orders)[CurOrder]]->Channels; Channel++ ) {
            ChannelData[Channel].NoteParm = 0;
            ChannelData[Channel].InstrumentParm = 0;
            ChannelData[Channel].VolumeParm = 256;
            ChannelData[Channel].NoteEffectParm = 0;
            ChannelData[Channel].NoteEffectDataParm = 0;
            ChannelData[Channel].InstrumentEffectParm = 0;
            ChannelData[Channel].InstrumentEffectDataParm = 0;
            ChannelData[Channel].VolumeEffectParm = 0;
            ChannelData[Channel].VolumeEffectDataParm = 0;

            if ( !Counters[Channel+1] ) {
               z = *(DataPointer++);
               if ( z & PATTERN_Counter )
                   Counters[Channel+1] = *(DataPointer++);

               if ( z & PATTERN_Instrument )
                   ChannelData[Channel].InstrumentParm = *(DataPointer++);

               if ( z & PATTERN_Note )
                   ChannelData[Channel].NoteParm = *(DataPointer++);

               if ( z & PATTERN_Volume )
                   ChannelData[Channel].VolumeParm = *(DataPointer++);

               if ( z & PATTERN_InstrumentEffect )
               {
                   ChannelData[Channel].InstrumentEffectParm = *(DataPointer++);
                   ChannelData[Channel].InstrumentEffectDataParm = *(DataPointer++);
               }

               if ( z & PATTERN_NoteEffect )
               {
                   ChannelData[Channel].NoteEffectParm = *(DataPointer++);
                   ChannelData[Channel].NoteEffectDataParm = *(DataPointer++);
               }

               if ( z & PATTERN_VolumeEffect )
               {
                   ChannelData[Channel].VolumeEffectParm = *(DataPointer++);
                   ChannelData[Channel].VolumeEffectDataParm = *(DataPointer++);
               }

               } else Counters[Channel+1]--;
            }
         LastPos = DataPointer;
         LastOrder = CurOrder;
         LastRow = Row;

         //Loop through each frame
         for ( int Frame = 0; Frame < FramesARow; Frame++ ) {
            FInf.Frame = Frame;
             
            int Channel = 0;
            while ( Channel < Patterns[(&Sequence->Orders)[CurOrder]]->Channels ) {
               CmdInfo *Data = &ChannelData[Channel];
               int Note = Data->NoteParm;
               int Instrument = Data->InstrumentParm;
               int Volume = Data->VolumeParm;
               int NoteEffect = Data->NoteEffectParm;

               if ( Instrument != 0 && Frame == 0 ) {
                  if ( Instrument > SampleInfoHeader->NoOfSamples + 1 ) {
                     Instrument = 0;
                     } else {
                     Data->Instrument = Instrument - 1;
                     if ( SampleInfos2[Data->Instrument]->Volume ) Data->Volume = SampleInfos2[Data->Instrument]->Volume * VolMax / 255;
                     Data->FineTune = SampleInfos2[Data->Instrument]->Frequency;
                     }
                  }

               //Check for a note change
               if ( Note == 255 && Frame == 0 ) {
                  Note = 0;
//                  Data->KeyState = 0;
//                  Data->TimeOff = Data->TimeCounter;
                  Data->Volume = 0;
                  Data->Flags |= CHAN_Volume;
                  }
               if ( Note & 128 ) {
                  }
               if ( Note && Note <= 108 && Frame == 0 ) {
                  if ( NoteEffect == EFF_N_Portamento ) {
                     Data->PortaTarget = (float) AMIGACONST * (float) (16777216/4) / (float) Data->FineTune / pow ( 2, (float) Data->Note / 12.0 );
                     } else {
                     Data->Note = Note - 1;
                     Data->Flags |= CHAN_Retrig | CHAN_Pan | CHAN_Pitch | CHAN_Volume;
                     ChanDat[Channel].SampleOffset = 0;
                     Data->Period = (float) AMIGACONST * (float) (16777216/4) / (float) Data->FineTune / pow ( 2, (float) Data->Note / 12.0 );
                     Data->ReverseFlag = 0;
                     }
                  }

               //Check for an instrument change
               if ( Instrument != 0 && Frame == 0 ) {
                  if ( Data->Instrument > SampleInfoHeader->NoOfSamples + 1 ) {
                     Data->Flags |= CHAN_Cut;
                     } else {
                     Data->Flags |= CHAN_Instrument | CHAN_Volume;
                     Data->Flags &= ~CHAN_Cut;
                     if ( Data->VibratoType / 4 == 0 ) {
                        Data->VibratoPos = 0;
                        }
                     if ( Data->TremoloType / 4 == 0 ) {
                        Data->TremoloPos = 0;
                        }
                     }
                  }

               if ( Volume > 0 && Volume <= 255 && Frame == 0 ) {
                  Data->Volume = Volume * VolMax / 255;
                  Data->Flags |= CHAN_Volume;
                  }

               Channel++;
               }

            for ( int Tick = 0; Tick < TicksAFrame / Multiple; Tick++ ) {
               for ( int Channel = 0; Channel < Patterns[(&Sequence->Orders)[CurOrder]]->Channels; Channel++ ) {
                  CmdInfo *Data = &ChannelData[Channel];
                  int VolumeEffect = Data->VolumeEffectParm;
                  int VolumeEffectData = Data->VolumeEffectDataParm;
                  int NoteEffect = Data->NoteEffectParm;
                  int NoteEffectData = Data->NoteEffectDataParm;
                  int InstrumentEffect = Data->InstrumentEffectParm;
                  int InstrumentEffectData = Data->InstrumentEffectDataParm;

                  if ( Data->PeriodShift != 0 ) Data->Flags |= CHAN_Pitch;
                  Data->PeriodShift = 0;
                  if ( Data->VolumeShift != 0 ) Data->Flags |= CHAN_Volume;
                  Data->VolumeShift = 0;
                  if ( Data->PanShift != 0 ) Data->Flags |= CHAN_Pan | CHAN_Volume;
                  Data->PanShift = 0;

                  switch ( VolumeEffect ) {
                     case EFF_V_None:
                        break;
                     case EFF_V_SlideUp:
                         Data->Volume += VolumeEffectData * VolMax / 255 / TicksAFrame;
                         if ( TicksAFrame > 1 && Tick == TicksAFrame - 1 ) Data->Volume += VolumeEffectData * VolMax / 255 - ( VolumeEffectData * VolMax / 255 / TicksAFrame ) * TicksAFrame;
                         if ( Data->Volume > VolMax ) Data->Volume = VolMax;
                         Data->Flags |= CHAN_Volume;
                        break;
                     case EFF_V_SlideDown:
                         Data->Volume -= VolumeEffectData * VolMax / 255 / TicksAFrame;
                         if ( TicksAFrame > 1 && Tick == TicksAFrame - 1 ) Data->Volume -= VolumeEffectData * VolMax / 255 - ( VolumeEffectData * VolMax / 255 / TicksAFrame ) * TicksAFrame;
                         if ( Data->Volume < 0 ) Data->Volume = 0;
                         Data->Flags |= CHAN_Volume;
                        break;
                     case EFF_V_Tremolo:
                        if ( Data->TremorPos >= ( VolumeEffectData >> 4 ) && Data->TremorPos < ( VolumeEffectData >> 4 ) + ( VolumeEffectData >> 15 ) ) {
                           Data->Flags |= CHAN_Volume;
                           Data->VolumeShift = -10000;
                           }
                        Data->TremorPos += Multiple;
                        if ( Data->TremorPos >= ( VolumeEffectData >> 4 ) + ( VolumeEffectData >> 15 ) ) Data->TremorPos -= ( ( VolumeEffectData >> 4 ) + ( VolumeEffectData >> 15 ) );
                        Data->Flags |= CHAN_Volume;
                        break;
                     case EFF_V_VibratoSine:
                        Data->TremoloType = 0;
                        TremoloIn:
                        Data->TremoloSpeed = ( VolumeEffectData >> 4 ) + 1;
                        Data->TremoloSize = ( VolumeEffectData & 15 );
                        Data->VolumeShift += Data->Volume * Data->TremoloSize * VibratoTypes[Data->TremoloType%4][Data->TremoloPos%256] / 255.0 / 128.0;
                        Data->TremoloPos += ( 256 / Data->TremoloSpeed ) * Multiple;
                        Data->Flags |= CHAN_Volume;
                        break;
                     case EFF_V_VibratoTriangular:
                        Data->TremoloType = 2;
                        goto TremoloIn;
                        break;
                     case EFF_V_VibratoSquare:
                        Data->TremoloType = 1;
                        goto TremoloIn;
                        break;
                     case EFF_V_SetPan:
                        if ( Frame == 0 && Tick == 0 ) {
                           Data->PanPos = VolumeEffectData * PanSpan / 255 - PanMax;
                           Data->Flags |= CHAN_Pan | CHAN_Volume;
                           }
                        break;
                     case EFF_V_SlidePanLeft:
                         Data->PanPos -= VolumeEffectData * PanSpan / 255 / TicksAFrame;
                         if ( TicksAFrame > 1 && Tick == TicksAFrame - 1 ) Data->PanPos -= VolumeEffectData * PanSpan / 255 - ( VolumeEffectData * PanSpan / 255 / TicksAFrame ) * TicksAFrame;
                         if ( Data->PanPos < -PanMax ) Data->PanPos = -PanMax;
                         Data->Flags |= CHAN_Pan | CHAN_Volume;
                        break;
                     case EFF_V_SlidePanRight:
                         Data->PanPos += VolumeEffectData * PanSpan / 255 / TicksAFrame;
                         if ( TicksAFrame > 1 && Tick == TicksAFrame - 1 ) Data->PanPos += VolumeEffectData * PanSpan / 255 - ( VolumeEffectData * PanSpan / 255 / TicksAFrame ) * TicksAFrame;
                         if ( Data->PanPos > PanMax ) Data->PanPos = PanMax;
                         Data->Flags |= CHAN_Pan | CHAN_Volume;
                        break;
                     case EFF_V_PanVibrato:
                        Data->PanbrelloSpeed = ( VolumeEffectData >> 4 ) + 1;
                        Data->PanbrelloSize = ( VolumeEffectData & 15 );
                        Data->PanShift += Data->PanbrelloSize * VibratoTypes[0][Data->PanbrelloPos%256] * PanSpan / 255 / 255;
                        Data->PanbrelloPos += ( 256 / Data->TremoloSpeed  * Multiple / 8 );
                        Data->Flags |= CHAN_Pan | CHAN_Volume;
                        break;
                     }
                  switch ( InstrumentEffect ) {
                     case EFF_I_None:
                        break;
                     case EFF_I_StopSample:
                        if ( Frame == 0 && Tick == 0 ) Data->Flags |= CHAN_Cut;
                        break;
                     case EFF_I_StopSampleLoop:
                        if ( Frame == 0 && Tick == 0 ) {
                           if ( Data->Instrument < 0 || Data->Instrument >= SampleInfoHeader->NoOfSamples ) break;
//                           ChanDat[Channel].NewLoopBegin = 0;
//                           ChanDat[Channel].NewLoopEnd = SampleInfos2[Data->Instrument]->Length;
//                           ChanDat[Channel].NewLoopFlags = 0;
//                           Data->Flags |= CHAN_NewLoop;
                           }
                        break;
                     case EFF_I_RestartSample:
                        if ( Frame == 0 && Tick == 0 ) {
                           Data->Volume = SampleInfos2[Data->Instrument]->Volume * VolMax / 255;
                           Data->FineTune = SampleInfos2[Data->Instrument]->Frequency;
                           ChanDat[Channel].SampleOffset = 0;
                           Data->Flags |= CHAN_Retrig;
                           Data->ReverseFlag = 0;
                           }
                        break;
                     case EFF_I_TriggerSample:
                        if ( ( InstrumentEffectData >> 4 ) < ( (Tick * Multiple) / 2 ) ) Data->Flags &= ~CHAN_Retrig;
                        else if ( (Tick * Multiple)%2 == 0 && ( InstrumentEffectData >> 4 ) == ( (Tick * Multiple) / 2 ) )
                        {
                            Data->ReverseFlag = 0;
                            Data->Flags |= CHAN_Retrig;
                            ChanDat[Channel].SampleOffset = 0;
                        }
                        break;
                     case EFF_I_TremoloRetrig:
                         if ( ((Tick * Multiple)/2)%(InstrumentEffectData >> 4) == 0 && (Tick * Multiple)%2 == 0 )
                         {
                             Data->ReverseFlag = 0;
                             ChanDat[Channel].SampleOffset = 0;
                             Data->Flags |= CHAN_Retrig;
                         }
                        break;
                     case EFF_I_SetSampleOffset:
                        if ( Frame == 0 && Tick == 0 ) {
                           if ( Data->Instrument < 0 || Data->Instrument >= SampleInfoHeader->NoOfSamples ) break;
                           if ( InstrumentEffectData * 256 > SampleInfos2[Data->Instrument]->Length ) break;
                           ChanDat[Channel].SampleOffset = InstrumentEffectData * 256;
                           }
                        break;
                     case EFF_I_ReverseSample:
                     ChangeDir:
                        if ( Data->Instrument < SampleInfoHeader->NoOfSamples )
                        {
                            if ( Data->ReverseFlag & 1 )
                            {
                                if ( SampleInfos2[Data->Instrument]->Flags & SMPL_Loop )
                                {
                                    ChanDat[Channel].NewLoopBegin = SampleInfos2[Data->Instrument]->LoopStart;
                                    ChanDat[Channel].NewLoopEnd = SampleInfos2[Data->Instrument]->LoopEnd;
                                    ChanDat[Channel].NewLoopFlags = INST_Loop;
                                } else {
                                    ChanDat[Channel].NewLoopBegin = 0;
                                    ChanDat[Channel].NewLoopEnd = SampleInfos2[Data->Instrument]->Length;
                                    ChanDat[Channel].NewLoopFlags = 0;
                                }
                            } else {
                                if ( SampleInfos2[Data->Instrument]->Flags & SMPL_Loop )
                                {
                                    ChanDat[Channel].NewLoopBegin = SampleInfos2[Data->Instrument]->LoopStart;
                                    ChanDat[Channel].NewLoopEnd = SampleInfos2[Data->Instrument]->LoopEnd;
                                    ChanDat[Channel].NewLoopFlags = INST_Loop | INST_Reverse;
                                } else {
                                    ChanDat[Channel].NewLoopBegin = 0;
                                    ChanDat[Channel].NewLoopEnd = SampleInfos2[Data->Instrument]->Length;
                                    ChanDat[Channel].NewLoopFlags = INST_Reverse;
                                }
                            }
                            
                            if ( ChanDat[Channel].NewLoopEnd > SampleInfos2[Data->Instrument]->Length )
                                ChanDat[Channel].NewLoopEnd = SampleInfos2[Data->Instrument]->Length;
                                
                            Data->Flags |= CHAN_NewLoop;
                            Data->ReverseFlag ^= 1;
                        }
                        break;
                     case EFF_I_RewindSample:
                        if ( Data->ReverseFlag & 2 )
                        {
                            Data->ReverseFlag &= ~2;
                            break;
                        }

                        Data->ReverseFlag |= 2;
                        goto ChangeDir;
                        break;
                     }

                  if ( InstrumentEffect != EFF_I_ReverseSample && ( Data->ReverseFlag & 2 ) )
                  {
                      Data->ReverseFlag &= ~2;
                      goto ChangeDir;
                  }
                  
                  switch ( NoteEffect ) {
                     case EFF_N_None:
                        break;
                     case EFF_N_SetFinetune:
                        if ( Tick == 0 ) Data->FineTune *= pow ( 2.0, (float) ( (signed char) NoteEffectData ) / 128.0 / 12.0 );
                        break;
                     case EFF_N_NoteDelay:
                        if ( ( NoteEffectData >> 4 ) / 8 < (Tick * Multiple) ) Data->Flags &= ~CHAN_Retrig;
                        else if ( ( NoteEffectData >> 4 ) / 8 == (Tick * Multiple) ) Data->Flags |= CHAN_Retrig;
                        break;
                     case EFF_N_Arpeggio:
                        if ( Data->ArpeggioCounter == 1 ) {
                           Data->PeriodShift = Data->Period / pow ( 1.059463094, NoteEffectData & 15 ) - Data->Period;
                           Data->Flags |= CHAN_Pitch;
                           } else if ( Data->ArpeggioCounter == 2 ) {
                           Data->PeriodShift = Data->Period / pow ( 1.059463094, NoteEffectData >> 4 ) - Data->Period;
                           Data->Flags |= CHAN_Pitch;
                           }
                        Data->ArpeggioCounter += Multiple;
                        if ( Data->ArpeggioCounter == 3 ) Data->ArpeggioCounter = 0;
                        break;
                     case EFF_N_SlideUp:
                        Data->Period /= pow ( 2.0, ( (float) NoteEffectData * Multiple / 16.0 / 12.0 / TicksAFrame ) );
                        if ( Data->Period < 57 ) Data->Period = 57;
                        Data->Flags |= CHAN_Pitch;
                        break;
                     case EFF_N_SlideDown:
                        Data->Period *= pow ( 2.0, ( (float) NoteEffectData * Multiple / 16.0 / 12.0 / TicksAFrame ) );
                        if ( Data->Period > 0x7FFF*64*4 ) Data->Period = 0x7FFF*64*4;
                        Data->Flags |= CHAN_Pitch;
                        break;
                     case EFF_N_Portamento:
                        if ( Data->PortaTarget < Data->Period ) {
                           Data->Period /= pow ( 2.0, ( (float) NoteEffectData * Multiple / 16.0 / 12.0 / TicksAFrame ) );
                           if ( Data->Period < Data->PortaTarget ) Data->Period = Data->PortaTarget;
                           Data->Flags |= CHAN_Pitch;
                           } else {
                           Data->Period *= pow ( 2.0, ( (float) NoteEffectData * Multiple / 16.0 / 12.0 / TicksAFrame ) );
                           if ( Data->Period > Data->PortaTarget ) Data->Period = Data->PortaTarget;
                           Data->Flags |= CHAN_Pitch;
                           }
                        break;
                     case EFF_N_ScratchPortamento:
                        if ( Data->PortaTarget < Data->Period ) {
                           Data->Period /= pow ( 2.0, ( (float) NoteEffectData * Multiple / 16.0 / 12.0 / TicksAFrame ) );
                           if ( Data->Period < Data->PortaTarget ) Data->Period = Data->PortaTarget;
                           Data->Flags |= CHAN_Pitch;
                           } else {
                           Data->Period *= pow ( 2.0, ( (float) NoteEffectData * Multiple / 16.0 / 12.0 / TicksAFrame ) );
                           if ( Data->Period > Data->PortaTarget ) Data->Period = Data->PortaTarget;
                           Data->Flags |= CHAN_Pitch;
                           }
                        Data->Period = Glissando ( Data->Period, (float) AMIGACONST * (float) (16777216/4) / (float) Data->FineTune );
                        break;
                     case EFF_N_VibratoSine:
                        Data->VibratoType = 0;
                        VibratoIn:
                        Data->VibratoSpeed = ( VolumeEffectData >> 4 ) + 1;
                        Data->VibratoSize = ( VolumeEffectData & 15 );
                        Data->VolumeShift += Data->Period * pow ( 2.0, Data->VibratoSize / 12.0 / 8.0 ) * VibratoTypes[Data->VibratoType%4][Data->VibratoPos%256] / 255 - Data->Period;
                        Data->VibratoPos += 256 / Data->VibratoSpeed * Multiple;
                        Data->Flags |= CHAN_Pitch;
                        break;
                     case EFF_N_VibratoTriangular:
                        Data->VibratoType = 2;
                        goto VibratoIn;
                        break;
                     case EFF_N_VibratoSquare:
                        Data->VibratoType = 1;
                        goto VibratoIn;
                        break;
                     case EFF_N_Tremolo:
                        if ( Data->NTremorPos >= ( NoteEffectData >> 4 ) && Data->NTremorPos < ( VolumeEffectData >> 4 ) + ( NoteEffectData >> 15 ) ) {
                           Data->Flags |= CHAN_Volume;
                           Data->VolumeShift = -10000;
                           }
                        Data->NTremorPos += Multiple;
                        if ( Data->NTremorPos >= ( NoteEffectData >> 4 ) + ( NoteEffectData >> 15 ) ) Data->NTremorPos -= ( ( NoteEffectData >> 4 ) + ( NoteEffectData >> 15 ) );
                        break;
                     case EFF_N_NoteCut:
                        if ( (Tick * Multiple)%2 == 0 && (Tick * Multiple) / 2 == ( NoteEffectData >> 4 ) ) Data->Flags |= CHAN_Cut;
                        break;
                     }

                  //Process channel variables and apply changes
                  if ( Data->Flags & CHAN_Instrument ) {
                     ChanDat[Channel].Sample = Data->Instrument;
                     }
                  if ( Data->Flags & CHAN_Pitch ) {
                     int NotePeriod;
                     NotePeriod = ( Data->Period + Data->PeriodShift );
                     if ( NotePeriod > 0x7FFF*64*4 ) NotePeriod = 0x7FFF*64*4;
                     if ( NotePeriod < 57 ) NotePeriod = 57;
                     ChanDat[Channel].Pitch = (8192*1712*64)/(NotePeriod);
                     }
                  if ( Data->Flags & CHAN_Volume ) {
                     int CurrentVolume, CurrentPan;

                     Data->Flags |= CHAN_Volume;

                     CurrentVolume = ( Data->Volume + Data->VolumeShift );
                     if ( CurrentVolume < 0 ) CurrentVolume = 0;
                     if ( CurrentVolume > VolMax ) CurrentVolume = VolMax;

                     CurrentPan = ( Data->PanPos + Data->PanShift );
                     if ( CurrentPan < -PanMax ) CurrentPan = -PanMax;
                     if ( CurrentPan > PanMax ) CurrentPan = PanMax;

                     ChanDat[Channel].MainVol = CurrentVolume;
                     ChanDat[Channel].LeftVol =  CurrentVolume * ( PanMax - CurrentPan ) / PanSpan;
                     ChanDat[Channel].RightVol = CurrentVolume * ( PanMax + CurrentPan ) / PanSpan;
                     ChanDat[Channel].Pan = CurrentPan;
                     }

                  ChanDat[Channel].Flags = Data->Flags;
                  Data->Flags = 0;
                  }
                
               // Let an output filter know of the current location.
               Device->SetFrameInfo(&FInf);
                  
               FInf.SongTime += FrameLength * Multiple * ( DelayTicks * 2 + 1 );
               if (SkipOrds == 0 && SkipRows == 0) {
                  long Rc = 0;
                  if ( ( Rc = Device->Compute ( &ChanDat, FrameLength * Multiple * ( DelayTicks * 2 + 1 ) ) ) != 0 ) {
                     ChanDat.free();
                     return Rc;
                     }
                  }
               DelayTicks = 0;
               }
            }
         if ( PatternDelay ) if ( --PatternDelay ) continue;
         if ( NewOrder != -1 ) break;
         else Row++;
         }
      if ( NewOrder == -1 ) CurOrder++;
         else CurOrder = NewOrder;
      }

ChanDat.free();
return PLAYOK;
}

