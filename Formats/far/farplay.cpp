// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   FARFormat - Muse file format to handle FAR files.
   
   FAR is an obscure little format.. 
   
   ##################################################################### */
									/*}}}*/
// Includes								/*{{{*/
#include <muse.h>
#include <farform.h>
#include <math.h>

#include "farstruc.h"
   									/*}}}*/

// FARFormat::Play - Plays the song				
// ---------------------------------------------------------------------
/* */
long museFARFormat::Play(museOutputBase *Device,musePlayerControl *Control)
{
   if ( ( Header == 0 ) || ( Orders == 0 ) || ( Orders->NoOfOrders == 0 ) )
      return PLAYFAIL_NotLoaded;

   CmdInfoFAR *ChannelData = new CmdInfoFAR[MAXNOCHANS];

   unsigned short CurOrder = 0;
   int NewRow = -1;
   int x;

   memset(ChannelData,0,sizeof(*ChannelData)*MAXNOCHANS);

   // Definintions of tracking variables
   unsigned short FramesARow = 16;
   unsigned long FrameLength = Device->SecondsToLen ( (float) Header->Tempo / 32.0 / 16.0 );
   int CurTempo = Header->Tempo * 4;

   // Ennumerate the # of channels
   Sequencebool BoolSeq;
   BoolSeq.construct();
   BoolSeq.reserve(MAXNOCHANS);

   for ( x = 0; x < MAXNOCHANS; x++ )
      BoolSeq[x] = true;

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
   ChanDat[15].Flags = 0;
   for ( x = 0; x < MAXNOCHANS; x++ )
   {
      ChanDat[x].Sample = 0xFFFF;
      ChanDat[x].ModuleChannel = x;
   }

   // Construct array to hold row bitmap
   PlayedRec *OrdPlayList = new PlayedRec[Orders->NoOfOrders + 1];
   memset(OrdPlayList,0,(Orders->NoOfOrders + 1)*sizeof(PlayedRec));
   museFrameInfo FInf;
   FInf.GlobalVolume = VolMax;
   FInf.Tempo = Header->Tempo;
   FInf.Speed = FramesARow;
   FInf.SongTime = 0;
   int SkipOrds = 0;
   int SkipRows = 0;
   
   while (CurOrder < Orders->NoOfOrders)
   {
      int Row = 0;
      int NewOrder = -1;

      if (Orders->Orders[CurOrder] == 255)
         break;

      FInf.Order = CurOrder;
      FInf.Pattern = Orders->Orders[CurOrder];

      if (SkipOrds != 0)
         SkipOrds--;

      while (Row < ( Orders->PatternLength[Orders->Orders[CurOrder]] - 2) / 64 )
      {
         if (NewRow != -1)
         {
            Row = NewRow;
            NewRow = -1;
            continue;
         }

         // Check that this row has never been played before
         if (AllowLoop == false && (CurOrder) < Orders->NoOfOrders)
         {
             if ((OrdPlayList[CurOrder].Rows[Row/32] & (1 << (Row % 32))) != 0)
             {
                 CurOrder = Orders->NoOfOrders;
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
                     NRow += (Orders->PatternLength[Orders->Orders[NewOrder]] - 2) / 64;
                     if (NRow >  0)
                        break;
                  }
               }
               else
               {
                  while (1)
                  {
                     if (NRow < (Orders->PatternLength[Orders->Orders[NewOrder]] - 2) / 64)
                        break;
                     NRow -= (Orders->PatternLength[Orders->Orders[NewOrder]] - 2) / 64;
                     NewOrder++;
                     if (Orders->Orders[NewOrder] < Orders->NoOfPatterns)
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
                  memset(ChannelData,0,sizeof(ChannelData));
                  for (int I = 0; I != MAXNOCHANS; I++)
                     ChannelData[I].Flags = CHAN_Retrig;

                  CurTempo = Header->Tempo * 4;
                  FInf.Tempo = Header->Tempo;

                  for (museChannel *Iter = ChanDat.begin(); Iter != ChanDat.end(); Iter++)
                     Iter->Sample = 0xFFFF;
                  FInf.SongTime = 0;

                  memset(OrdPlayList,0,(Orders->NoOfOrders + 1)*sizeof(PlayedRec));
                  SkipOrds = 0;
                  SkipRows = 0;
               }
               break;
            }
         }

         //Loop through each frame
         for ( int Frame = 0; Frame < FramesARow; Frame++ )
         {
            FInf.Frame = Frame;
            int Channel = 0;
             
            while ( Channel < MAXNOCHANS )
            {
               CmdInfoFAR *Data = &ChannelData[Channel];

               int Note = (&Patterns[Orders->Orders[CurOrder]]->Data)[16*Row+Channel].Note;
               int Instrument = (&Patterns[Orders->Orders[CurOrder]]->Data)[16*Row+Channel].Instrument;
               int Volume = (&Patterns[Orders->Orders[CurOrder]]->Data)[16*Row+Channel].Volume;
               int Effect = (&Patterns[Orders->Orders[CurOrder]]->Data)[16*Row+Channel].Instrument / 16;
               int EffectData = (&Patterns[Orders->Orders[CurOrder]]->Data)[16*Row+Channel].Instrument & 15;

               //Check for a note change
               if ( Note && Frame == 0 )
               {
                  if ( Instrument > 64 || !( SampleMap[(Instrument)/32] >> ( (Instrument)%32 ) ) )
                  {
                     Instrument = 0;
                     Data->Flags &= ~CHAN_Retrig;
                     Data->Flags |= CHAN_Cut;
                  }
                  else
                  {
                     Data->Sample = Instrument;
                     Data->Flags |= CHAN_Instrument;
                     Data->Note = Note;
                     if ( Effect == EFF_Porta )
                     {
                        Data->Flags |= CHAN_Volume | CHAN_Pan;
                        Data->PortaTarget = (int)(8363.0 * pow ( 2.0, Data->Note / 12.0 - 1));
                     }
                     else
                     {
                        Data->Flags &= ~CHAN_Cut;
                        Data->Flags |= CHAN_Retrig | CHAN_Volume | CHAN_Pan | CHAN_Pitch;
                        ChanDat[Channel].SampleOffset = 0;
                        Data->Pitch = (int)(8363.0 * pow ( 2.0, Data->Note / 12.0 - 1));
                        Data->PortaSpeed = 0;
                     }
                  }
               }
               if ( Data->PortaSpeed )
               {
                  if ( Data->Pitch < Data->PortaTarget )
                  {
                     Data->Pitch += Data->PortaSpeed;
                     if ( Data->Pitch >= Data->PortaTarget )
                     {
                        Data->Pitch = Data->PortaTarget;
                        Data->PortaSpeed = 0;
                     }
                  }
                  else
                  {
                     Data->Pitch += Data->PortaSpeed;
                     if ( Data->Pitch <= Data->PortaTarget )
                     {
                        Data->Pitch = Data->PortaTarget;
                        Data->PortaSpeed = 0;
                     }
                  }
               }
               if ( Volume )
               {
                  int TempVol;
                  TempVol = ( Volume - 1 ) / 16 + ( ( Volume - 1 ) & 0x0F ) * 16;
                  if ( Data->Volume != TempVol )
                  {
                     Data->Volume = TempVol;
                     Data->Flags |= CHAN_Volume;
                  }
               }

               switch ( Effect )
               {
                  case EFF_None:
                     break;

                  case EFF_SlideUp:
                     if ( Frame%4 == 0 )
                        Data->Pitch += EffectData;
                     break;

                  case EFF_SlideDown:
                     if ( Frame%4 == 0 )
                        Data->Pitch -= EffectData;
                     break;

                  case EFF_Porta:
                     Data->PortaSpeed = ( Data->PortaTarget - Data->Pitch ) / EffectData;
                     break;

                  case EFF_MultiRetrig:
                     Data->RetrigCount += EffectData;
                     if ( Data->RetrigCount >= 16 )
                     {
                        Data->RetrigCount -= 16;
                        Data->Flags &= ~CHAN_Cut;
                        Data->Flags |= CHAN_Retrig;
                        ChanDat[Channel].SampleOffset = 0;
                     }
                     break;

                  case EFF_FineTempoUp:
                     CurTempo -= EffectData;
                     if ( CurTempo < 1 )
                        CurTempo = 1;
                     FInf.Tempo = CurTempo/4;
                     FrameLength = Device->SecondsToLen ( (float) CurTempo / 128.0 / 16.0 );
                     break;

                  case EFF_FineTempoDown:
                     CurTempo += EffectData;
                     if ( CurTempo > 128 )
                        CurTempo = 128;
                     FInf.Tempo = CurTempo/4;
                     FrameLength = Device->SecondsToLen ( (float) CurTempo / 128.0 / 16.0 );
                     break;

                  case EFF_Tempo:
                     FrameLength = Device->SecondsToLen ( (float) EffectData / 32.0 / 16.0 );
                     CurTempo = EffectData * 4;
                     FInf.Tempo = EffectData;
                     break;
               }

               if ( Data->Flags & CHAN_Instrument )
                  ChanDat[Channel].Sample = Data->Sample;

               if ( Data->Flags & CHAN_Pitch )
                  ChanDat[Channel].Pitch = Data->Pitch;

               if ( Data->Flags & (CHAN_Volume | CHAN_Pan) )
               {
                  ChanDat[Channel].Pan = (2*Header->Pan[Channel] - 0xF)*(PanSpan/2)/0xF;
                  ChanDat[Channel].MainVol = (Data->Volume*VolMax)/255;
                  ChanDat[Channel].LeftVol = (ChanDat[Channel].MainVol*(PanMax - ChanDat[Channel].Pan))/PanSpan;
                  ChanDat[Channel].RightVol = (ChanDat[Channel].MainVol*(PanMax + ChanDat[Channel].Pan))/PanSpan;
               }

               if (SkipRows == 0 && SkipOrds == 0) {
                  ChanDat[Channel].Flags = Data->Flags;
                  Data->Flags = 0;
                  }
               
               Channel++;
            }

            Device->SetFrameInfo(&FInf);
            FInf.SongTime += FrameLength;

            if (SkipRows == 0 && SkipOrds == 0)
            {
               long Rc = 0;
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
      {
         CurOrder++;
         if (CurOrder == Orders->NoOfOrders)
            CurOrder = Orders->LoopLocation;
      }
      else
         CurOrder = NewOrder;
   }

   ChanDat.free();
   delete [] ChannelData;
   return PLAYOK;
}
