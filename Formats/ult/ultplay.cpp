// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   ULTFormat - Muse file format to handle ULT files.
   
   ##################################################################### */
									/*}}}*/
// Includes									/*{{{*/
#include <muse.h>
#include <ultform.h>
#include <math.h>

#include "ultstruc.h"
   									/*}}}*/

#define GUSCONST 1.025

// ULTFormat::Play - Plays the song				
// ---------------------------------------------------------------------
/* */
long museULTFormat::Play(museOutputBase *Device,musePlayerControl *Control)
{
   if ( Header == 0 || ( PatHeader->NoOfPatterns == 0 ) || ( HeaderData->NoOfSamples == 0) || ( Device == 0 ) )
      return PLAYFAIL_NotLoaded;

   const unsigned char EffectTable[16] = { 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1 };
   
   signed short VSineTable[256];
   signed short VSquare[256];
   signed short VRamp[256];
   signed short VRandom[256];
   signed short *VibratoTypes[4] = { VSineTable, VSquare, VRamp, VSineTable };

   CmdInfoULT *ChannelData = new CmdInfoULT[MAXNOCHANS];
   
   LArgType *LastArgs = new LArgType[2];
   CPatType *CurrentPattern = new CPatType[64];

   int LastPattern, NewRow;
   unsigned short CurOrder, SongLength;
   char PatternDelay = 0;
   int x, y;

   LastPattern = -1;
   NewRow = -1;
   CurOrder = 0;
   SongLength = 0;
   PatternDelay = 0;
   
   memset ( ChannelData, 0, MAXNOCHANS*sizeof ( CmdInfoULT ) );
   memset ( LastArgs, 0, MAXNOCHANS*36*2 );

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
   BoolSeq.reserve(MAXNOCHANS);

   for ( x = 0; x < MAXNOCHANS; x++ )
   {
      if (x < PatHeader->NoOfChannels + 1)
         BoolSeq[x] = true;
      else
         BoolSeq[x] = false;
   }

   Device->SetChannelMask(&BoolSeq);
   Device->SetVolumeScale (0);
   BoolSeq.free();

   // Load the samples into the mixer.
   SequenceSample MixSamples;
   GetSongSamples(MixSamples);
   Device->LoadSamples(&MixSamples);
   MixSamples.free();

   SequenceChannel ChanDat;
   ChanDat.construct();
   ChanDat.reserve(PatHeader->NoOfChannels + 1);
   for (x = 0; x <= PatHeader->NoOfChannels; x++)
   {
      ChanDat[x].Sample = 0xFFFF;
      ChanDat[x].ModuleChannel = x;
   }

   while (PatHeader->Orders[SongLength] != 0xFF && SongLength < 0x100)
      SongLength++;

   if ( UltType >= 3 )
      for ( x = 0; x < PatHeader->NoOfChannels + 1; x++ )
      {
         ChannelData[x].PanPos = (&PatHeader->PanPosTable)[x];
         ChannelData[x].Flags |= CHAN_Pan;
      }
   else
      for ( x = 0; x < PatHeader->NoOfChannels + 1; x++ )
      {
         if ( x%2 )
            ChannelData[x].PanPos = 0xC;
         else
            ChannelData[x].PanPos = 0x3;
         ChannelData[x].Flags |= CHAN_Pan;
      }

   // Construct array to hold row bitmap
   PlayedRec *OrdPlayList = new PlayedRec[SongLength + 1];
   memset(OrdPlayList,0,(SongLength + 1)*sizeof(PlayedRec));
   museFrameInfo FInf;
   FInf.GlobalVolume = VolMax;
   FInf.Tempo = 128;
   FInf.Speed = 6;
   FInf.SongTime = 0;
   int SkipRows = 0;
   int SkipOrds = 0;
   
   while ( CurOrder < SongLength )
   {
      int Row = 0;
      if ( NewRow != -1 )
      {
         Row = NewRow;
         NewRow = -1;
      }
      int NewOrder = -1;

      if ( PatHeader->Orders[CurOrder] != LastPattern )
      {
         unsigned char *DataPointer;
         unsigned char *PatternPointer;

         for ( y = 0; y < PatHeader->NoOfChannels + 1; y++ )
         {
            int Repeat = 0;
            DataPointer = Patterns[y * ( PatHeader->NoOfPatterns + 1 ) + PatHeader->Orders[CurOrder]];
            PatternPointer = CurrentPattern[0].C[y];
            for ( x = 0; x < 64; x++ )
            {
               if ( *DataPointer == 0xFC )
               {
                  ( (unsigned long *) PatternPointer)[0] = ( (unsigned long *) (DataPointer+2) )[0];
                  PatternPointer[4] = DataPointer[6];
                  Repeat++;
                  if ( Repeat == DataPointer[1] )
                  {
                     DataPointer += 7;
                     if ( *DataPointer == 0xFC ) Repeat = 0;
                  }
               }
               else
               {
                  ( (unsigned long *) PatternPointer)[0] = ( (unsigned long *) DataPointer )[0];
                  PatternPointer[4] = DataPointer[4];
                  DataPointer += 5;

                  if ( *DataPointer == 0xFC )
                     Repeat = 0;
               }
               PatternPointer += MAXNOCHANS * 5;
            }
         }
         LastPattern = PatHeader->Orders[CurOrder];
      }

      FInf.Order = CurOrder;
      FInf.Pattern = PatHeader->Orders[CurOrder];

      if (SkipOrds != 0)
         SkipOrds--;

      while ( Row < 64 )
      {
          // Check that this row has never been played before
          if (AllowLoop == false && (CurOrder) < SongLength)
          {
              if ((OrdPlayList[CurOrder].Rows[Row/32] &
                   (1 << (Row % 32))) != 0)
              {
                  CurOrder = SongLength;
                  Trace(TRACE_INFO,"Aborting");
                  break;
              }
          }
          
          // Mark the row as played.
          OrdPlayList[CurOrder].Rows[Row/32] |= (1 << (Row % 32));
          FInf.Row = Row;

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
               NewOrder = max(NOrd + NRow/64,0);
               NewRow = NRow % 64;
               if (NRow < 0)
                  NewRow = 64 - NewRow;

               // Cut all the channels notes
               for ( x = 0; x < PatHeader->NoOfChannels + 1; x++ )
               {
                  ChannelData[x].Flags &= ~CHAN_Retrig;
                  ChannelData[x].Flags |= CHAN_Cut;
               }

               // Jump to start, reset all vars
               if ((CurOrder == 0) && (NewRow == 0))
               {
                  NewRow = -1;
                  CurOrder = 0;
                  PatternDelay = 0;

                  FInf.Tempo = 128;
                  FInf.Speed = 6;
                  FInf.SongTime = 0;
                  FramesARow = 6;
                  FrameLength = Device->SecondsToLen(60.0/(24.0*((float)128)));

                  for (x = 0; x <= PatHeader->NoOfChannels; x++)
                     ChanDat[x].Sample = 0xFFFF;

                  memset ( ChannelData, 0, MAXNOCHANS*sizeof ( CmdInfoULT ) );
                  memset ( LastArgs, 0, MAXNOCHANS*36*2 );
               }
               break;
            }
         }

//         int LoopRow = 0;
//         int CurrentLoop = 0;
         unsigned char *DataPointer = CurrentPattern[Row].C[0];

         int Channel;

         //For each channel, calculate the parameters
         for ( Channel = 0; Channel < PatHeader->NoOfChannels + 1; Channel++ )
         {
            ChannelData[Channel].NoteParm = DataPointer[0];
            ChannelData[Channel].InstrumentParm = DataPointer[1];
            ChannelData[Channel].Effect1 = DataPointer[2] & 15;
            ChannelData[Channel].Effect2 = DataPointer[2] >> 4;
            ChannelData[Channel].EffectParm1 = DataPointer[3];
            ChannelData[Channel].EffectParm2 = DataPointer[4];
            DataPointer += 5;
         }

         //Loop through each frame
         for ( int Frame = 0; Frame < FramesARow; Frame++ )
         {
            FInf.Frame = Frame;
             
            for ( Channel = 0; Channel < PatHeader->NoOfChannels + 1; Channel++ )
            {
               CmdInfoULT *Data = &ChannelData[Channel];
               int Note = Data->NoteParm;
               int Instrument = Data->InstrumentParm;
               int Effect[2], EffectParm[2];
               Effect[0] = Data->Effect1;
               Effect[1] = Data->Effect2;
               EffectParm[0] = Data->EffectParm1;
               EffectParm[1] = Data->EffectParm2;

               if (EffectParm[0] == 0 && EffectTable[Effect[0]])
                  EffectParm[0] = LastArgs[0].L[Channel][Effect[0]];
               else
                  LastArgs[0].L[Channel][Effect[0]] = EffectParm[0];
               if (EffectParm[1] == 0 && EffectTable[Effect[1]])
                  EffectParm[1] = LastArgs[1].L[Channel][Effect[1]];
               else
                  LastArgs[1].L[Channel][Effect[1]] = EffectParm[1];

               //Check for a note change
               if ( Note != 0 && Frame == 0 )
               {
                  Data->Note = Note;
                  if ( Instrument == 0 && Effect[0] == EFF_Porta )
                  {
                     if ( Data->Sample < HeaderData->NoOfSamples )
                     {
                        if ( UltType < 4 )
                           Data->PortaTarget = (int)(8363*1712 / ( 8363 * pow ( 2, (float) ( Note - 25 ) / 12.0 + ( (OldULTSample *) Samples )[Data->Sample].FineTune / 12.0 / 32768.0 ) ));
                        else
                           Data->PortaTarget = (int)(8363*1712 / ( *( (int *) &Samples[Data->Sample].FineTune ) * pow ( 2, (float) ( Note - 25 ) / 12.0 ) ));
                     }
                  }
                  else
                  {
                     Data->Flags &= ~CHAN_Cut;
                     Data->Flags |= CHAN_Volume | CHAN_Pitch | CHAN_Retrig;
                     ChanDat[Channel].SampleOffset = 0;
                     Data->Volume = Data->StartVolume;
                  }
               }

               //Check for an instrument change
               if ( Instrument != 0 && Frame == 0 )
               {
                  if ( Instrument > HeaderData->NoOfSamples )
                  {
                     Instrument = 1;
                     Data->Flags &= ~CHAN_Retrig;
                     Data->Flags |= CHAN_Cut;
                  }
                  else
                  {
                     Data->Sample = Instrument - 1;
                     Data->Flags |= CHAN_Instrument;
                     if ( Data->Note != 0 && Frame == 0 )
                     {
                        if ( Effect[0] != EFF_Porta )
                        {
                           Data->Flags |= CHAN_Pitch;
                           if ( UltType < 4 )
                              Data->Period = (int)(8363 * 1712 / ( 8363 * pow ( 2, (float) ( Note - 25 ) / 12.0 + ( (OldULTSample *) Samples )[Data->Sample].FineTune / 12.0 / 32768.0 ) ));
                           else
                              Data->Period = (int)(8363 * 1712 / ( Samples[Data->Sample].FineTune * pow ( 2, (float) ( Note - 25 ) / 12.0 + Samples[Data->Sample].BaseFrequency / 12.0 / 32768.0 ) ));

                           if (Data->VibratoType / 4 == 0)
                              Data->VibratoPos = 0;

                           if ( Data->TremoloType / 4 == 0 )
                              Data->TremoloPos = 0;

                           if ( UltType < 4 )
                              Data->StartVolume = Data->Volume = ( (OldULTSample *) Samples )[Data->Sample].Volume;
                           else
                              Data->StartVolume = Data->Volume = Samples[Data->Sample].Volume;

                           Data->Flags |= CHAN_Volume | CHAN_Pan | CHAN_Pitch;
                        }
                     }
                  }
               }

               if (Data->PeriodShift != 0)
                  Data->Flags |= CHAN_Pitch;
               Data->PeriodShift = 0;

               if (Data->VolumeShift != 0)
                  Data->Flags |= CHAN_Volume;
               Data->VolumeShift = 0;

               for ( int CurEffect = 0; CurEffect < 2; CurEffect++ )
               {
                  int EffectData = EffectParm[CurEffect];

                  switch ( Effect[CurEffect] )
                  {
                     case EFF_None:
                     {
                     break;
                     }

                     case EFF_SlideUp:
                     {
                        if ( Frame != 0 )
                        {
                           Data->Period -= EffectData * 4;
                           if (Data->Period < 57)
                              Data->Period = 57;
                           Data->Flags |= CHAN_Pitch;
                        }
                        break;
                     }

                     case EFF_SlideDown:
                     {
                        if ( Frame != 0 )
                        {
                           Data->Period += EffectData * 4;
                           if (Data->Period > 0x7FFF)
                              Data->Period = 0x7FFF;
                           Data->Flags |= CHAN_Pitch;
                        }
                        break;
                     }

                     case EFF_Porta:
                     {
                        if ( Frame != 0 && Data->PortaTarget )
                        {
                           if ( Data->Period < Data->PortaTarget )
                           {
                              Data->Period += EffectData * 4;
                              if ( Data->Period > Data->PortaTarget )
                                 Data->Period = Data->PortaTarget;
                           }
                           else
                           {
                              Data->Period -= EffectData * 4;
                              if ( Data->Period < Data->PortaTarget )
                                 Data->Period = Data->PortaTarget;
                           }
                           Data->Flags |= CHAN_Pitch;
                        }
                        break;
                     }

                     case EFF_Vibrato:
                     {
                        Data->VibratoSpeed = EffectData & 15;
                        Data->VibratoSize = EffectData >> 4;
                        Data->PeriodShift += VibratoTypes[Data->VibratoType%4][Data->VibratoPos%256] * ( Data->VibratoSize + 1 ) / 255;
                        Data->VibratoPos += Data->VibratoSpeed;
                        Data->Flags |= CHAN_Pitch;
                        break;
                     }

                     case EFF_SpecialA:
                     {
                        switch ( EffectData >> 4 )
                        {
                           case EFF_NoCommand:
                              break;
                           case EFF_NoLoopForwards:
                              if ( Data->Sample < HeaderData->NoOfSamples ) {
                                 ChanDat[Channel].NewLoopBegin = 0;
   	                         ChanDat[Channel].NewLoopEnd = ( Samples[Data->Sample].SizeEnd - Samples[Data->Sample].SizeStart ) * ( 1 + ( ( Samples[Data->Sample].Flags & FLAG_16Bit ) > 0 ) );
                                 ChanDat[Channel].NewLoopFlags = 0;
                                 ChanDat[Channel].SampleOffset = 0;
                                 Data->Flags |= CHAN_NewLoop;
                                 }
                              break;
                           case EFF_NoLoopBackwards:
                              if ( Data->Sample < HeaderData->NoOfSamples ) {
                                 ChanDat[Channel].NewLoopBegin = 0;
                                 ChanDat[Channel].NewLoopEnd = ( Samples[Data->Sample].SizeEnd - Samples[Data->Sample].SizeStart ) * ( 1 + ( ( Samples[Data->Sample].Flags & FLAG_16Bit ) > 0 ) );
  	                         ChanDat[Channel].SampleOffset = ( Samples[Data->Sample].SizeEnd - Samples[Data->Sample].SizeStart ) * ( 1 + ( ( Samples[Data->Sample].Flags & FLAG_16Bit ) > 0 ) );
                                 ChanDat[Channel].NewLoopFlags = INST_Reverse;
                                 Data->Flags |= CHAN_NewLoop;
                                 }
                              break;

                           // Stopgap
                           case EFF_StopLoop:
                              if ( Data->Sample < HeaderData->NoOfSamples ) {
                                 ChanDat[Channel].NewLoopBegin = 0;
                                 ChanDat[Channel].NewLoopEnd = ( Samples[Data->Sample].SizeEnd - Samples[Data->Sample].SizeStart ) * ( 1 + ( ( Samples[Data->Sample].Flags & FLAG_16Bit ) > 0 ) );
                                 ChanDat[Channel].NewLoopFlags = 0;
                                 Data->Flags |= CHAN_NewLoop;
                                 }
                              break;
                        }
                        break;
                     }

                     case EFF_Tremolo:
                     {
                        if ( Frame != 0 )
                        {
                           Data->TremoloSpeed = EffectData & 15;
                           Data->TremoloSize = EffectData >> 4;
                           Data->VolumeShift += VibratoTypes[Data->TremoloType%4][(Data->TremoloPos*4)%256] * ( Data->TremoloSize + 1 ) / 255;
                           Data->TremoloPos += Data->TremoloSpeed;
                           Data->Flags |= CHAN_Volume;
                        }
                        break;
                     }

                     case EFF_SetSampleOffset:
                     {
                        if (Frame == 0)
                           ChanDat[Channel].SampleOffset = EffectData * 0x100;
                        break;
                     }

                     case EFF_SetFineSampleOffset:
                     {
                        if (Frame == 0)
                           ChanDat[Channel].SampleOffset = EffectData * 4;
                        break;
                     }

                     case EFF_VolumeSlide:
                     {
                        if ( Frame != 0 )
                        {
                           if ( ( EffectData & 15 ) == 0 )
                              Data->Volume += ( EffectData >> 4 );
                           if ( ( EffectData & ~15 ) == 0 )
                              Data->Volume -= ( EffectData & 15 );
                           if ( Data->Volume < 0 )
                              Data->Volume = 0;
                           if ( Data->Volume > 255 )
                              Data->Volume = 255;
                           Data->Flags |= CHAN_Volume;
                        }
                        break;
                     }

                     case EFF_SetPanPos:
                     {
                        if ( Frame == 0 )
                        {
                           Data->PanPos = EffectData;
                           Data->Flags |= CHAN_Pan;
                        }
                        break;
                     }

                     case EFF_SetVolume:
                     {
                        if ( Frame == 0 )
                        {
                           Data->Volume = EffectData;
                           Data->Flags |= CHAN_Volume;
                        }
                        break;
                     }

                     case EFF_PatternBreak:
                     {
                        NewOrder = CurOrder + 1;
                        NewRow = EffectData;
                        break;
                     }

                     case EFF_Special:
                     {
                        switch ( EffectData >> 4 )
                        {
                           case EFF_SetVibratoValue:
                           {
                              Data->VibratoSize = EffectData & 15;
                              break;
                           }

                           case EFF_FineSlideUp:
                           {
                              if ( Frame != 0 )
                              {
                                 Data->Period -= EffectData & 15;
                                 if ( Data->Period < 57 )
                                    Data->Period = 57;
                                 Data->Flags |= CHAN_Pitch;
                              }
                              break;
                           }

                           case EFF_FineSlideDown:
                           {
                              if ( Frame != 0 )
                              {
                                 Data->Period += EffectData & 15;
                                 if ( Data->Period > 0x7FFF )
                                    Data->Period = 0x7FFF;
                                 Data->Flags |= CHAN_Pitch;
                              }
                              break;
                           }

                           case EFF_PatternDelay:
                           {
                              if ( Frame == 0 )
                                 if ( PatternDelay == 0 ) PatternDelay = EffectData & 15 + 1;
                              break;
                           }

                           case EFF_Retrig:
                           {
                              if ( Frame%( ( EffectData & 15 ) + 1 ) == 0 )
                              {
                                 Data->Flags &= ~CHAN_Cut;
                                 Data->Flags |= CHAN_Retrig;
                                 ChanDat[Channel].SampleOffset = 0;
                              }
                              break;
                           }

                           case EFF_FineVolumeSlideUp:
                           {
                              if ( Frame == 0 )
                              {
                                 Data->Volume += ( EffectData & 15 );
                                 if ( Data->Volume > 255 )
                                    Data->Volume = 255;
                                 Data->Flags |= CHAN_Volume;
                              }
                              break;
                           }

                           case EFF_FineVolumeSlideDown:
                           {
                              if ( Frame == 0 )
                              {
                                 Data->Volume -= ( EffectData & 15 );
                                 if ( Data->Volume < 0 )
                                    Data->Volume = 0;
                                 Data->Flags |= CHAN_Volume;
                              }
                              break;
                           }

                           case EFF_NoteCut:
                           {
                              if ( Frame == EffectData & 15 )
                              {
                                 Data->Flags &= ~CHAN_Retrig;
                                 Data->Flags |= CHAN_Cut;
                              }
                              break;
                           }

                           case EFF_NoteDelay:
                           {
                              if ( Frame < ( EffectData & 15 ) )
                                 Data->Flags &= ~CHAN_Retrig;
                              if ( Frame == ( EffectData & 15 ) )
                              {
                                 Data->Flags &= ~CHAN_Retrig;
                                 Data->Flags |= CHAN_Retrig;
                              }
                              break;
                           }
                        }
                        break;
                     }

                     case EFF_SetSpeed:
                     {
                        if ( Frame == 0 && EffectData)
                        {
                           if ( EffectData == 0 )
                              break;
                           if ( EffectData < 32 )
                           {
                              FramesARow = EffectData;
                              FInf.Speed = EffectData;
                           }
                           else
                           {
                              FrameLength = Device->SecondsToLen ( 60.0 / ( 24.0 * ( ( float ) EffectData ) ) );
                              FInf.Tempo = EffectData;
                           }
                        }
                        break;
                     }
                  }
               }

               //Process channel variables and apply changes
               if ( Data->Flags & CHAN_Instrument )
                  ChanDat[Channel].Sample = Data->Sample;

               if ( Data->Flags & CHAN_Pitch )
               {
                  int NotePeriod;
                  NotePeriod = ( Data->Period + Data->PeriodShift );
                  if ( NotePeriod > 0x7FFF )
                     NotePeriod = 0x7FFF;
                  if ( NotePeriod < 57 )
                     NotePeriod = 57;
                  else
                     ChanDat[Channel].Pitch = 8363*1712/(NotePeriod);
               }

               if (Data->Flags & CHAN_Volume)
               {
                  long CurrentVolume, CurrentPan;

                  Data->Flags |= CHAN_Volume;

                  CurrentVolume = Data->Volume + Data->VolumeShift;
                  boundv(0L,CurrentVolume,255L);
                  CurrentPan = (2*Data->PanPos - 0xF)*(PanSpan/2)/0xF;
                  boundv(-PanMax,CurrentPan,PanMax);

                  if ( UltType <= 1 )
                     ChanDat[Channel].MainVol = (int)((float) pow ( GUSCONST, CurrentVolume ) * (float) VolMax / (float) pow ( GUSCONST, 255 ));
                      //(((CurrentVolume*CurrentVolume*CurrentVolume)/255/255)*VolMax)/255;
                  else
                     ChanDat[Channel].MainVol = CurrentVolume*VolMax/255;

                  ChanDat[Channel].LeftVol = (ChanDat[Channel].MainVol * (PanMax - CurrentPan))/PanSpan;
                  ChanDat[Channel].RightVol = (ChanDat[Channel].MainVol * (PanMax + CurrentPan))/PanSpan;
                  ChanDat[Channel].Pan = CurrentPan;
               }

               if (SkipOrds == 0 && SkipRows == 0) {
                  ChanDat[Channel].Flags = Data->Flags;
                  Data->Flags = 0;
                  }
            }

            Device->SetFrameInfo(&FInf);

            FInf.SongTime += FrameLength;
            if (SkipOrds == 0 && SkipRows == 0)
            {
               long Rc;
               if ((Rc = Device->Compute ( &ChanDat, FrameLength )) != 0 )
               {
                  ChanDat.free();
                  delete [] ChannelData;
                  delete [] LastArgs;
                  delete [] CurrentPattern;
                  return Rc;
               }
            }
         }

         if ( PatternDelay )
            if ( --PatternDelay )
               continue;

         if ( NewOrder != -1 )
            break;
         else
            Row++;
      }
      if ( NewOrder == -1 )
         CurOrder++;
      else
         CurOrder = NewOrder;
   }
   
   ChanDat.free();
   delete [] ChannelData;
   delete [] LastArgs;
   delete [] CurrentPattern;
   return PLAYOK;
}

