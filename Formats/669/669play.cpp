// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   669Format - Muse file format to handle 669 files.

   ##################################################################### */
									/*}}}*/
// Includes								/*{{{*/
#include <muse.h>
#include <669form.h>
#include <stdlib.h>
#include <math.h>

#include "669struc.h"
   									/*}}}*/

// 669Format::Play - Plays the song				
// ---------------------------------------------------------------------
/* */
long muse669Format::Play(museOutputBase *Device,musePlayerControl *Control)
{
   if ( ( Header == 0) || ( Header->OrderList[0] == 0xFF ) || ( Header->NoOfPatterns == 0 ) || ( Header->NoOfSamples == 0 ) )
      return PLAYFAIL_NotLoaded;

   const signed short VSineTable[64] = {0,24,49,74,97,120,141,161,180,197,212,224,
                                        235,244,250,253,255,253,250,244,235,224,
                                        212,197,180,161,141,120,97,74,49,24,0,-24,
                                        -49,-74,-97,-120,-141,-161,-180,-197,-212,-224,
                                        -235,-244,-250,-253,-255,-253,-250,-244,-235,-224,
                                        -212,-197,-180,-161,-141,-120,-97,-74,-49,-24};
   
   CmdInfo669 *ChannelData = new CmdInfo669[MAXNOCHANS];

   unsigned short CurOrder = 0;
   unsigned int x;
   int NoOfOrders;

   memset(ChannelData,0,sizeof(*ChannelData)*MAXNOCHANS);

   // Convert to more usable panning values
   for ( x = 0; x < MAXNOCHANS; x++ )
   {
      if (x % 2)
         ChannelData[x].PanPos = (2*0xC - 0xF)*(PanSpan/2)/0xF;
      else
         ChannelData[x].PanPos = (2*0x3 - 0xF)*(PanSpan/2)/0xF;

      ChannelData[x].LastCommand = 0xFF;
      ChannelData[x].Flags |= CHAN_Pan;
   }

   // Definintions of tracking variables
   unsigned long FrameLength = Device->SecondsToLen ( 60.0 / ( 24.0 * ( ( float ) 80 ) ) );

   // Ennumerate the # of channels
   Sequencebool BoolSeq;
   BoolSeq.construct();
   BoolSeq.reserve(32);

   for (x = 0; x < MAXNOCHANS; x++)
      BoolSeq[x] = true;

   Device->SetChannelMask(&BoolSeq);
   Device->SetVolumeScale (0);
   BoolSeq.free();

   // Load the samples into the mixer.
   SequenceSample Samples;
   Samples.construct();
   GetSongSamples(Samples);
   Device->LoadSamples(&Samples);
   Samples.free();

   SequenceChannel ChanDat;
   ChanDat.construct();
   ChanDat[7].Flags = 0;
   for (x = 0; x < MAXNOCHANS; x++)
   {
      ChanDat[x].Sample = 0xFFFF;
      ChanDat[x].ModuleChannel = x;
   }

   Restart:

   // Construct array to hold row bitmap
   NoOfOrders = 0;
   for ( x = 0; x < sizeof( Header->OrderList); x++ ) if ( Header->OrderList[x] < Header->NoOfPatterns ) NoOfOrders++;
   PlayedRec *OrdPlayList = new PlayedRec[x + 1];
   memset(OrdPlayList,0,(NoOfOrders + 1)*sizeof(PlayedRec));
   museFrameInfo FInf;
   FInf.GlobalVolume = VolMax;
   FInf.Tempo = 80;
   FInf.SongTime = 0;
   int SkipRows = 0;
   int SkipOrds = 0;
   int NewRow = -1;
   
   while ( Header->OrderList[CurOrder] < Header->NoOfPatterns )
   {
      int Row = 0, Rows;
      int NewOrder = -1;
      char FramesARow;
      FramesARow = Header->TempoList[Header->OrderList[CurOrder]];
      FInf.Speed = FramesARow;
      FInf.Order = CurOrder;
      FInf.Pattern = Header->OrderList[CurOrder];

      Rows = Header->BreakList[Header->OrderList[CurOrder]];

      if (SkipOrds != 0)
         SkipOrds--;

      while ( Row <= Rows )
      {
         if (NewRow != -1)
         {
            Row = NewRow;
            NewRow = -1;
            continue;
         }

         // Check that this row has never been played before
         if (AllowLoop == false && (CurOrder) < NoOfOrders)
         {
             if ((OrdPlayList[CurOrder].Rows[Row/32] & (1 << (Row % 32))) != 0)
             {
                 CurOrder = NoOfOrders;
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
                     NRow += Header->BreakList[Header->OrderList[NewOrder]];
                     if (NRow >  0)
                        break;
                  }
               }
               else
               {
                  while (1)
                  {
                     if (NRow <  Header->BreakList[Header->OrderList[NewOrder]])
                        break;
                     NRow -= Header->BreakList[Header->OrderList[NewOrder]];
                     NewOrder++;
                     if (Header->OrderList[NewOrder] < Header->NoOfPatterns)
                     {
                        NRow = 0;
                        break;
                     }
                  }
               }
               NewRow = NRow;

               // Cut all the channels notes
               for (int I = 0; I != MAXNOCHANS; I++)
               {
                  ChannelData[I].Flags &= ~CHAN_Retrig;
                  ChannelData[I].Flags |= CHAN_Cut;
               }

               // Jump to start, reset all vars
               if ((NewOrder == -1) && (NewRow == 0))
               {
                  NewRow = -1;
   
                  FramesARow = Header->TempoList[Header->OrderList[NewOrder]];
                  FInf.Speed = FramesARow;
                  memset(ChannelData,0,sizeof(ChannelData));

                  for (int I = 0; I != MAXNOCHANS; I++)
                     ChannelData[I].Flags = CHAN_Retrig;

                  for (museChannel *Iter = ChanDat.begin(); Iter != ChanDat.end(); Iter++)
                     Iter->Sample = 0xFFFF;
                  FInf.SongTime = 0;

                  memset(OrdPlayList,0,(NoOfOrders + 1)*sizeof(PlayedRec));
                  SkipRows = 0;
                  SkipOrds = 0;
               }
               break;
            }
         }

         _669Row *DataPointer;
         DataPointer = PatternData[Header->OrderList[CurOrder]]->Rows[Row];

         //Loop through each frame
         for ( int Frame = 0; Frame < FramesARow; Frame++ )
         {
            FInf.Frame = Frame;
             
            int Channel = 0;
            while ( Channel < MAXNOCHANS )
            {
               CmdInfo669 *Data = &ChannelData[Channel];

               if ( Frame == 0 )
               {
//                  Data->PortaTarget = 0;
                  if (DataPointer->Data[0] == 0xFE)
                  {
                     Data->Volume = (DataPointer->Data[1] & 15);
                     Data->Flags |= CHAN_Volume;
                  }
                  else
                     if ( DataPointer->Data[0] != 0xFF )
                     {
                        if ( (DataPointer->Data[2] >> 4) == EFF_Porta || ( DataPointer->Data[2] == 0xFF && Data->LastCommand == EFF_Porta ) )
                        {
                           Data->PortaTarget = (int)(8363 / 4 * pow ( 2, (float) ( DataPointer->Data[0] >> 2 ) / 12.0 ));
                        }
                        else
                        {
                           Data->BasePitch = Data->Pitch = (int)(8363 / 4 * pow ( 2, (float) ( DataPointer->Data[0] >> 2 ) / 12.0 ));
                           Data->Sample = ( ( ( DataPointer->Data[0] & 3 ) << 4 ) + ( DataPointer->Data[1] >> 4 ) );
                           if (Data->Sample > Header->NoOfSamples)
                              Data->Sample = Header->NoOfSamples - 1;
                           Data->Volume = (DataPointer->Data[1] & 15);
                           Data->Flags &= ~CHAN_Cut;
                           Data->Flags |= CHAN_Retrig | CHAN_Pitch | CHAN_Volume | CHAN_Instrument;
                           ChanDat[Channel].SampleOffset = 0;
                           Data->LastCommand = 0xFF;
                        }
                     }
               }
               int Command = DataPointer->Data[2];
               if ( Command != 0xFF && Command != Data->LastCommand )
               {
                  Data->Pitch = Data->BasePitch;
                  Data->Flags |= CHAN_Pitch;
               }
               if ( Command != 0xFF && ( Command & 15 ) != 0 )
                  Data->LastCommand = Command;
               else
                  Command = Data->LastCommand;

               int Period;
               if ( Command != 0xFF ) switch ( Command >> 4 )
               {
                  case EFF_SlideUp:
                     if (!Data->BasePitch)
                        break;
                     Period = 14317056 / Data->BasePitch;
                     Period -= (Command & 15)*4;
                     if (Period < 32)
                        Period = 32;
                     Data->BasePitch = Data->Pitch = 14317056 / Period;
                     Data->Flags |= CHAN_Pitch;
                     break;

                  case EFF_SlideDown:
                     if (!Data->BasePitch)
                        break;
                     Period = 14317056 / Data->BasePitch;
                     Period += (Command & 15)*4;
                     if (Period > 0x7FFF)
                        Period = 0x7FFF;
                     Data->BasePitch = Data->Pitch = 14317056 / Period;
                     Data->Flags |= CHAN_Pitch;
                     break;

                  case EFF_Porta:
                     if (!Data->BasePitch)
                        break;
                     if (!Data->PortaTarget)
                        break;

                     if (Data->PortaTarget > Data->Pitch)
                     {
                        Period = 14317056 / Data->BasePitch;
                        Period -= (Command & 15)*4;
                        if (Period < 32)
                           Period = 32;

                        Data->Pitch = 14317056 / Period;

                        if (Data->Pitch > Data->PortaTarget)
                           Data->Pitch = Data->PortaTarget;

                        Data->BasePitch = Data->Pitch;
                        Data->Flags |= CHAN_Pitch;
                     }
                     else
                     {
                        Period = 14317056 / Data->BasePitch;
                        Period += (Command & 15)*4;
                        if (Period > 0x7FFF)
                           Period = 0x7FFF;
                        Data->Pitch = 14317056 / Period;
                        if (Data->Pitch < Data->PortaTarget)
                           Data->Pitch = Data->PortaTarget;
                        Data->BasePitch = Data->Pitch;
                        Data->Flags |= CHAN_Pitch;
                     }
                     break;

                  case EFF_FreqAdjust:
                     Data->Pitch = (int)(Data->BasePitch * pow ( 2, (float) ( ( Command & 15 ) - 8 ) / 64.0 / 12.0 ));
                     break;

                  case EFF_Vibrato:
                    Data->Pitch = Data->BasePitch * ( 12*256 + VSineTable[Data->Vibrato] ) / 12 / 256;
                     if ( Frame == 0 ) break;
                      Data->Vibrato += 4;
                     Data->Flags |= CHAN_Pitch;
                     break;

                  case EFF_SetTempo:
                     if ( Command & 15 )
                        FramesARow = Command & 15;
                     else
                        FramesARow = Header->TempoList[Header->OrderList[CurOrder]];
                     FInf.Speed = FramesARow;
                     break;
               }

               if ( Data->Flags & CHAN_Pitch )
               {
                  ChanDat[Channel].Pitch = Data->Pitch;
               }

               if (Data->Flags & CHAN_Volume)
               {
                  ChanDat[Channel].MainVol = Data->Volume * VolMax / 15;
                  boundv(0,ChanDat[Channel].MainVol,VolMax);

                  ChanDat[Channel].Pan = Data->PanPos;
                  ChanDat[Channel].LeftVol = (ChanDat[Channel].MainVol*(PanMax - ChanDat[Channel].Pan))/PanSpan;
                  ChanDat[Channel].RightVol = (ChanDat[Channel].MainVol*(PanMax + ChanDat[Channel].Pan))/PanSpan;
               }

               if ( Data->Flags & CHAN_Instrument )
               {
                  ChanDat[Channel].Sample = Data->Sample;
               }

               if (SkipRows == 0 && SkipOrds == 0) {
                  ChanDat[Channel].Flags = Data->Flags;
                  Data->Flags = 0;
                  }
               
               Channel++;
               DataPointer++;
            }
            DataPointer -= 8;

            Device->SetFrameInfo(&FInf);

            FInf.SongTime += FrameLength;
            if (SkipRows == 0 && SkipOrds == 0)
            {
               long Rc;
               if ((Rc = Device->Compute ( &ChanDat, FrameLength )) != 0 )
               {
                  ChanDat.free();
                  delete [] ChannelData;
                  return Rc;
               }
            }
         }
         Row++;
      }
      if (NewOrder == -1)
         CurOrder++;
      else
         CurOrder = NewOrder;
   }

   if ( Header->LoopOrder && AllowLoop == true )
   {
      CurOrder = Header->LoopOrder;
      goto Restart;
   }

   ChanDat.free();
   delete [] ChannelData;
   return PLAYOK;
}

