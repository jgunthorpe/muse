// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   ITFormat - Muse file format to handle IT files.
   
   This player handles Impulse tracker files.
   
   Check the loop changing code.
   
   ##################################################################### */
									/*}}}*/
// Includues								/*{{{*/
#include <muse.h>
#include <itform.h>
#include <math.h>

#include "itstruc.h"
   									/*}}}*/

// ITFormat::Play - Plays the song				
// ---------------------------------------------------------------------
/* */
long museITFormat::Play(museOutputBase *Device,musePlayerControl *Control)
{
   if ( ( Header == 0) || ( Header->NoOfOrders == 0 ) || ( Header->NoOfPatterns == 0 ) || ( Header->NoOfSamples == 0) )
      return PLAYFAIL_NotLoaded;

   const signed char RetrigAdd[16] = { 0,-1,-2,-4,-8,-16, 0,0, 0, 1, 2, 4, 8,16, 0, 0};
   const signed char RetrigMul[16] = {16,16,16,16,16, 16,10,8,16,16,16,16,16,16,24,32};
   const unsigned char EffectMap[256] = {
 //    0  A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P  Q  R   S   T  U   V  W   X  Y   Z
       0, 0, 0, 0, 1, 2, 2, 2, 3, 4, 5, 1, 1, 0, 6, 7, 8, 9, 10, 11, 0, 12, 0, 13, 0, 14, 0 };
   
   if (MaxNoOfChannels == 0)
      MaxNoOfChannels = Device->GetMaxChannels();

   if ( MaxNoOfChannels > MAXNOCHANS ) MaxNoOfChannels = MAXNOCHANS;
   
   signed short VSineTable[256];
   signed short VSquare[256];
   signed short VRamp[256];
   signed short VRandom[256];
   signed short *VibratoTypes[4] = { VSineTable, VSquare, VRamp, VRandom };

   CmdInfoIT *ChannelData = new CmdInfoIT[PATTERNCHANS];

   unsigned char LastByte[PATTERNCHANS];
   unsigned char LastNote[PATTERNCHANS];
   unsigned char LastInstrument[PATTERNCHANS];
   unsigned char LastVolume[PATTERNCHANS];
   unsigned char LastEffect[2][PATTERNCHANS];
   unsigned char *DataPointer = 0;

   CmdInfoIT *MainChannelData[MAXNOCHANS];

   #define EFFECTMAPSIZE 14
   unsigned char EffectHistory[PATTERNCHANS][EFFECTMAPSIZE];

   char PatternDelay;
   int v, x, y, z;
   int GlobalVolume, GlobalFlag, NewOrder, LastOrder, LastRow, NewRow, LoopRow, CurrentLoop;
   unsigned short CurOrder;
   unsigned char *LastPos = 0;

   GlobalVolume = Header->GlobalVolume * VolMax / 128;
   GlobalFlag = 0;
   PatternDelay = 0;
   CurOrder = 0;
   NewOrder = -1;
   LastPos = 0;
   LastOrder = -1;
   LastRow = -1;
   NewRow = -1;
   LoopRow = 0;
   CurrentLoop = 0;
   
   memset ( ChannelData, 0, PATTERNCHANS*sizeof ( CmdInfoIT ) );
   memset ( MainChannelData, 0, MAXNOCHANS*sizeof ( CmdInfoIT * ) );
   memset ( EffectHistory, 0, PATTERNCHANS*EFFECTMAPSIZE );

   for ( x = 0; x < 256; x++ ) VSineTable[x] = (int)(256 * sin ( x * 3.14159265 / 128 ));
   for ( x = 0; x < 256; x++ ) VSquare[x] = ( x < 128 ? 128 : -128 );
   for ( x = 0; x < 256; x++ ) VRamp[x] = ( x - 128 ) * 2;
   for ( x = 0; x < 256; x++ ) VRandom[x] = (rand() % 510) - 255;

   // Definintions of tracking variables
   unsigned short FramesARow = Header->InitialSpeed;
   unsigned long FrameLength = Device->SecondsToLen ( 125 / ( 50.0 * ( ( float ) Header->InitialTempo ) ) );
   if ( FramesARow == 0 ) FramesARow = 6;

   // Ennumerate the # of channels
   Sequencebool BoolSeq;
   BoolSeq.construct();
   BoolSeq.reserve(MaxNoOfChannels);

   for ( x = 0; x < MaxNoOfChannels; x++ )
      BoolSeq[x] = 1;

   Device->SetChannelMask(&BoolSeq);
   Device->SetVolumeScale (0);
   BoolSeq.free();

   // Load the samples into the mixer.
   SequenceSample Samples_;
   GetSongSamples(Samples_);
   Device->LoadSamples(&Samples_);
   Samples_.free();

   SequenceChannel ChanDat;
   ChanDat.construct();
   ChanDat.reserve(MaxNoOfChannels - 1);
   for ( x = 0; x < MaxNoOfChannels; x++ )
   {
      ChanDat[x].Sample = 0xFFFF;
      ChanDat[x].Flags = CHAN_Free;
   }

   for ( x = 0; x < PATTERNCHANS; x++ )
   {
      ChannelData[x].ChannelVolume = Header->ChannelVolume[x] * VolMax / 64;
      if ( Header->ChannelPan[x] & 128 ) ChannelData[x].Muted = 1;
      ChannelData[x].PanPos = ( ( Header->ChannelPan[x] & 127 ) * PanSpan / 64) - PanMax;
      boundv(-PanMax,ChannelData[x].PanPos,PanMax);
      ChannelData[x].Flags |= CHAN_Volume | CHAN_Pan;
   }

   // Construct array to hold row bitmap
   PlayedRec *OrdPlayList = new PlayedRec[Header->NoOfOrders + 1];
   memset(OrdPlayList,0,(Header->NoOfOrders + 1)*sizeof(PlayedRec));
   museFrameInfo FInf;
   FInf.GlobalVolume = GlobalVolume;
   FInf.Tempo = Header->InitialTempo;
   FInf.Speed = Header->InitialSpeed;
   FInf.SongTime = 0;
   unsigned long SkipOrds = 0;
   unsigned long SkipRows = 0;

   while ( CurOrder < Header->NoOfOrders ) {
      int Row = 0, CurRow = 0;
      if ( NewRow != -1 ) {
         Row = NewRow;
         NewRow = -1;
         }
      NewOrder = -1;

      if ( Orders[CurOrder] == 254 ) {
         CurOrder++;
         continue;
         }
      if ( Orders[CurOrder] == 255 ) break;

      FInf.Order = CurOrder;
      FInf.Pattern = Orders[CurOrder];

      // Smart Order Skipping
      if (SkipOrds != 0)
          SkipOrds--;
      
      while ( ( !Patterns[Orders[CurOrder]] && Row < 64 ) || ( Patterns[Orders[CurOrder]] && Orders[CurOrder] < Header->NoOfPatterns && Row < Patterns[Orders[CurOrder]]->Rows ) ) {

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
                         NRow += Patterns[Orders[NewOrder]]->Rows;
                         if (NRow >  0)
                             break;
                     }
                 }
                 else
                 {
                     while (1)
                     {
                         if (NRow < Patterns[Orders[NewOrder]]->Rows)
                             break;
                         NRow -= Patterns[Orders[NewOrder]]->Rows;
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
                 for (x = 0; x < MaxNoOfChannels; x++)
                 {
                     if ( MainChannelData[x] )
                     {
                         MainChannelData[x]->Flags &= ~CHAN_Retrig;
                         MainChannelData[x]->Flags |= CHAN_Cut;
                     }
                 }
                 
                 // Jump to start, reset all vars
                 if ((NewOrder == 0) && (NewRow == 0))
                 {
                     GlobalVolume = Header->GlobalVolume * VolMax / 128;
                     GlobalFlag = 0;
                     PatternDelay = 0;
                     NewOrder = 0;
                     NewRow = 0;
                     LoopRow = 0;
                     CurrentLoop = 0;

                     memset ( ChannelData, 0, PATTERNCHANS*sizeof ( CmdInfoIT ) );
                     memset ( MainChannelData, 0, MAXNOCHANS*sizeof ( CmdInfoIT * ) );
                     memset ( EffectHistory, 0, PATTERNCHANS*EFFECTMAPSIZE );

                     unsigned short FramesARow = Header->InitialSpeed;
//                     unsigned long FrameLength = Device->SecondsToLen ( 125 / ( 50.0 * ( ( float ) Header->InitialTempo ) ) );
                     if ( FramesARow == 0 ) FramesARow = 6;

                     for ( x = 0; x < MaxNoOfChannels; x++ )
                     {
                         ChanDat[x].Sample = 0xFFFF;
                         ChanDat[x].Flags = CHAN_Free | CHAN_Cut;
                     }

                     for ( x = 0; x < PATTERNCHANS; x++ )
                     {
                         ChannelData[x].ChannelVolume = Header->ChannelVolume[x] * VolMax / 64;
                         if ( Header->ChannelPan[x] & 128 ) ChannelData[x].Muted = 1;
                         ChannelData[x].PanPos = ( ( Header->ChannelPan[x] & 127 ) * PanSpan / 64) - PanMax;
                         boundv(-PanMax,ChannelData[x].PanPos,PanMax);
                     }
                     
                     FInf.GlobalVolume = GlobalVolume;
                     FInf.Tempo = Header->InitialTempo;
                     FInf.Speed = FramesARow;
                     FInf.SongTime = 0;

                     memset(OrdPlayList,0,(Header->NoOfOrders + 1)*sizeof(PlayedRec));
                     SkipOrds = 0;
                     SkipRows = 0;
                     
                 }
                 break;
             }
         }
         
         //Get the pointer to the current row
         if ( ( LastOrder != CurOrder || LastRow != Row - 1 ) && !PatternDelay ) {

            memset ( LastByte, 0, PATTERNCHANS );
            memset ( LastNote, 120, PATTERNCHANS );
            memset ( LastInstrument, 0, PATTERNCHANS );
            memset ( LastVolume, 255, PATTERNCHANS );
            memset ( LastEffect, 0, 2*PATTERNCHANS );

            if ( LastOrder == CurOrder && LastRow < Row ) CurRow = LastRow + 1;

            if ( CurRow == 0 ) {
               if ( Patterns[Orders[CurOrder]] ) DataPointer = &Patterns[Orders[CurOrder]]->Data;
               else DataPointer = 0;
               }

	    LastOrder = CurOrder;
            }

         unsigned char Notev[PATTERNCHANS];
         unsigned char Instrumentv[PATTERNCHANS];
         unsigned char Volumev[PATTERNCHANS];
         unsigned char Effectv[PATTERNCHANS];
         unsigned char EffectDatav[PATTERNCHANS];
         unsigned char ChannelMap[PATTERNCHANS];

         if ( DataPointer ) while ( CurRow <= Row ) {
            //For each channel, calculate the parameters
            if ( CurRow == Row )
            {
                memset ( Notev, 120, PATTERNCHANS );
                memset ( Instrumentv, 0, PATTERNCHANS );
                memset ( Volumev, 255, PATTERNCHANS );
                memset ( Effectv, 0, PATTERNCHANS );
                memset ( EffectDatav, 0, PATTERNCHANS );
                memset ( ChannelMap, 0, PATTERNCHANS );
            }
             
            while ( *DataPointer ) {
               char Channel, Mask;
               Channel = *DataPointer++;
               if ( Channel & 128 ) {
                  Mask = *DataPointer++;
                  LastByte[Channel & 63] = Mask;
                  } else Mask = LastByte[Channel & 63];
               Channel = ( Channel & 63 ) - 1;

               ChannelMap[Channel] = 1;
 	       if ( Mask & 1 ) {
                  Notev[Channel] = LastNote[Channel] = *DataPointer++;
                  } else if ( ( Mask & 16 ) ) {
                  Notev[Channel] = LastNote[Channel];
                  }

               if ( Mask & 2 ) {
                  Instrumentv[Channel] = LastInstrument[Channel] = *DataPointer++;
                  } else if ( ( Mask & 32 ) ) {
                  Instrumentv[Channel] = LastInstrument[Channel];
                  }

               if ( Mask & 4 ) {
                  Volumev[Channel] = LastVolume[Channel] = *DataPointer++;
                  } else if ( ( Mask & 64 ) ) {
                  Volumev[Channel] = LastVolume[Channel];
                  }

               if ( Mask & 8 ) {
                  Effectv[Channel]     = LastEffect[0][Channel] = *DataPointer++;
                  EffectDatav[Channel] = LastEffect[1][Channel] = *DataPointer++;
                  } else if ( ( Mask & 128 ) ) {
                  Effectv[Channel]     = LastEffect[0][Channel];
                  EffectDatav[Channel] = LastEffect[1][Channel];
                  } else {
                  Effectv[Channel] = 0;
                  EffectDatav[Channel] = 0;
                  }
               }
            DataPointer++;

            CurRow++;
            }
         LastRow = Row;

         //Loop through each frame
         for ( int Frame = 0; Frame < FramesARow; Frame++ ) {
            int Channel;
            FInf.Frame = Frame;

            for ( Channel = 0; Channel < MaxNoOfChannels; Channel++ ) {
               CmdInfoIT *Data = MainChannelData[Channel];

               if ( !Data || Data->Muted ) continue;

               if ( Data->PeriodShift != 0 ) Data->Flags |= CHAN_Pitch;
               Data->PeriodShift = 0;
               if ( Data->VolumeShift != 0 ) Data->Flags |= CHAN_Volume;
               Data->VolumeShift = 0;
               if ( Data->PanShift != 0 ) Data->Flags |= CHAN_Pan;
               Data->PanShift = 0;

	       }

            for ( Channel = 0; Channel < min ( PATTERNCHANS, MaxNoOfChannels ); Channel++ )
            if ( ChannelMap[Channel] ) {
               CmdInfoIT *Data = &ChannelData[Channel];

               int Note       = Notev[Channel];
               int Instrument = Instrumentv[Channel];
               int Volume     = Volumev[Channel];
               int Effect     = Effectv[Channel];
               int EffectData = EffectDatav[Channel];

               if ( EffectData == 0 ) {
                  if ( EffectMap[Effect] && EffectMap[Effect] <= EFFECTMAPSIZE ) {
                     EffectData = EffectHistory[Channel][EffectMap[Effect]-1];
                     }
                  } else {
                  if ( EffectMap[Effect] && EffectMap[Effect] <= EFFECTMAPSIZE ) {
                     EffectHistory[Channel][EffectMap[Effect]-1] = EffectData;
                     }
                  }

               if ( Note == 120 && Instrument && ( Data->Instrument != (Instrument - 1) || ( Data->Channel && ChanDat[Data->Channel-1].Flags & CHAN_Free ) ) ) Note = Data->Note;

               register unsigned char NoteDelay = ( Effect == EFF_Special && ( EffectData >> 4 ) == EFF_SPECIAL_NoteDelay );
               unsigned char NoteNow = !PatternDelay && ( ( Frame == 0 && !NoteDelay ) || ( NoteDelay && Frame == ( EffectData & 15 ) ) );

	       //Check for a note change
               if ( Note == 255 && NoteNow ) {
                  Note = 0;
                  Data->KeyState &= ~1;
//                  Instrument = 0;
                  }

               if ( Note == 254 && NoteNow ) {
                  Note = 0;
                  Data->Flags |= CHAN_Cut | CHAN_Free;
                  Data->Flags &= ~CHAN_Retrig;
//                  Instrument = 0;
                  }

               if ( Note && Note < 120 && NoteNow ) {
                  Data->Parent = Channel;
                  if ( ( Header->Flags & 4 ) && Data->Channel && !( ( Effect == EFF_Porta || Effect == EFF_DualPorta ) && Data->Period ) ) {
                     if ( Header->Compatability >= 0x200 ) {
                        switch ( Instruments2[Data->Instrument]->DuplicateCheckType ) {
                           case DCT_Off:
                              break;
                           case DCT_Note:
                              for ( x = 0; x < PATTERNCHANS; x++ ) {
                                 if ( Data->Channels[x] && MainChannelData[Data->Channels[x] - 1]->Parent == Channel && MainChannelData[Data->Channels[x] - 1]->Note == Note ) {
                                    if ( MainChannelData[Data->Channels[x] - 1] && MainChannelData[Data->Channels[x] - 1] != Data ) {
                                       switch ( Instruments2[Data->Instrument]->DuplicateCheckAction ) {
                                          case DCA_Cut:
                                             MainChannelData[Data->Channels[x] - 1]->Flags |= CHAN_Cut;
                                             MainChannelData[Data->Channels[x] - 1]->Flags &= ~CHAN_Retrig;
                                             break;
                                          case DCA_Noteoff:
          	        	             MainChannelData[Data->Channels[x] - 1]->KeyState &= ~1;
                                             break;
                                          case DCA_Notefade:
          	        	             MainChannelData[Data->Channels[x] - 1]->KeyState &= ~2;
                                             break;
                                          }
                                       }
                                    Data->Channels[x] = 0;
                                    break;
                                    }
                                 }
                              break;
                           case DCT_Sample:
                              for ( x = 0; x < PATTERNCHANS; x++ ) {
                                 if ( Data->Channels[x] && MainChannelData[Data->Channels[x] - 1]->Parent == Channel && MainChannelData[Data->Channels[x] - 1]->Sample == Instruments[Data->Instrument]->Mapping[Data->Note][1] - 1 ) {
                                    if ( MainChannelData[Data->Channels[x] - 1] && MainChannelData[Data->Channels[x] - 1] != Data ) {
                                       switch ( Instruments2[Data->Instrument]->DuplicateCheckAction ) {
                                          case DCA_Cut:
                                             MainChannelData[Data->Channels[x] - 1]->Flags |= CHAN_Cut;
                                             MainChannelData[Data->Channels[x] - 1]->Flags &= ~CHAN_Retrig;
                                             break;
                                          case DCA_Noteoff:
          	        	             MainChannelData[Data->Channels[x] - 1]->KeyState &= ~1;
                                             break;
                                          case DCA_Notefade:
          	        	             MainChannelData[Data->Channels[x] - 1]->KeyState &= ~2;
                                             break;
                                          }
                                       }
                                    Data->Channels[x] = 0;
                                    break;
                                    }
                                 }
                              break;
                           case DCT_Instrument:
                              for ( x = 0; x < PATTERNCHANS; x++ ) {
                                 if ( Data->Channels[x] && MainChannelData[Data->Channels[x] - 1]->Parent == Channel && MainChannelData[Data->Channels[x] - 1]->Instrument == Instrument - 1 ) {
                                    if ( MainChannelData[Data->Channels[x] - 1] && MainChannelData[Data->Channels[x] - 1] != Data ) {
                                       switch ( Instruments2[Data->Instrument]->DuplicateCheckAction ) {
                                          case DCA_Cut:
                                             MainChannelData[Data->Channels[x] - 1]->Flags |= CHAN_Cut;
                                             MainChannelData[Data->Channels[x] - 1]->Flags &= ~CHAN_Retrig;
                                             break;
                                          case DCA_Noteoff:
          	        	             MainChannelData[Data->Channels[x] - 1]->KeyState &= ~1;
                                             break;
                                          case DCA_Notefade:
          	        	             MainChannelData[Data->Channels[x] - 1]->KeyState &= ~2;
                                             break;
                                          }
                                       }
                                    Data->Channels[x] = 0;
                                    break;
                                    }
                                 }
                              break;
                           }
                        } else {
                        if ( Instruments[Data->Instrument]->DuplicateNoteCheck ) {
                           for ( x = 0; x < PATTERNCHANS; x++ ) {
                              if ( Data->Channels[x] && MainChannelData[Data->Channels[x] - 1]->Parent == Channel && MainChannelData[Data->Channels[x] - 1]->Note == Note && MainChannelData[Data->Channels[x] - 1]->Instrument == Data->Instrument ) {
                                 if ( MainChannelData[Data->Channels[x] - 1] && MainChannelData[Data->Channels[x] - 1] != Data ) {
                                    MainChannelData[Data->Channels[x] - 1]->Flags |= CHAN_Cut;
                                    MainChannelData[Data->Channels[x] - 1]->Flags &= ~CHAN_Retrig;
                                    }
                                 Data->Channels[x] = 0;
                                 break;
                                 }
                              }
                           }
                        }

                     switch ( Data->NewNoteAction ) {
                        case NNA_Cut: //Note cut
                           break;
                        case NNA_Continue:
                           if ( Data->Channel ) {

                              ChanDat[Data->Channel - 1].Flags &= CHAN_Surround;
                              ChanDat[Data->Channel - 1].MainVol = VolMax;

                              ContinueIn:
                              for ( x = 0; x < PATTERNCHANS; x++ ) if ( !Data->Channels[x] ) break;
                              if ( x == PATTERNCHANS ) {
                                 memmove ( Data->Channels + 1, Data->Channels, PATTERNCHANS-1 );
                                 x = 0;
                                 }
                              Data->Channels[x] = Data->Channel;
                              if ( MainChannelData[Data->Channel - 1] && MainChannelData[Data->Channel - 1]->Channel == 0 && MainChannelData[Data->Channel - 1]->Child ) free ( MainChannelData[Data->Channel - 1] );
                              MainChannelData[Data->Channel - 1] = (CmdInfoIT *) malloc ( sizeof ( CmdInfoIT ) );
                              memcpy ( (void *) MainChannelData[Data->Channel - 1], (void *) &ChannelData[Channel], sizeof ( CmdInfoIT ) );
                              MainChannelData[Data->Channel - 1]->Channel = 0;
                                  
                              MainChannelData[Data->Channel - 1]->Child = 1;
                              Data->Channel = 0;
                              }
                           break;
                        case NNA_Noteoff: //Note off
                           if ( Data->Channel ) {

                              MainChannelData[Data->Channel - 1]->KeyState &= ~1;
                              ChanDat[Data->Channel - 1].Flags &= CHAN_Surround;
                              ChanDat[Data->Channel - 1].MainVol = VolMax;

                              goto ContinueIn;
                              }
                        case NNA_Notefade: //Note fade
                           if ( Data->Channel ) {
                              MainChannelData[Data->Channel - 1]->KeyState &= ~2;
                              ChanDat[Data->Channel - 1].Flags &= CHAN_Surround;
                              ChanDat[Data->Channel - 1].MainVol = VolMax;

                              goto ContinueIn;
                              }
                        }
                     }

                  Data->Note = Note;
                  if ( Data->Channel == 0 ) {
                     for ( x = 0; x < MaxNoOfChannels; x++ ) {
                        if ( ChanDat[x].Flags & CHAN_Free ) {
                           for ( y = 0; y < PATTERNCHANS; y++ ) if ( ChannelData[y].Channel - 1 == x ) ChannelData[y].Channel = 0;
                           break;
                           }
                        }
                     if ( x != MaxNoOfChannels ) {
                        Data->Channel = x + 1;
                        ChanDat[x].Flags &= CHAN_Surround;
                        ChanDat[x].MainVol = VolMax;
                        if ( MainChannelData[x] && MainChannelData[x]->Channel == 0 && MainChannelData[Data->Channel - 1]->Child ) free ( MainChannelData[x] );
                        MainChannelData[x] = Data;
                     } else {
                        Counter Counters[MAXNOCHANS];
                        int Counted = 0;

                        for ( x = 0; x < MaxNoOfChannels; x++ ) {
                           if ( MainChannelData[x] ) {
                              for ( y = 0; y < Counted; y++ ) {
                                 if ( Counters[y].Sample == MainChannelData[x]->Sample ) break;
                                 }
                              if ( y == Counted ) {
                                 Counted++;
                                 Counters[Counted-1].Sample = MainChannelData[x]->Sample;
                                 Counters[Counted-1].Count = 1;
                                 Counters[Counted-1].Background = 0;
                                 if ( MainChannelData[x]->Channel == 0 ) {
                                    Counters[Counted-1].Background++;
                                    Counters[Counted-1].Channel = x;
                                    Counters[Counted-1].MinVol = ChanDat[x].MainVol;
                                    } else {
                                    Counters[Counted-1].Channel = -1;
                                    Counters[Counted-1].MinVol = VolMax+1;
                                    }
                                 } else {
                                 Counters[y].Count++;
                                 if ( MainChannelData[x]->Channel == 0 ) {
                                    Counters[y].Background++;
                                    if ( ChanDat[x].MainVol < Counters[y].MinVol ) {
                                       Counters[y].Channel = x;
                                       Counters[y].MinVol = ChanDat[x].MainVol;
                                       }
                                    }
                                 }
                              }
                           }
                        y = -1;
                        z = 1;
                        for ( x = 0; x < Counted; x++ ) {
                           if ( Counters[x].Count > z && Counters[x].Background >= 2 && Counters[x].Channel >= 0 ) {
                              y = x;
                              z = Counters[x].Count;
                              }
                           }
                        if ( y >= 0 ) {
                           y = Counters[y].Channel;
	     		   goto GotChannel;
                           }

                        x = 0x10000;
                        y = -1;
                        for ( z = 0; z < MaxNoOfChannels; z++ ) {
                           if ( MainChannelData[z]->Channel == 0 && ChanDat[z].MainVol < x ) {
                              y = z;
                              x = ChanDat[z].MainVol;
                              }
                           }

     			GotChannel:
                        if ( y >= 0 && MainChannelData[y] && MainChannelData[y]->Channel == 0 ) {
                           if ( ChannelData[MainChannelData[y]->Parent].Channel == y + 1 ) ChannelData[MainChannelData[y]->Parent].Channel = 0;
                           for ( z = 0; z < PATTERNCHANS; z++ ) if ( ChannelData[MainChannelData[y]->Parent].Channels[z] == y + 1 ) ChannelData[MainChannelData[y]->Parent].Channels[z] = 0;
                           free ( MainChannelData[y] );
                           ChanDat[y].Flags &= CHAN_Surround;
                           ChanDat[y].MainVol = VolMax;
                           MainChannelData[y] = Data;
                           Data->Channel = y + 1;
                           Data->Child = 0;
                           } else Data->Channel = 0;
                        }
                     }

                  if ( Data->Channel ) {
                     if ( ( Effect == EFF_Porta || Effect == EFF_DualPorta ) && Data->Period && ( ChanDat[Data->Channel-1].Flags & CHAN_Free ) ) {
                        Data->Flags |= CHAN_Retrig;
                        Data->Flags &= ~CHAN_Cut;
                        ChanDat[Data->Channel-1].SampleOffset = 0;
                        Data->VolEnvelopePos = 0;
                        Data->PanEnvelopePos = 0;
                        Data->PitchEnvelopePos = 0;
                        Data->KeyState = 3;
                        }
                  
                     ChanDat[Data->Channel - 1].MainVol = VolMax;
                     ChanDat[Data->Channel - 1].Flags &= ~CHAN_Free;
                     Data->Child = 0;
                     }
                  }

               //Set up the instrument
               if ( Header->Flags & 4 ) {
                   if ( Instrument != 0 && NoteNow ) {
                       if ( Instrument > Header->NoOfInstruments ) {
                           Instrument = 0;
                       } else {
                           Data->Instrument = Instrument - 1;
                       }
                   }
               } else {
                   if ( Instrument != 0 && NoteNow ) {
                       if ( Instrument > Header->NoOfSamples ) {
                           Instrument = 0;
                       } else {
                           Data->Instrument = Data->Sample = Instrument - 1;
                       }
                   }
               }
               
               if ( Note && Note < 120 && NoteNow ) {
                  if ( ( Header->Flags & 4 ) && Instrument > 0 ) {
                     Data->Sample = Instruments[Data->Instrument]->Mapping[Data->Note][1] - 1;
                     if ( Data->Sample < 0 || Data->Sample >= Header->NoOfSamples ) {
                        Data->Sample = 0;
                        Data->Flags |= CHAN_Cut;
                        Data->Flags &= ~CHAN_Retrig;
                        } else {
                        Data->Flags |= CHAN_Instrument;
                        }
                     }
                  if ( ( Effect == EFF_Porta || Effect == EFF_DualPorta ) && Data->Period ) {
/*                     if ( Data->Channel && ( ChanDat[Data->Channel-1].Flags & CHAN_Free ) ) {
                        Data->Flags |= CHAN_Retrig;
                        Data->Flags &= ~CHAN_Cut;
                        ChanDat[Data->Channel-1].SampleOffset = 0;
                        Data->VolEnvelopePos = 0;
                        Data->PanEnvelopePos = 0;
                        Data->PitchEnvelopePos = 0;
                        Data->KeyState = 3;
                        }*/
                     if ( Header->Compatability < 0x200 ) Data->FadeOut = 512;
                     else Data->FadeOut = 1024;
                     if ( Header->Flags & 4 ) {
                        Data->PortaTarget = (int)((float) 1712 * (float) 16777216 / (float) Samples[Data->Sample]->BaseFrequency / pow ( 2, (float) ( Instruments[Data->Instrument]->Mapping[Data->Note][0] ) / 12.0 ));
                        } else {
                        Data->PortaTarget = (int)((float) 1712 * (float) 16777216 / (float) Samples[Data->Sample]->BaseFrequency / pow ( 2, (float) ( Data->Note ) / 12.0 ));
                        }
                     } else {
                     Data->Flags |= CHAN_Retrig | CHAN_Pitch | CHAN_Volume | CHAN_Pan;
                     Data->Flags &= ~CHAN_Cut;
                     if ( Data->Channel ) ChanDat[Data->Channel-1].SampleOffset = 0;
                     Data->VolEnvelopePos = 0;
                     Data->PanEnvelopePos = 0;
                     Data->PitchEnvelopePos = 0;
                     Data->KeyState = 3;
                     if ( Header->Compatability < 0x200 ) Data->FadeOut = 512;
                     else Data->FadeOut = 1024;
                     Data->VibratoTotal = 0;
                     Data->VibratoCounter = 0;
                     if ( Header->Flags & 4 ) {
                        Data->Period = (int)((float) 1712 * (float) 16777216 / (float) Samples[Data->Sample]->BaseFrequency / pow ( 2, (float) ( Instruments[Data->Instrument]->Mapping[Data->Note][0] ) / 12.0 ));
                        } else {
                        Data->Period = (int)((float) 1712 * (float) 16777216 / (float) Samples[Data->Sample]->BaseFrequency / pow ( 2, (float) ( Data->Note ) / 12.0 ));
                        }

                     }
                  }

               //Check for an instrument change
               if ( Instrument > 0 && NoteNow ) {
//                  if ( !(Header->Flags & 4) ) {
//                     Data->Sample = Data->Instrument;
//                  }

                  Data->Flags |= CHAN_Instrument;
//                  Data->Flags &= ~CHAN_Cut;
                  if ( Data->Sample >= 0 && Note < 254 ) {
                      Data->Volume = Samples[Data->Sample]->Volume * VolMax / 64;
                      Data->Flags |= CHAN_Volume;
                      }
                  if ( Header->Flags & 4 ) {
                     if ( Header->Compatability >= 0x200 ) {
                        Data->VolEnvelope = Instruments2[Data->Instrument]->VolumeEnvelope.Flags & 1;
                        Data->PanEnvelope = Instruments2[Data->Instrument]->PanEnvelope.Flags & 1;
                        Data->PitchEnvelope = Instruments2[Data->Instrument]->PitchEnvelope.Flags & 1;
                        Data->NewNoteAction = Instruments2[Data->Instrument]->NewNoteAction;
     		        if ( !(Instruments2[Data->Instrument]->DefaultPan & 128) ) {
                           Data->PanPos = ( Instruments2[Data->Instrument]->DefaultPan & 127 ) * PanSpan / 64 - PanMax;
                           boundv(-PanMax,Data->PanPos,PanMax);
                           Data->Flags |= CHAN_Pan;
                           }
                        if ( Instruments2[Data->Instrument]->RandomVolume ) {
                           Data->Volume += Data->Volume * ( rand()%(Instruments2[Data->Instrument]->RandomVolume*2)-Instruments2[Data->Instrument]->RandomVolume ) / 100;
                           boundv(0,Data->Volume,VolMax);
                           Data->Flags |= CHAN_Volume;
                           }
                        if ( Instruments2[Data->Instrument]->RandomPan ) {
                           Data->PanPos += Data->PanPos * ( rand()%(Instruments2[Data->Instrument]->RandomPan*2)-Instruments2[Data->Instrument]->RandomPan ) / 100;
                           Data->Flags |= CHAN_Pan;
                           boundv(-PanMax,Data->PanPos,PanMax);
                           }
                        if ( Instruments2[Data->Instrument]->PitchPanSeparation && Data->Note < 120 ) {
                           Data->PitchPanPos = ( Data->Note - Instruments2[Data->Instrument]->PitchPanCentre ) * Instruments2[Data->Instrument]->PitchPanSeparation * PanMax / 64 / 4;
                           Data->Flags |= CHAN_Pan;
                           } else Data->PitchPanPos = 0;

                        } else {
                        Data->VolEnvelope = Instruments[Data->Instrument]->Flags & 1;
                        Data->PanEnvelope = 0;
                        Data->PitchEnvelope = 0;
                        Data->NewNoteAction = Instruments[Data->Instrument]->NewNoteAction;
                        }
                     }
                  }

               if ( Volume <= 64 && NoteNow ) {
                  Data->Volume = Volume * VolMax / 64;
                  Data->Flags |= CHAN_Volume;
                  }
               if ( Volume >= 128 && Volume <= 192 && NoteNow ) {
                  Data->PanPos = ( Volume - 128 ) * PanSpan / 64 - PanMax;
                  Data->Flags |= CHAN_Pan | CHAN_Volume;
                  }

               switch ( Effect ) {
                  case EFF_None:
                     break;
                  case EFF_SetSpeed:
                     if ( Frame == 0 ) {
                        if ( !EffectData ) break;
                        FramesARow = EffectData;
                        FInf.Speed = FramesARow;
                        }
                     break;
                  case EFF_SetOrder:
                     if ( Header->NoOfOrders && Frame == 0 ) NewOrder = EffectData;
                     break;
                  case EFF_BreakRow:
                     if ( Frame == 0 ) {
                        NewOrder = CurOrder + 1;
                        NewRow = EffectData;
                        }
                     break;
                  case EFF_VolSlide:
                  case EFF_DualVibrato:
                  case EFF_DualPorta:
                        if ( ( EffectData & 15 ) == 0 ) {
                        if ( Frame != 0 ) Data->Volume += ( EffectData >> 4 ) * VolMax / 64;
                        } else if ( ( EffectData & ~15 ) == 0 ) {
                        if ( Frame != 0 ) Data->Volume -= ( EffectData & 15 ) * VolMax / 64;
                        } else if ( ( EffectData & 15 ) == 0xF ) {
                        Data->Volume += ( EffectData >> 4 ) * VolMax / 64 / FramesARow;
                        if ( FramesARow > 1 && Frame == FramesARow - 1 )
                           Data->Volume += ( EffectData >> 4 ) * VolMax / 64 - ( ( EffectData >> 4 ) * VolMax / 64 / FramesARow ) * FramesARow;
                        } else if ( ( EffectData & ~15 ) == 0xF0 ) {
                        Data->Volume -= ( EffectData & 15 ) * VolMax / 64 / FramesARow;
                        if ( FramesARow > 1 && Frame == FramesARow - 1 )
                           Data->Volume -= ( EffectData & 15 ) * VolMax / 64 - ( ( EffectData & 15 ) * VolMax / 64 / FramesARow ) * FramesARow;
                        }
                     boundv(0,Data->Volume,VolMax);
                     Data->Flags |= CHAN_Volume;

                     if ( Effect == EFF_DualVibrato ) goto VibratoIn;
                     if ( Effect == EFF_DualPorta ) {
                        EffectData = Data->LastPorta;
                        goto PortaIn;
                        }
                     break;
                  case EFF_SlideDown:
                     if ( Header->Flags & 8 ) {
                        if ( ( EffectData & 0xF0 ) == 0xE0 ) {
                           if ( Frame == 0 ) Data->Period = (int) ( (float) Data->Period * pow ( 2, ( EffectData & 0xF ) / 768.0 ) );
                           } else if ( ( EffectData & 0xF0 ) == 0xF0 ) {
                           if ( Frame == 0 ) Data->Period = (int) ( (float) Data->Period * pow ( 2, ( EffectData & 0xF ) * 4 / 768.0 ) );
                           } else {
                           if ( Frame != 0 ) Data->Period = (int) ( (float) Data->Period * pow ( 2, EffectData * 4 / 768.0 ) );
                           }
                        } else {
                        if ( ( EffectData & 0xF0 ) == 0xE0 ) {
                           if ( Frame == 0 ) Data->Period += ( EffectData & 0xF ) * 64;
                           } else if ( ( EffectData & 0xF0 ) == 0xF0 ) {
                           if ( Frame == 0 ) Data->Period += ( EffectData & 0xF ) * 4 * 64;
                           } else {
                           if ( Frame != 0 ) Data->Period += EffectData * 4 * 64;
                           }
                        }
                     if ( Data->Period > 0x7FFF * 4 * 64 ) Data->Period = 0x7FFF * 4 * 64;
                     Data->Flags |= CHAN_Pitch;
                     break;
                  case EFF_SlideUp:
                     if ( Header->Flags & 8 ) {
                        if ( ( EffectData & 0xF0 ) == 0xE0 ) {
                           if ( Frame == 0 ) Data->Period = (int) ( (float) Data->Period / pow ( 2, ( EffectData & 0xF ) / 768.0 ) );
                           } else if ( ( EffectData & 0xF0 ) == 0xF0 ) {
                           if ( Frame == 0 ) Data->Period = (int) ( (float) Data->Period / pow ( 2, ( EffectData & 0xF ) * 4 / 768.0 ) );
                           } else {
                           if ( Frame != 0 ) Data->Period = (int) ( (float) Data->Period / pow ( 2, EffectData * 4 / 768.0 ) );
                           }
                        } else {
                        if ( ( EffectData & 0xF0 ) == 0xE0 ) {
                           if ( Frame == 0 ) Data->Period -= ( EffectData & 0xF ) * 64;
                           } else if ( ( EffectData & 0xF0 ) == 0xF0 ) {
                           if ( Frame == 0 ) Data->Period -= ( EffectData & 0xF ) * 4 * 64;
                           } else {
                           if ( Frame != 0 ) Data->Period -= EffectData * 4 * 64;
                           }
                        }

                     if ( Data->Period < 57 * 64 ) Data->Period = 57 * 64;
                     Data->Flags |= CHAN_Pitch;
                     break;
                  case EFF_Porta:
                     Data->LastPorta = EffectData;
                     PortaIn:
                     if ( Frame != 0 && Data->PortaTarget ) {
                        if ( Data->Period > Data->PortaTarget ) {
                           if ( Header->Flags & 8 ) {
                              Data->Period = (int) ( (float) Data->Period / pow ( 2, (float) EffectData * 4.0 / SLIDESPEED ) );
                              } else {
                              Data->Period -= EffectData * 64 * 4;
                              }
                           if ( Data->Period < Data->PortaTarget ) Data->Period = Data->PortaTarget;
                           } else {
                           if ( Header->Flags & 8 ) {
                              Data->Period = (int) ( (float) Data->Period * pow ( 2, (float) EffectData * 4.0 / SLIDESPEED ) );
                           } else {
                              Data->Period += EffectData * 64 * 4;
                              }
                           if ( Data->Period > Data->PortaTarget ) Data->Period = Data->PortaTarget;
                           }
                        Data->Flags |= CHAN_Pitch;
                        }
                     break;
                  case EFF_Vibrato:
                     Data->VibratoSpeed = EffectData >> 4;
                     Data->VibratoSize = ( ( EffectData & 15 ) + 1 ) * 4;
                     VibratoIn:
                     if ( Note < 120 ) Data->VibratoPos = 0;
                     if ( Header->Flags & 16 ) Data->PeriodShift += (int) ( (float) Data->Period * (float) VibratoTypes[Data->VibratoType%4][Data->VibratoPos%256] * (float) Data->VibratoSize / 64.0 / 255.0 / 12.0 );
                        else Data->PeriodShift += (int) ( (float) Data->Period * (float) VibratoTypes[Data->VibratoType%4][Data->VibratoPos%256] * (float) Data->VibratoSize / 64.0 / 255.0 / 24.0 );
                     if ( Frame == 0 && ( Header->Flags & 16 ) ) break;
                     Data->VibratoPos += Data->VibratoSpeed * 4;
                     Data->Flags |= CHAN_Pitch;
                     break;
                  case EFF_Tremor:
                     if ( ( EffectData & 15 ) + ( EffectData >> 4 ) ) if ( Frame%( ( EffectData & 15 ) + ( EffectData >> 4 ) ) >= ( EffectData & 15 )  ) Data->VolumeShift = -65536*256*16;
                     break;
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
                  case EFF_SetChanVolume:
                     if ( Frame == 0 ) {
                        if ( EffectData <= 64 ) Data->ChannelVolume = EffectData * VolMax / 64;
                           else Data->ChannelVolume = VolMax;
                        }
                     break;
                  case EFF_SlideChanVolume:
                     if ( ( EffectData & 15 ) == 0 ) {
                        if ( Frame != 0 ) Data->ChannelVolume += ( EffectData >> 4 ) * VolMax / 64;
                        } else if ( ( EffectData & ~15 ) == 0 ) {
                        if ( Frame != 0 ) Data->ChannelVolume -= ( EffectData & 15 ) * VolMax / 64;
                        } else if ( ( EffectData & 15 ) == 0xF ) {
                        Data->ChannelVolume += ( EffectData >> 4 ) * VolMax / 64 / FramesARow;
                        if ( FramesARow > 1 && Frame == FramesARow - 1 )
                           Data->ChannelVolume += ( EffectData >> 4 ) * VolMax / 64 - ( ( EffectData >> 4 ) * VolMax / 64 / FramesARow ) * FramesARow;
                        } else if ( ( EffectData & ~15 ) == 0xF0 ) {
                        Data->ChannelVolume -= ( EffectData & 15 ) * VolMax / 64 / FramesARow;
                        if ( FramesARow > 1 && Frame == FramesARow - 1 )
                           Data->ChannelVolume -= ( EffectData & 15 ) * VolMax / 64 - ( ( EffectData & 15 ) * VolMax / 64 / FramesARow ) * FramesARow;
                        }
                     boundv(0,Data->ChannelVolume,VolMax);
                     Data->Flags |= CHAN_Volume;
                     break;
                  case EFF_SetSampleOffset:
                     if (Header->Flags & 16 && 
			 Data->Sample >= 0 && 
			 Data->Sample < Header->NoOfSamples && 
			 (unsigned long)(EffectData*0x100) > Samples[Data->Sample]->Length)
		       break;
                     if ( Frame == 0 && Data->Channel ) ChanDat[Data->Channel-1].SampleOffset = EffectData * 0x100;
                     break;
                  case EFF_PanSlide:
                     if ( ( EffectData & 15 ) == 0 ) {
                        if ( Frame != 0 ) Data->PanPos -= ( EffectData >> 4 ) * PanSpan / 64;
                        } else if ( ( EffectData & ~15 ) == 0 ) {
                        if ( Frame != 0 ) Data->PanPos += ( EffectData & 15 ) * PanSpan / 64;
                        } else if ( ( EffectData & 15 ) == 0xF ) {
                        Data->PanPos += ( EffectData >> 4 ) * PanSpan / 64 / FramesARow;
                        if ( FramesARow > 1 && Frame == FramesARow - 1 )
                           Data->PanPos += ( EffectData >> 4 ) * PanSpan / 64 - ( ( EffectData >> 4 ) * PanSpan / 64 / FramesARow ) * FramesARow;
                        } else if ( ( EffectData & ~15 ) == 0xF0 ) {
                        Data->PanPos -= ( EffectData & 15 ) * PanSpan / 64 / FramesARow;
                        if ( FramesARow > 1 && Frame == FramesARow - 1 )
                           Data->PanPos -= ( EffectData & 15 ) * PanSpan / 64 - ( ( EffectData & 15 ) * PanSpan / 64 / FramesARow ) * FramesARow;
                        }
                     boundv(-PanMax,Data->PanPos,PanMax);
                     Data->Flags |= CHAN_Pan | CHAN_Volume;
                     break;
                  case EFF_MultiRetrig:
                     if ( Data->RetrigCounter ) Data->RetrigCounter--;
                     else Data->RetrigCounter = EffectData & 0xF;

                     if ( Data->RetrigCounter == 0 ) {
                        Data->RetrigCounter = EffectData & 0xF;
                        Data->Volume *= RetrigMul[EffectData >> 4];
                        Data->Volume /= 16;
                        Data->Volume += RetrigAdd[EffectData >> 4] * VolMax / 64;
                        Data->Flags |= CHAN_Volume | CHAN_Retrig;
                        Data->Flags &= ~CHAN_Cut;
                        boundv(0,Data->Volume,VolMax);
                        if ( Data->Channel ) ChanDat[Data->Channel - 1].SampleOffset = 0;
                        }
                     break;
                  case EFF_Tremolo:
                     Data->TremoloSpeed = EffectData & 15;
                     Data->TremoloSize = EffectData >> 4;
                     Data->VolumeShift += VibratoTypes[Data->TremoloType%4][Data->TremoloPos%256] * ( Data->TremoloSize + 1 ) / 255;
                     if ( Frame == 0 && ( Header->Flags & 16 ) ) break;
                     Data->TremoloPos += Data->TremoloSpeed;
                     Data->Flags |= CHAN_Volume;
                     break;
                  case EFF_Special:
                     switch ( EffectData >> 4 ) {
                        case EFF_SPECIAL_SetVibratoWave:
                           Data->VibratoType = EffectData & 7;
                           break;
                        case EFF_SPECIAL_SetTremoloWave:
                           Data->TremoloType = EffectData & 7;
                           break;
                        case EFF_SPECIAL_SetPanbrelloWave:
                           Data->PanbrelloType = EffectData & 7;
                           break;
                        case EFF_SPECIAL_PastNote:
                           switch ( EffectData & 15 ) {
                              case EFF_SPECIAL_PASTNOTE_NNACut:
                                 Data->NewNoteAction = NNA_Cut;
                                 break;
                              case EFF_SPECIAL_PASTNOTE_NNACont:
                                 Data->NewNoteAction = NNA_Continue;
                                 break;
                              case EFF_SPECIAL_PASTNOTE_NNAOff:
                                 Data->NewNoteAction = NNA_Noteoff;
                                 break;
                              case EFF_SPECIAL_PASTNOTE_NNAFade:
                                 Data->NewNoteAction = NNA_Notefade;
                                 break;
                              case EFF_SPECIAL_PASTNOTE_NoteCut:
                                 for ( x = 0; x < PATTERNCHANS; x++ ) {
                                    if ( Data->Channels[x] && MainChannelData[Data->Channels[x] - 1]->Parent == Channel ) {
                                       if ( MainChannelData[Data->Channels[x] - 1] && MainChannelData[Data->Channels[x] - 1] != Data ) {
                                          MainChannelData[Data->Channels[x] - 1]->Flags |= CHAN_Cut;
                                          MainChannelData[Data->Channels[x] - 1]->Flags &= ~CHAN_Retrig;
                                          }
                                       Data->Channels[x] = 0;
                                       }
                                    }
                                 break;
                              case EFF_SPECIAL_PASTNOTE_NoteOff:
                                 for ( x = 0; x < PATTERNCHANS; x++ ) {
                                    if ( Data->Channels[x] && MainChannelData[Data->Channels[x] - 1]->Parent == Channel ) {
                                       if ( MainChannelData[Data->Channels[x] - 1] && MainChannelData[Data->Channels[x] - 1] != Data ) {
                                          MainChannelData[Data->Channels[x] - 1]->KeyState &= ~1;
                                          }
                                       Data->Channels[x] = 0;
                                       }
                                    }
                                 break;
                              case EFF_SPECIAL_PASTNOTE_NoteFade:
                                 for ( x = 0; x < PATTERNCHANS; x++ ) {
                                    if ( Data->Channels[x] && MainChannelData[Data->Channels[x] - 1]->Parent == Channel ) {
                                       if ( MainChannelData[Data->Channels[x] - 1] && MainChannelData[Data->Channels[x] - 1] != Data ) {
                                          MainChannelData[Data->Channels[x] - 1]->KeyState &= ~2;
                                          }
                                       Data->Channels[x] = 0;
                                       }
                                    }
                                 break;
                              case EFF_SPECIAL_PASTNOTE_VolEnvOff:
                                 Data->VolEnvelope = 0;
                                 break;
                              case EFF_SPECIAL_PASTNOTE_VolEnvOn:
                                 Data->VolEnvelope = 1;
                                 break;
                              case EFF_SPECIAL_PASTNOTE_PanEnvOff:
                                 Data->PanEnvelope = 0;
                                 break;
                              case EFF_SPECIAL_PASTNOTE_PanEnvOn:
                                 Data->PanEnvelope = 1;
                                 break;
                              case EFF_SPECIAL_PASTNOTE_PitchEnvOff:
                                 Data->PitchEnvelope = 0;
                                 break;
                              case EFF_SPECIAL_PASTNOTE_PitchEnvOn:
                                 Data->PitchEnvelope = 1;
                                 break;
                              }
                           break;
                        case EFF_SPECIAL_SetPanPos:
                           Data->PanPos = ( EffectData & 0xF ) * PanSpan / 15 - PanMax;
                           Data->Flags |= CHAN_Pan | CHAN_Volume;
                           break;
                        case EFF_SPECIAL_SetSurround:
                           if ( EffectData ) Data->Flags |= CHAN_Surround;
                              else Data->Flags &= ~CHAN_Surround;
                           break;
                        case EFF_SPECIAL_PatternLoop:
                           if ( Frame == 0 ) {
                             if ( ( EffectData & 15 ) == 0 ) {
                                  LoopRow = Row;
                                  break;
                                  }
                               if ( CurrentLoop == 0 ) {
                                  CurrentLoop = EffectData & 15;
                                  NewRow = LoopRow;
                                  NewOrder = CurOrder;
                                  break;
                                  }
                               if ( --CurrentLoop ) {
                                  NewRow = LoopRow;
                                  NewOrder = CurOrder;
                                  }
                              }
                           break;
                        case EFF_SPECIAL_NoteCut:
                           if ( Frame == EffectData & 15 ) {
                              Data->Flags |= CHAN_Cut;
                              Data->Flags &= ~CHAN_Retrig;
                              }
                           break;
                        case EFF_SPECIAL_PatternDelay:
                           if ( Frame == 0 ) if ( PatternDelay == 0 ) PatternDelay = ( EffectData & 15 ) + 1;
                           break;
                        }
                     break;
                  case EFF_SetTempo:
                     if ( EffectData > 32 )
                     {
                        FrameLength = Device->SecondsToLen ( 125 / ( 50.0 * ( ( float ) EffectData ) ) );
                        FInf.Tempo = EffectData;
                     }
                     break;
                  case EFF_FineVibrato:
                     if ( Frame != 0 || !( Header->Flags & 16 ) ) {
                        Data->VibratoSpeed = EffectData >> 4;
                        Data->VibratoSize = ( EffectData & 15 ) + 1;
                        if ( Note < 120 ) Data->VibratoPos = 0;
                        if ( Header->Flags & 16 ) Data->PeriodShift += (int) ( (float) Data->Period * (float) VibratoTypes[Data->VibratoType%4][Data->VibratoPos%256] * (float) Data->VibratoSize / 64.0 / 255.0 / 12.0 );
                        else Data->PeriodShift += (int) ( (float) Data->Period * (float) VibratoTypes[Data->VibratoType%4][Data->VibratoPos%256] * (float) Data->VibratoSize / 64.0 / 255.0 / 24.0 );
                        Data->VibratoPos += Data->VibratoSpeed;
                        Data->Flags |= CHAN_Pitch;
                        }
                     break;
                  case EFF_SetGlobalVol:
                     if ( Frame == 0 ) {
                        GlobalVolume = EffectData * VolMax / 128;
                        if ( GlobalVolume > VolMax ) GlobalVolume = VolMax;
                        GlobalFlag |= 1;
                        }
                     FInf.GlobalVolume = GlobalVolume;
                     break;
                  case EFF_SlideGlobalVol:
                     if ( ( EffectData & 15 ) == 0 ) {
                        if ( Frame != 0 ) GlobalVolume += ( EffectData >> 4 ) * VolMax / 128;
                        } else if ( ( EffectData & ~15 ) == 0 ) {
                        if ( Frame != 0 ) GlobalVolume -= ( EffectData & 15 ) * VolMax / 128;
                        } else if ( ( EffectData & 15 ) == 0xF ) {
                        GlobalVolume += ( EffectData >> 4 ) * VolMax / 128 / FramesARow;
                        if ( FramesARow > 1 && Frame == FramesARow - 1 )
                           GlobalVolume += ( EffectData >> 4 ) * VolMax / 128 - ( ( EffectData >> 4 ) * VolMax / 128 / FramesARow ) * FramesARow;
                        } else if ( ( EffectData & ~15 ) == 0xF0 ) {
                        GlobalVolume -= ( EffectData & 15 ) * VolMax / 128 / FramesARow;
                        if ( FramesARow > 1 && Frame == FramesARow - 1 )
                           GlobalVolume -= ( EffectData & 15 ) * VolMax / 128 - ( ( EffectData & 15 ) * VolMax / 128 / FramesARow ) * FramesARow;
                        }
                     boundv(0,GlobalVolume,VolMax);
                     FInf.GlobalVolume = GlobalVolume;
                     GlobalFlag |= 1;
                     break;
                  case EFF_SetPanPos:
                     if ( Frame == 0 ) {
                        Data->PanPos = EffectData * PanSpan / 255 - PanMax;
                        Data->Flags |= CHAN_Pan | CHAN_Volume;
                        }
                     break;
                  case EFF_PanBrello:
                     Data->PanbrelloSpeed = ( EffectData >> 4 );
                     Data->PanbrelloSize = EffectData & 15;
                     if ( ( Data->PanbrelloType & 3 ) < 3 ) {
                        Data->PanbrelloShift = VibratoTypes[Data->PanbrelloType%4][Data->PanbrelloPos%256] * ( Data->PanbrelloSize + 1 ) * PanSpan / 128 / 64;
                        Data->PanbrelloPos += Data->PanbrelloSpeed;
                        } else {
                        if ( Data->PanbrelloSpeed <= 1 ) break;
                        Data->PanbrelloShift = VibratoTypes[Data->PanbrelloType%4][( Data->PanbrelloPos / ( Data->PanbrelloSpeed - 1 ) + abs(VRandom[Channel]) )%256] * ( Data->PanbrelloSize + 1 ) * PanSpan / 128 / 24;
                        Data->PanbrelloPos++;
                        }
                     Data->Flags |= CHAN_Pan | CHAN_Volume;
                     break;
                  }
               }

            for ( Channel = 0; Channel < MaxNoOfChannels; Channel++ ) {
               CmdInfoIT *Data = MainChannelData[Channel];

               if ( !Data ) {
                  ChanDat[Channel].Flags |= CHAN_Free;
                  continue;
                  }
               if ( Data->Muted ) continue;
               ChanDat[Channel].ModuleChannel = Data->Parent;

                  //Instrument fadeout
                  if ( ( !( Data->KeyState & 2 ) || ( !( Data->KeyState & 1 ) && ( !Data->VolEnvelope || ( (Instruments[Data->Instrument]->Flags&2) && Header->Compatability < 0x200 ) || ( (Instruments2[Data->Instrument]->VolumeEnvelope.Flags&2) && Header->Compatability >= 0x200  ) ) ) ) ) {
                     unsigned short FadeOut;
                     if ( Header->Compatability < 0x200 ) {
                        FadeOut = Instruments[Data->Instrument]->FadeOut;
                        } else {
                        FadeOut = Instruments2[Data->Instrument]->FadeOut;
                        }

                     if ( FadeOut ) {
                        Data->FadeOut -= FadeOut;
                        if ( Data->FadeOut < 0 ) {
                           Data->FadeOut = 0;
                           Data->Flags |= CHAN_Cut;
                           Data->Flags &= ~CHAN_Retrig;
                           }
                        if ( Header->Compatability < 0x200 ) {
                           Data->VolumeShift += Data->Volume * Data->FadeOut / 512 - Data->Volume;
                           } else {
                           Data->VolumeShift += Data->Volume * Data->FadeOut / 1024 - Data->Volume;
                           }
                        Data->Flags |= CHAN_Volume;
                        }
                     }

                  //Apply envelopes and other instrument properties
                  if ( Header->Flags & 4 )
                  if ( Header->Compatability < 0x200 ) {
                     if ( Data->Instrument < Header->NoOfInstruments && Data->VolEnvelopePos >= 0 ) {
                        //Volume envelope
                        if ( Data->VolEnvelope && Data->Volume ) {
                           Envelope *Env = Instruments[Data->Instrument]->VolumeEnvelope;

                           x = Data->VolEnvelopePos;
                           for ( y = 1; y < 25; y++ ) {
                              z = Env[y].Value;
                              if ( Env[y].Position > x || z == 0xFF ) break;
                              }
                           y--;
                           if ( z == 0xFF ) {
                              v = Env[y].Value;
//                              Data->KeyState &= ~2;
                              } else {
                              v = ( Env[y].Value * ( Env[y+1].Position - x ) + Env[y+1].Value * ( x - Env[y].Position ) ) / ( Env[y+1].Position - Env[y].Position );
                              }

                           Data->VolumeShift += Data->Volume * v / 64 - Data->Volume;
                           Data->Flags |= CHAN_Volume;
                           Data->VolEnvelopePos++;

                           if ( ( Instruments[Data->Instrument]->Flags & 4 ) && ( Data->KeyState & 1 ) ) {
                              if ( Data->VolEnvelopePos == Env[Instruments[Data->Instrument]->SustainLoopEnd].Position + 1 ) Data->VolEnvelopePos = Env[Instruments[Data->Instrument]->SustainLoopStart].Position;
                              z = 0;
                              }
                           if ( Instruments[Data->Instrument]->Flags & 2 ) {
                              if ( Data->VolEnvelopePos == Env[Instruments[Data->Instrument]->VolumeLoopEnd].Position + 1 ) Data->VolEnvelopePos = Env[Instruments[Data->Instrument]->VolumeLoopStart].Position;
                              z = 0;
                              }
                           
                           if ( z == 0xFF ) Data->KeyState &= ~2;
                           
                           }
                        }
                     } else if ( Data->Instrument >= 0 && Data->Instrument < Header->NoOfInstruments ) {
                     if ( Data->VolEnvelopePos >= 0 && Data->Volume ) {
                        //Volume envelope
                        Envelope2 *Env = &Instruments2[Data->Instrument]->VolumeEnvelope;

                        if ( Env->Flags & 1 ) {
                           x = Data->VolEnvelopePos;
                           for ( y = 1; y < Env->Points; y++ ) {
                              if ( Env->Nodes[y].Position > x ) break;
                              }
                           y--;
                           if ( y + 1 >= Env->Points ) {
                              v = Env->Nodes[y].Value;
                              } else {
                              v = ( Env->Nodes[y].Value * ( Env->Nodes[y+1].Position - x ) + Env->Nodes[y+1].Value * ( x - Env->Nodes[y].Position ) ) / ( Env->Nodes[y+1].Position - Env->Nodes[y].Position );
                           }

                           Data->VolumeShift += Data->Volume * v / 64 - Data->Volume;
                           Data->Flags |= CHAN_Volume;
                           Data->VolEnvelopePos++;

                           if ( ( Env->Flags & 4 ) && ( Data->KeyState & 1 ) ) {
                              if ( Data->VolEnvelopePos == Env->Nodes[Env->SustainLoopEnd].Position + 1 ) Data->VolEnvelopePos = Env->Nodes[Env->SustainLoopBegin].Position;
                              y = -2;
                              }
                           if ( Env->Flags & 2 ) {
                              if ( Data->VolEnvelopePos == Env->Nodes[Env->LoopEnd].Position + 1 ) Data->VolEnvelopePos = Env->Nodes[Env->LoopBegin].Position;
                              y = -2;
                              }

                           if ( y + 1 >= Env->Points ) {
                              Data->KeyState &= ~2;
                              if ( v == 0 ) {
                                 Data->Flags |= CHAN_Cut;
                                 Data->Flags &= ~CHAN_Retrig;
                                 }
                              }
                           
                           }
                        }

                     if ( Data->PanEnvelopePos >= 0 && ( Data->Volume + Data->VolumeShift > 0 ) ) {
                        //Panning envelope
                        Envelope2 *Env = &Instruments2[Data->Instrument]->PanEnvelope;

                        if ( Env->Flags & 1 ) {
                           x = Data->PanEnvelopePos;
                           for ( y = 1; y < Env->Points; y++ ) {
                              if ( Env->Nodes[y].Position > x ) break;
                              }
                           y--;
                           if ( y + 1 >= Env->Points ) {
                              v = Env->Nodes[y].Value;
                              } else {
                              v = ( Env->Nodes[y].Value * ( Env->Nodes[y+1].Position - x ) + Env->Nodes[y+1].Value * ( x - Env->Nodes[y].Position ) ) / ( Env->Nodes[y+1].Position - Env->Nodes[y].Position );
                              }
                           v = v * PanMax / 32;
                           Data->PanShift += ( Data->PanPos + v ) / 2 - Data->PanPos;
                           Data->Flags |= CHAN_Pan;
                           Data->PanEnvelopePos++;

                           if ( ( Env->Flags & 4 ) && ( Data->KeyState & 1 ) ) {
                              if ( Data->PanEnvelopePos == Env->Nodes[Env->SustainLoopEnd].Position + 1 ) Data->PanEnvelopePos = Env->Nodes[Env->SustainLoopBegin].Position;
                              }
                           if ( Env->Flags & 2 ) {
                              if ( Data->PanEnvelopePos == Env->Nodes[Env->LoopEnd].Position + 1 ) Data->PanEnvelopePos = Env->Nodes[Env->LoopBegin].Position;
                              }
                           }
                        }

                     if ( Data->PitchEnvelopePos >= 0 && ( Data->Volume + Data->VolumeShift > 0 ) ) {
                        //Pitch envelope
                        Envelope2 *Env = &Instruments2[Data->Instrument]->PitchEnvelope;

                        if ( Env->Flags & 1 ) {
                           x = Data->PitchEnvelopePos;
                           for ( y = 1; y < Env->Points; y++ ) {
                              if ( Env->Nodes[y].Position > x ) break;
                              }
                           y--;
                           if ( y + 1 >= Env->Points ) {
                              v = Env->Nodes[y].Value;
                              } else {
                              v = ( Env->Nodes[y].Value * ( Env->Nodes[y+1].Position - x ) + Env->Nodes[y+1].Value * ( x - Env->Nodes[y].Position ) ) / ( Env->Nodes[y+1].Position - Env->Nodes[y].Position );
                           }
                           Data->PeriodShift += (int)(Data->Period / pow ( 2, v / 24.0 ) - Data->Period);
                           Data->Flags |= CHAN_Pitch;
                           Data->PitchEnvelopePos++;

                           if ( ( Env->Flags & 4 ) && ( Data->KeyState & 1 ) ) {
                              if ( Data->PitchEnvelopePos == Env->Nodes[Env->SustainLoopEnd].Position + 1 ) 
				 Data->PitchEnvelopePos = (int)(Env->Nodes[Env->SustainLoopBegin].Position);
                              }
                           if ( Env->Flags & 2 ) {
                              if ( Data->PitchEnvelopePos == Env->Nodes[Env->LoopEnd].Position + 1 ) Data->PitchEnvelopePos = Env->Nodes[Env->LoopBegin].Position;
                              }
                           }
                        }
		     }

               if ( Data->Flags & CHAN_Cut ) {
                  Data->Volume = 0;
                  Data->Flags |= CHAN_Volume;
                  ChanDat[Channel].Flags |= CHAN_Free;
                  ChanDat[Channel].MainVol = 0;
                  ChanDat[Channel].LeftVol = 0;
                  ChanDat[Channel].RightVol = 0;
                  }

               //Instrument vibrato
               if ( Data->Sample >= 0 && Data->Sample < Header->NoOfSamples && Samples[Data->Sample]->VibratoDepth ) {
                  x = Data->Sample;
                  Data->VibratoTotal += Samples[x]->VibratoRate;
                  if ( Data->VibratoTotal >= Samples[x]->VibratoDepth * 256 ) Data->VibratoTotal = Samples[x]->VibratoDepth * 256;
                  Data->PeriodShift += (int) ( (float) Data->Period * (float) VibratoTypes[Samples[x]->VibratoType%4][Data->VibratoCounter%256] * (float) Data->VibratoTotal / 16777216.0 / 4.0 );
                  Data->VibratoCounter += Samples[x]->VibratoSpeed;
                  Data->Flags |= CHAN_Pitch;
                  }

               if ( !( Data->KeyState & 5 ) ) {
                  if ( ( Samples[Data->Sample]->Flags & 160 ) ) {
                     if ( ( Samples[Data->Sample]->Flags & 80 ) == 16 ) {
                        ChanDat[Channel].NewLoopBegin = Samples[Data->Sample]->LoopBegin;
                        ChanDat[Channel].NewLoopEnd = Samples[Data->Sample]->LoopEnd;
                        ChanDat[Channel].NewLoopFlags = INST_Loop;
                        } else if ( ( Samples[Data->Sample]->Flags & 80 ) == 80 ) {
                        ChanDat[Channel].NewLoopBegin = Samples[Data->Sample]->LoopBegin;
                        ChanDat[Channel].NewLoopEnd = Samples[Data->Sample]->LoopEnd;
                        ChanDat[Channel].NewLoopFlags = INST_PingPong;
                        } else {
                        ChanDat[Channel].NewLoopBegin = 0;
                        ChanDat[Channel].NewLoopEnd = Samples[Data->Sample]->Length;
                        ChanDat[Channel].NewLoopFlags = 0;
                        }
                     Data->Flags |= CHAN_NewLoop;
                     }
                  Data->KeyState &= ~5;
                  Data->KeyState |= 4;
                  }

               if (SkipOrds != 0 || SkipRows != 0) continue;
               
               //Process channel variables and apply changes
               if ( Data->Flags & CHAN_Instrument ) {
                  ChanDat[Channel].Sample = Data->Sample;
/*                  if ( ( Samples[Data->Sample]->Flags & 160 ) ) {
                     if ( ( Samples[Data->Sample]->Flags & 160 ) == 32 ) {
                        ChanDat[Channel].NewLoopBegin = Samples[Data->Sample]->SustainLoopBegin;
                        ChanDat[Channel].NewLoopEnd = Samples[Data->Sample]->SustainLoopEnd;
                        ChanDat[Channel].NewLoopFlags = INST_Loop;
                        Data->Flags |= CHAN_NewLoop;
                        } else if ( ( Samples[Data->Sample]->Flags & 160 ) == 160 ) {
                        ChanDat[Channel].NewLoopBegin = Samples[Data->Sample]->SustainLoopBegin;
                        ChanDat[Channel].NewLoopEnd = Samples[Data->Sample]->SustainLoopEnd;
                        ChanDat[Channel].NewLoopFlags = INST_PingPong;
                        Data->Flags |= CHAN_NewLoop;
                        }
                     }*/
                  }

               if ( Data->Flags & CHAN_Pitch ) {
                  int NotePeriod;
                  NotePeriod = ( Data->Period + Data->PeriodShift );
                  if ( NotePeriod > 0x7FFF * 64 * 4 ) NotePeriod = 0x7FFF * 64 * 4;
                  if ( NotePeriod < 57 ) NotePeriod = 57;
                  ChanDat[Channel].Pitch = (8192*1712*64)/(NotePeriod);
                  }
               if ( ( Data->Flags & CHAN_Volume ) || ( GlobalFlag & 1 ) ) {
                  long CurrentPan;

                  if ( GlobalFlag ) Data->Flags |= CHAN_Volume;
                  if ( Header->Flags & 1 ) {
                     int Pan = Data->PanPos + Data->PitchPanPos + Data->PanShift + Data->PanbrelloShift;
                     CurrentPan = (Pan*Header->InitialPanning)/128;
                     boundv(-PanMax,CurrentPan,PanMax);
                     } else {
                     CurrentPan = 0;
                     }

                  long CurrentVolume = (long)((float) (Data->Volume + Data->VolumeShift) * (float) GlobalVolume / (float) VolMax);
                  CurrentVolume = (long)((float) CurrentVolume * (float) ChannelData[Data->Parent].ChannelVolume / (float) VolMax);
                  if ( Data->Sample >= 0 && Data->Sample < Header->NoOfSamples )
                      CurrentVolume = CurrentVolume * Samples[Data->Sample]->GlobalVolume / 64;
                  if ( Header->Compatability >= 0x200 && Data->Instrument >= 0 && Data->Instrument < Header->NoOfInstruments ) {
                      CurrentVolume = (int)(CurrentVolume * Instruments2[Data->Instrument]->GlobalVolume / 128);
                  }

                  boundv(0,CurrentVolume,VolMax);
                  ChanDat[Channel].MainVol = CurrentVolume;

                  if ( ( Data->Flags & CHAN_Surround ) == 0 )
                  {
                     ChanDat[Channel].LeftVol  = ChanDat[Channel].MainVol * ( PanMax - CurrentPan ) / PanSpan;
                     ChanDat[Channel].RightVol = ChanDat[Channel].MainVol * ( PanMax + CurrentPan ) / PanSpan;
                     ChanDat[Channel].Pan = CurrentPan;
                  } else {
                     ChanDat[Channel].LeftVol = ChanDat[Channel].MainVol/2;
                     ChanDat[Channel].RightVol = -1*ChanDat[Channel].MainVol/2;
                     ChanDat[Channel].Pan = PanSurround;
                  }


               }

               if (SkipOrds == 0 && SkipRows == 0) {
                  ChanDat[Channel].Flags = ( ChanDat[Channel].Flags & ( CHAN_Free | CHAN_Surround ) ) | Data->Flags;
                  Data->Flags &= CHAN_Surround;
                  }

               if ( Frame == 1 ) GlobalFlag &= ~1;
               }

            // Let an output filter know of the current location.
            Device->SetFrameInfo(&FInf);

            FInf.SongTime += FrameLength;
            if (SkipOrds == 0 && SkipRows == 0) {
               long Rc = 0;
               if ((Rc = Device->Compute ( &ChanDat, FrameLength )) != 0 ) {
                   ChanDat.free();
                   delete [] ChannelData;
                   return Rc;
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

for ( x = 0; x < MaxNoOfChannels; x++ ) {
   if ( MainChannelData[x] && MainChannelData[x]->Channel == 0 ) free ( MainChannelData[x] );
   }

delete [] ChannelData;
return PLAYOK;
}
