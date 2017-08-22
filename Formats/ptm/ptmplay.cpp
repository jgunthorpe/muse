// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   PTMFormat - Muse file format to handle PTM files.
   
   PTM is a little used format created by a tracker called PolyTracker.
   I heard a rumor that they dumped the project when Impulse came out.
   
   ##################################################################### */
									/*}}}*/
// Includes								/*{{{*/
#include <muse.h>
#include <ptmform.h>
#include <math.h>

#include "ptmstruc.h"
   									/*}}}*/

// PTMFormat::Play - Plays the song				
// ---------------------------------------------------------------------
/* */
long musePTMFormat::Play(museOutputBase *Device,musePlayerControl *Control)
{
   if ( ( Header == 0 ) || ( Header->NoOfOrders == 0 ) || ( Header->NoOfPatterns == 0 ) || ( Header->NoOfInstruments == 0 ) || ( Header->NoOfChannels == 0 ) )
      return 1;

   const signed char RetrigAdd[16] = { 0,-1,-2,-4,-8,-16, 0,0, 0, 1, 2, 4, 8,16, 0, 0};
   const signed char RetrigMul[16] = {16,16,16,16,16, 16,10,8,16,16,16,16,16,16,24,32};

   signed short VSineTable[256];
   signed short VSquare[256];
   signed short VRamp[256];
   signed short VRandom[256];
   signed short *VibratoTypes[4] = { VSineTable, VSquare, VRamp, VSineTable };

   CmdInfoPTM *ChannelData = new CmdInfoPTM[MAXNOCHANS];

   unsigned short CurOrder;
   char PatternDelay;
   unsigned char *LastPos;
   int x, z;
   int GlobalVolume, GlobalFlag, LastOrder, NewRow, LastRow;
   
   unsigned char EffectHistory[MAXNOCHANS][32];

   CurOrder = 0;
   LastPos = 0;
   PatternDelay = 0;
   GlobalVolume = 128;
   GlobalFlag = 0;
   LastOrder = -1;
   NewRow = -1;
   LastRow = -2;
   
   memset ( ChannelData, 0, MAXNOCHANS*sizeof ( CmdInfoPTM ) );
   memset ( EffectHistory, 0, MAXNOCHANS*32 );

   for ( x = 0; x < 256; x++ ) VSineTable[x] = (int)(256 * sin ( x * 3.14159265 / 128 ));
   for ( x = 0; x < 256; x++ ) VSquare[x] = ( x < 128 ? 128 : -128 );
   for ( x = 0; x < 256; x++ ) VRamp[x] = ( x - 128 ) * 2;
   for ( x = 0; x < 256; x++ ) VRandom[x] = (rand() % 510) - 255;

   // Definintions of tracking variables
   unsigned short FramesARow = 6;
   unsigned long FrameLength = Device->SecondsToLen ( 60.0 / ( 24.0 * ( ( float ) 128 ) ) );

   // Ennumerate the # of channels
   Sequencebool BoolSeq;
   BoolSeq.construct();
   BoolSeq.reserve(Header->NoOfChannels);

   for ( x = 0; x < Header->NoOfChannels; x++ ) BoolSeq[x] = 1;

   Device->SetChannelMask(&BoolSeq);
   Device->SetVolumeScale (0);
   BoolSeq.free();

   // Load the samples into the mixer.
   SequenceSample Samples;
   GetSongSamples(Samples);
   Device->LoadSamples(&Samples);
   Samples.free();

   SequenceChannel ChanDat;
   ChanDat.construct();
   ChanDat[Header->NoOfChannels].Flags = 0;
   for ( x = 0; x < Header->NoOfChannels; x++ ) {
      ChanDat[x].Sample = 0xFFFF;
      ChanDat[x].Flags = CHAN_Free;
      ChanDat[x].ModuleChannel = x;
      ChannelData[x].PanPos = Header->Panning[x];
      ChannelData[x].Flags |= CHAN_Pan;
//      printf ( "%d>%d\n", x, ChannelData[x].PanPos );
      }

   // Construct array to hold row bitmap
   PlayedRec *OrdPlayList = new PlayedRec[Header->NoOfOrders + 1];
   memset(OrdPlayList,0,(Header->NoOfOrders + 1)*sizeof(PlayedRec));
   museFrameInfo FInf;
   FInf.GlobalVolume = GlobalVolume;
   FInf.Tempo = 128;
   FInf.Speed = FramesARow;
   FInf.SongTime = 0;
   unsigned long SkipOrds = 0;
   unsigned long SkipRows = 0;

   while ( CurOrder < Header->NoOfOrders ) {
      int Row = 0;
      if ( NewRow != -1 ) {
         Row = NewRow;
         NewRow = -1;
         }
      int NewOrder = -1;

      if ( Header->Orders[CurOrder] == 254 || Header->Orders[CurOrder] >= Header->NoOfPatterns ) {
         CurOrder++;
         continue;
         }
      if ( Header->Orders[CurOrder] == 255 ) break;

      FInf.Order = CurOrder;
      FInf.Pattern = Header->Orders[CurOrder];

      // Smart Order Skipping
      if (SkipOrds != 0)
          SkipOrds--;
      
      while ( Row < 64 ) {
         int LoopRow = 0;
         int CurrentLoop = 0;

         // Check that this row has never been played before
         if (AllowLoop == false && (CurOrder) < Header->NoOfOrders)
         {
             if ((OrdPlayList[CurOrder].Rows[Row/32] & (1 << (Row % 32))) != 0)
             {
                 CurOrder = Header->NoOfOrders;
                 Trace(TRACE_INFO,"Aborting");
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
                         NRow += 64;
                         if (NRow >  0)
                             break;
                     }
                 }
                 else
                 {
                     while (1)
                     {
                         if (NRow < 64)
                             break;
                         NRow -= 64;
                         NewOrder++;
                         if (NewOrder > Header->NoOfOrders)
                         {
                             NRow = 0;
                             break;
                         }
                     }
                 }
                 NewRow = NRow;
                 
                 // Cut all the channels notes
                 for (x = 0; x < MAXNOCHANS; x++)
                 {
                      ChannelData[x].Flags &= ~CHAN_Retrig;
                      ChannelData[x].Flags |= CHAN_Cut;
                 }
                 
                 // Jump to start, reset all vars
                 if ((NewOrder == 0) && (NewRow == 0))
                 {
                     CurOrder = 0;
                     LastPos = 0;
                     PatternDelay = 0;
                     GlobalVolume = 128;
                     GlobalFlag = 0;
                     LastOrder = -1;
                     NewRow = -1;
                     LastRow = -2;
                     
                     memset ( ChannelData, 0, MAXNOCHANS*sizeof ( CmdInfoPTM ) );
                     memset ( EffectHistory, 0, MAXNOCHANS*32 );
                     
//                   unsigned short FramesARow = 6;
//                   unsigned long FrameLength = Device->SecondsToLen ( 60.0 / ( 24.0 * ( ( float ) 128 ) ) );
                     
                     for ( x = 0; x < Header->NoOfChannels; x++ )
                     {
                         ChanDat[x].Sample = 0xFFFF;
                         ChannelData[x].Flags = CHAN_Free | CHAN_Cut;
                         ChanDat[x].ModuleChannel = x;
                         ChannelData[x].PanPos = Header->Panning[x];
                     }
                     
                     FInf.GlobalVolume = GlobalVolume;
                     FInf.Tempo = 128;
                     FInf.Speed = 6;
                     FInf.SongTime = 0;
                     
                     memset(OrdPlayList,0,(Header->NoOfOrders + 1)*sizeof(PlayedRec));
                     SkipOrds = 0;
                     SkipRows = 0;
                     
                 }
                 break;
             }
         }
         
         //Get the pointer to the current row
         unsigned char *DataPointer;
         if ( LastOrder != CurOrder || LastRow != Row - 1 ) {
            DataPointer = Patterns[Header->Orders[CurOrder]];
            if ( Row != 0 ) {
               x = 0;
               while ( Row != x ) {
                  while ( *DataPointer ) {
                     z = *DataPointer++;
                     if ( z & PATTERN_Note ) DataPointer += 2;
                     if ( z & PATTERN_Effect ) DataPointer += 2;
                     if ( z & PATTERN_Volume ) DataPointer ++;
                     }
                  DataPointer++;
                  x++;
                  }
               }
            } else DataPointer = LastPos;

         for ( x = 0; x < Header->NoOfChannels; x++ ) {
            CmdInfoPTM *Data = &ChannelData[x];
            Data->NoteParm = 0;
            Data->InstrumentParm = 0;
            Data->VolumeParm = 0xFF;
            Data->EffectParm = 0;
            Data->EffectDataParm = 0;
            }

         while ( *DataPointer ) {
            z = *DataPointer++;
            CmdInfoPTM *Data = &ChannelData[( z & 31 )];
            if ( z & PATTERN_Note ) {
               Data->NoteParm = *DataPointer++;
               Data->InstrumentParm = *DataPointer++;
               }
            if ( z & PATTERN_Effect ) {
               Data->EffectParm = *DataPointer++;
               Data->EffectDataParm = *DataPointer++;
               }
            if ( z & PATTERN_Volume ) {
               Data->VolumeParm = *DataPointer++;
               }
            }
         LastPos = DataPointer++;

         //Loop through each frame
         for ( int Frame = 0; Frame < FramesARow; Frame++ ) {

            FInf.Frame = Frame;
             
            int Channel;
            for ( Channel = 0; Channel < Header->NoOfChannels; Channel++ ) {
               CmdInfoPTM *Data = &ChannelData[Channel];

               int Note = Data->NoteParm;
               int Instrument = Data->InstrumentParm;
               int Volume = Data->VolumeParm;
               int Effect = Data->EffectParm;
               int EffectData = Data->EffectDataParm;

               if ( Data->PeriodShift != 0 ) Data->Flags |= CHAN_Pitch;
               Data->PeriodShift = 0;
               if ( Data->VolumeShift != 0 ) Data->Flags |= CHAN_Volume;
               Data->VolumeShift = 0;

               if ( EffectData == 0 ) {
                  if ( Effect < 32 && EffectMap[Effect] ) EffectData = EffectHistory[Channel][Effect];
                  } else {
                  if ( Effect < 32 && EffectMap[Effect] ) EffectHistory[Channel][Effect] = EffectData;
                  }

               //Check for an instrument change
               if ( Instrument != 0 && Frame == 0 ) {
                  if ( Data->Instrument > Header->NoOfInstruments + 1 ) {
                     Instrument = 0;
                     Data->Flags |= CHAN_Cut;
                     } else {
                     Data->Instrument = Instrument - 1;
                     Data->Volume = Instruments[Data->Instrument]->Volume;
                     Data->BaseFrequency = Instruments[Data->Instrument]->BaseFrequency;
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

               //Check for a note change
               if ( Note == 254 && Frame == 0 ) {
                   Note = 0;
                   Data->Volume = 0;
//                   Data->Flags |= CHAN_Cut;
                   Data->Flags |= CHAN_Volume;
               }
               if ( Note && Note < 120 && Frame == 0 ) {
                   if ( Effect == EFF_Porta || Effect == EFF_DualPorta  ) {
                       Data->PortaTarget = (int)((float) 1712 * (float) 16777216 / Data->BaseFrequency / pow ( 2, (float) ( Note - 1 ) / 12.0 ));
                   } else if ( Effect == EFF_NoteSlideDown || Effect == EFF_NoteSlideDownR ) {
                       Data->PortaTarget = (int)((float) 1712 * (float) 16777216 / Data->BaseFrequency / pow ( 2, (float) ( Note - 1 - ( EffectData & 0xF ) ) / 12.0 ));
                   } else if ( Effect == EFF_NoteSlideUp || Effect == EFF_NoteSlideUpR ) {
                       Data->PortaTarget = (int)((float) 1712 * (float) 16777216 / Data->BaseFrequency / pow ( 2, (float) ( Note - 1 + ( EffectData & 0xF ) ) / 12.0 ));
                   } else {
                       Data->Note = Note - 1;
                       Data->Flags |= CHAN_Retrig | CHAN_Pan | CHAN_Pitch;
                       ChanDat[Channel].SampleOffset = 0;
                       Data->Period = (int)((float) 1712 * (float) 16777216 / Data->BaseFrequency / pow ( 2, (float) Data->Note / 12.0 ));
                   }
               }

               if ( Volume <= 64 && Frame == 0 ) {
                  Data->Volume = Volume;
                  Data->Flags |= CHAN_Volume;
                  }

               switch ( Effect ) {
                  case EFF_Arpeggio:
                     if ( EffectData != 0 ) {
                        if ( Data->ArpeggioCounter == 1 ) {
                           Data->PeriodShift = (int)(Data->Period / pow ( 1.059463094, EffectData & 15 ) - Data->Period);
                           Data->Flags |= CHAN_Pitch;
                           } else if ( Data->ArpeggioCounter == 2 ) {
                           Data->PeriodShift = (int)(Data->Period / pow ( 1.059463094, EffectData >> 4 ) - Data->Period);
                           Data->Flags |= CHAN_Pitch;
                           }
                        Data->ArpeggioCounter++;
                        if ( Data->ArpeggioCounter == 3 ) Data->ArpeggioCounter = 0;
                        }
                     break;
                  case EFF_SlideUp:
                     if ( EffectData & 0xF0 == 0xE0 ) {
                        if ( Frame == 0 ) Data->Period += ( EffectData & 0xF ) * 64;
                        } else if ( EffectData & 0xF0 == 0xF0 ) {
                        if ( Frame == 0 ) Data->Period += ( EffectData & 0xF ) * 4 * 64;
                        } else if ( Frame != 0 ) Data->Period += EffectData * 4 * 64;
                     Data->Flags |= CHAN_Pitch;
                     break;
                  case EFF_SlideDown:
                     if ( EffectData & 0xF0 == 0xE0 ) {
                        if ( Frame == 0 ) Data->Period -= ( EffectData & 0xF ) * 64;
                        } else if ( EffectData & 0xF0 == 0xF0 ) {
                        if ( Frame == 0 ) Data->Period -= ( EffectData & 0xF ) * 4 * 64;
                        } else if ( Frame != 0 ) Data->Period -= EffectData * 4 * 64;
                     Data->Flags |= CHAN_Pitch;
                     break;
                  case EFF_Porta:
                     Data->LastPorta = EffectData;
                     PortaIn:
                     if ( Frame != 0 && Data->PortaTarget ) {
                        if ( Data->Period > Data->PortaTarget ) {
                           Data->Period -= EffectData * 8 * 64;
                           if ( Data->Period < Data->PortaTarget ) Data->Period = Data->PortaTarget;
                           } else {
                           Data->Period += EffectData * 8 * 64;
                           if ( Data->Period > Data->PortaTarget ) Data->Period = Data->PortaTarget;
                           }
                        Data->Flags |= CHAN_Pitch;
                        }
                     break;
                  case EFF_Vibrato:
                     Data->VibratoSpeed = EffectData & 15;
                     Data->VibratoSize = ( EffectData >> 4 ) * 4;
                     VibratoIn:
                     Data->PeriodShift += VibratoTypes[Data->VibratoType%4][Data->VibratoPos%256] * ( Data->VibratoSize + 1 ) / 16;
                     if ( Frame == 0 ) break;
                     Data->VibratoPos += Data->VibratoSpeed * 4;
                     Data->Flags |= CHAN_Pitch;
                     break;
                  case EFF_VolumeSlide:
                  case EFF_DualVibrato:
                  case EFF_DualPorta:
                     if ( ( EffectData & 15 ) == 0xF ) {
                        if ( Frame == 0 ) Data->Volume += ( EffectData >> 4 );
                        } else if ( ( EffectData & ~15 ) == 0xF0 ) {
                        if ( Frame == 0 ) Data->Volume -= ( EffectData & 15 );
                        } else if ( ( EffectData & 15 ) == 0 ) {
                        if ( Frame != 0 ) Data->Volume += ( EffectData >> 4 );
                        } else if ( ( EffectData & ~15 ) == 0 ) {
                        if ( Frame != 0 ) Data->Volume -= ( EffectData & 15 );
                        }
                     boundv(0,Data->Volume,64);
                     Data->Flags |= CHAN_Volume;

                     if ( Effect == EFF_DualVibrato ) goto VibratoIn;
                     if ( Effect == EFF_DualPorta ) {
                        EffectData = Data->LastPorta;
                        goto PortaIn;
                        }
                  case EFF_Tremolo:
                     Data->TremoloSpeed = EffectData & 15;
                     Data->TremoloSize = EffectData >> 4;
                     Data->VolumeShift += VibratoTypes[Data->TremoloType%4][Data->TremoloPos%256] * ( Data->TremoloSize + 1 ) / 255 / 8;
                     if ( Frame == 0 ) break;
                     Data->TremoloPos += Data->TremoloSpeed * 4;
                     Data->Flags |= CHAN_Volume;
                     break;
                  case EFF_SetSampleOffset:
                     if ( Frame == 0 ) ChanDat[Channel].SampleOffset = EffectData * 0x100;
                     break;
                  case EFF_SetOrder:
                     if ( Header->NoOfOrders && Frame == 0 ) NewOrder = EffectData;
                     break;
                  case EFF_SetVolume:
                     if ( Frame == 0 ) {
                        if ( EffectData < 64 ) Data->Volume = EffectData;
                           else Data->Volume = 64;
                        Data->Flags |= CHAN_Volume;
                        }
                     break;
                  case EFF_BreakRow:
                     if ( Frame == 0 ) {
                        NewOrder = CurOrder + 1;
                        NewRow = EffectData;
                        }
                     break;
                  case EFF_Special:
                     switch ( EffectData >> 4 ) {
                        case EFF_Special_FineSlideDown:
                           if ( Frame == 0 ) Data->Period += ( EffectData & 0xF ) * 64 * 4;
                           break;
                        case EFF_Special_FineSlideUp:
                           if ( Frame == 0 ) Data->Period -= ( EffectData & 0xF ) * 64 * 4;
                           break;
                        case EFF_Special_SetVibratoWave:
                           if ( Frame == 0 ) Data->VibratoType = ( EffectData & 3 );
                           break;
                        case EFF_Special_SetFinetune:
                           if ( Data->Instrument < Header->NoOfInstruments ) {
                              Data->BaseFrequency = Instruments[Data->Instrument]->BaseFrequency * FineTuneTable[(EffectData & 0xF)] / 8363;
                              }
                           break;
                        case EFF_Special_PatternLoop:
                           if ( Frame == 0 ) {
                             if ( ( EffectData & 15 ) == 0 ) {
                                  LoopRow = Row;
                                  break;
                                  }
                               if ( CurrentLoop == 0 ) {
                                  CurrentLoop = EffectData & 15;
                                  NewRow = LoopRow;
                                  break;
                                  }
                               if ( --CurrentLoop ) {
                                  NewRow = LoopRow;
                                  }
                              }
                           break;
                        case EFF_Special_SetTremoloWave:
                           if ( Frame == 0 ) Data->TremoloType = ( EffectData & 3 );
                           break;
                        case EFF_Special_SetPanPosition:
                           if ( Frame == 0 ) {
                              Data->PanPos = ( EffectData & 15 );
                              Data->Flags |= CHAN_Pan;
                              }
                           break;
                        case EFF_Special_RetriggerNote:
                           if ( ( Frame % ( ( EffectData & 15 ) + 1 ) ) == 0 ) Data->Flags |= CHAN_Retrig;
                           break;
                        case EFF_Special_FineVolumeSlideUp:
                           if ( Frame == 0 ) Data->Volume += ( EffectData & 15 );
                           if ( Data->Volume > 64 ) Data->Volume = 64;
                           Data->Flags |= CHAN_Volume;
                           break;
                        case EFF_Special_FineVolumeSlideDown:
                           if ( Frame == 0 ) Data->Volume -= ( EffectData & 15 );
                           if ( Data->Volume < 0 ) Data->Volume = 0;
                           Data->Flags |= CHAN_Volume;
                           break;
                        case EFF_Special_NoteCut:
                           if ( Frame == ( EffectData & 15 ) ) {
//                              Data->Flags |= CHAN_Cut;
                              Data->Volume = 0;
                              Data->Flags |= CHAN_Volume;
                              }
                           break;
                        case EFF_Special_NoteDelay:
                           if ( Frame < ( EffectData & 15 ) ) Data->Flags &= ~CHAN_Retrig;
                           if ( Frame == ( EffectData & 15 ) ) Data->Flags |= CHAN_Retrig;
                           break;
                        case EFF_Special_PatternDelay:
                           if ( Frame == 0 ) if ( PatternDelay == 0 ) PatternDelay = EffectData & 15 + 1;
                           break;
                        }
                     break;
                  case EFF_SetSpeed:
                     if ( Frame == 0 ) {
                        if ( !EffectData ) break;
                        if ( EffectData < 32 ) {
                           FramesARow = EffectData;
                           FInf.Speed = EffectData;
                           } else {
                           FrameLength = Device->SecondsToLen ( 60.0 / ( 24.0 * ( ( float ) EffectData ) ) );
                           FInf.Tempo = EffectData;
                           }
                        }
                     break;
                  case EFF_SetGlobalVolume:
                     if ( EffectData < 128 ) GlobalVolume = EffectData;
                        else GlobalVolume = 128;
                     FInf.GlobalVolume = GlobalVolume;
                     GlobalFlag |= 1;
                     break;
                  case EFF_MultiRetrig:
                     if ( Frame == 0 ) {
                        if ( Data->RetrigCounter ) Data->RetrigCounter--;

                        if ( Data->RetrigCounter == 0 ) {
                           Data->RetrigCounter = EffectData & 0xF;
                           Data->Volume *= RetrigMul[EffectData >> 4];
                           Data->Volume /= 16;
                           Data->Volume += RetrigAdd[EffectData >> 4];
                           boundv(0,Data->Volume,64);
                           Data->Flags |= CHAN_Volume | CHAN_Retrig;
                           ChanDat[Channel].SampleOffset = 0;
                           }
                        }
                     break;
                  case EFF_FineVibrato:
                     if ( Frame != 0 ) {
                        Data->VibratoSpeed = EffectData & 15;
                        Data->VibratoSize = EffectData >> 4;
                        Data->PeriodShift += VibratoTypes[Data->VibratoType%4][Data->VibratoPos%256] * ( Data->VibratoSize + 1 ) / 16;
                        Data->VibratoPos += Data->VibratoSpeed * 4;
                        Data->Flags |= CHAN_Pitch;
                        }
                     break;
                  case EFF_NoteSlideDown:
                     if ( Frame != 0 ) Data->Period += ( EffectData >> 4 ) * 64 * 8;
                     if ( Data->Period > Data->PortaTarget ) Data->Period = Data->PortaTarget;
                     Data->Flags |= CHAN_Pitch;
                     break;
                  case EFF_NoteSlideUp:
                     if ( Frame != 0 ) Data->Period -= ( EffectData >> 4 ) * 64 * 8;
                     if ( Data->Period < Data->PortaTarget ) Data->Period = Data->PortaTarget;
                     Data->Flags |= CHAN_Pitch;
                     break;
                  case EFF_NoteSlideDownR:
                     if ( Frame != 0 ) Data->Period += ( EffectData >> 4 ) * 64 * 8;
                     if ( Data->Period > Data->PortaTarget ) Data->Period = Data->PortaTarget;
                     ChanDat[Channel].SampleOffset = 0;
                     Data->Flags |= CHAN_Pitch | CHAN_Retrig;
                     break;
                  case EFF_NoteSlideUpR:
                     if ( Frame != 0 ) Data->Period -= ( EffectData >> 4 ) * 64 * 8;
                     if ( Data->Period < Data->PortaTarget ) Data->Period = Data->PortaTarget;
                     ChanDat[Channel].SampleOffset = 0;
                     Data->Flags |= CHAN_Pitch | CHAN_Retrig;
                     break;
                  }

               if (SkipOrds != 0 || SkipRows != 0) continue;
               
               //Process channel variables and apply changes
               if ( Data->Flags & CHAN_Instrument ) {
                  ChanDat[Channel].Sample = Data->Instrument;
                  }
               if ( Data->Flags & CHAN_Pitch ) {
                  int NotePeriod;
                  NotePeriod = ( Data->Period + Data->PeriodShift );
                  if ( NotePeriod > 0x7FFF * 64 * 4 ) NotePeriod = 0x7FFF * 64 * 4;
                  if ( NotePeriod < 57 * 64 ) NotePeriod = 57 * 64;
                  ChanDat[Channel].Pitch = (8192*1712*64*2)/(NotePeriod);
                  }
               if ( Data->Flags & ( CHAN_Volume | CHAN_Pan ) || ( GlobalFlag & 1 ) ) {
                  int CurrentVolume;

                  int CurrentPan = ( 2 * ( Data->PanPos & 15 ) - 0xF ) * ( PanSpan/2 ) / 0xF;
                  boundv(-PanMax,CurrentPan,PanMax);
                  
                  if ( GlobalFlag ) Data->Flags |= CHAN_Volume;

                  CurrentVolume = ( ( Data->Volume + Data->VolumeShift ) * GlobalVolume ) * VolMax / 64 / 128;
                  boundv(0,CurrentVolume,VolMax);

                  ChanDat[Channel].MainVol = CurrentVolume;
                  ChanDat[Channel].Pan = CurrentPan;
                  ChanDat[Channel].LeftVol = ( CurrentVolume * ( PanMax - CurrentPan ) ) / PanSpan;
                  ChanDat[Channel].RightVol = ( CurrentVolume * ( PanMax + CurrentPan ) ) / PanSpan;
                  }

               if ( SkipOrds == 0 && SkipRows == 0 ) {
                  ChanDat[Channel].Flags = Data->Flags;
                  Data->Flags = 0;
                  }

               if ( Frame == 1 ) GlobalFlag &= ~1;
               }

            // Let an output filter know of the current location.
            Device->SetFrameInfo(&FInf);
            
            FInf.SongTime += FrameLength;

            if (SkipOrds == 0 && SkipRows == 0) {
               if ( Device->Compute ( &ChanDat, FrameLength ) != 0 ) {
                  ChanDat.free();
                  delete [] ChannelData;
                  return 0;
                  }
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
delete [] ChannelData;
return 0;
}

