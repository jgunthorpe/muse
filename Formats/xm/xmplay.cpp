// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   XMFormat - Muse file format to handle XM files.

   The XM player is very large and moderately complecated. The main size
   is the two command switches.
   
   ##################################################################### */
									/*}}}*/
// Includes								/*{{{*/
#include <muse.h>
#include <xmform.h>
#include <math.h>

#include "xmstruc.h"
   									/*}}}*/
// XMFormat::Glissando - Round pitch values to the nearest note		/*{{{*/
// ---------------------------------------------------------------------
/* This helps implement the XM glissando mode by rouding the period
   values. */
unsigned long museXMFormat::Glissando(long Frequency,long FineTune)
{
   if (1 - FineTune*0.0037164 == 0)
      return Frequency;
   
   float Current = 14317056/4/(428 * (1 - FineTune * 0.0037164)) / 256;

   while (Current * 2 <= Frequency) 
      Current *= 2;

   while (Current * 1.059463094 <= Frequency) 
      Current *= 1.059463094;

   if (Frequency - Current <= Current * 1.059463094 - Frequency) 
      return (long)Current;
   else 
      return (long)(Current * 1.059463094);
}
									/*}}}*/

// XMFormat::Play - Plays the XM to the output device 
// ---------------------------------------------------------------------
/* This converts the XM to muse patterndata format */
long museXMFormat::Play(museOutputBase *Device,musePlayerControl *Control)
{
   // See if its okay to play
   if ((Header == 0) || (Header->SongLength == 0) || 
       (Header->NoOfPatterns == 0) || (NoOfSamples == 0))
      return PLAYFAIL_NotLoaded;

   // Retrig tables
   const signed char RetrigAdd[16] = {0,-1,-2,-4,-8,-16,0,0,0,1,2,4,8,16,0,0};
   const signed char RetrigMul[16] = {16,16,16,16,16,16,10,8,16,16,16,16,16,
                                      16,24,32};
   
   /* These two define a mask for the info byte storage location. Some
      commands store 00 info data per channel per effect, others only per
      effect. */
   const char RepeatInfo[36] = {0,1,1,1,1,1,1,1,0,0,1,0,0,0,2,0,0,1,0,0,0,0,0,
                                0,0,1,0,1,0,0,0,0,0,1,0,0};
   const char SpecialRepeatInfo[16] = {0,1,1,0,0,0,0,0,0,0,1,1,0,0,0,0};

   // Main channel structure
   CmdInfoXM *ChannelData = new CmdInfoXM[MAXNOCHANS];
   memset(ChannelData,0,MAXNOCHANS*sizeof(CmdInfoXM));

   // General things
   int x, y, z;
   int GlobalVolume = 64;
   int GlobalFlag = 0;
   int GlissandoControl = 0;
   unsigned char PatternDelay = 0;
   unsigned short CurOrder = 0;
   int NewRow = -1;
   short LastOrder = -1;
   short LastRow = -2;
   unsigned char *LastPos = 0;
   unsigned char SpecialLastArgs[MAXNOCHANS][16];

   unsigned char LastArgs[MAXNOCHANS][36];
   memset(LastArgs,0,MAXNOCHANS*36);

   // Waveform tables
   signed short VSineTable[256];
   for (x = 0; x < 256; x++) 
      VSineTable[x] = (long)(256 * sin (x * 3.14159265 / 128));
   
   signed short VSquare[256];
   for (x = 0; x < 256; x++) 
      VSquare[x] = (x < 128 ? 128 : -128);
   
   signed short VRamp[256];
   for (x = 0; x < 256; x++) 
      VRamp[x] = (x - 128) * 2;

   signed short VRandom[256];
   for (x = 0; x < 256; x++) 
      VRandom[x] = (rand() % 510) - 255;

   signed short *VibratoTypes[4] = { VSineTable, VSquare, VRamp, VSineTable };

   // Definintions of tracking variables
   unsigned short FramesARow = Header->InitialTempo;
   unsigned long FrameLength = Device->SecondsToLen(60.0/(24.0*(
			       (float)Header->InitialBPM)));

   if (FramesARow == 0)
      FramesARow = 4;

   // Ennumerate the # of channels, this configures the output device.
   Sequencebool BoolSeq;
   BoolSeq.reserve(MAXNOCHANS);

   for (x = 0; x < MAXNOCHANS; x++) 
      if (x < Header->NoOfChannels) 
	 BoolSeq[x] = true; 
      else 
         BoolSeq[x] = false;
   Device->SetChannelMask(&BoolSeq);
   BoolSeq.free();
   
   // Config the output devices volume scaling
   Device->SetVolumeScale(0);

   // Load the samples into the mixer.
   SequenceSample Samples;
   GetSongSamples(Samples);
   Device->LoadSamples(&Samples);
   Samples.free();

   // Initialize the mixers channel record
   SequenceChannel ChanDat;
   ChanDat.construct();
   ChanDat.reserve(MAXNOCHANS);
   for (x = 0; x < Header->NoOfChannels; x++)
   {
      ChanDat[x].Sample = 0xFFFF;
      ChanDat[x].ModuleChannel = x;
   }

   // Construct array to hold row bitmap
   PlayedRec *OrdPlayList = new PlayedRec[Header->SongLength + 1];
   memset(OrdPlayList,0,(Header->SongLength + 1)*sizeof(PlayedRec));

   // Initialize the frame info record
   museFrameInfo FInf;
   FInf.GlobalVolume = GlobalVolume * VolMax / 64;
   FInf.Tempo = Header->InitialBPM;
   FInf.Speed = Header->InitialTempo;
   FInf.SongTime = 0;
   
   unsigned long SkipOrds = 0;
   unsigned long SkipRows = 0;

   while (CurOrder < Header->SongLength) 
   {
      int Row = 0;
      if (NewRow != -1) 
      {
         Row = NewRow;
         NewRow = -1;
      }
      
      int NewOrder = -1;
      int RowRepeatFlag = 0;

      // Error check.
      if (Header->OrderList[CurOrder] > Header->NoOfPatterns)
      {
         ChanDat.free();
         delete [] ChannelData;
         return PLAYFAIL_NotLoaded;
      }

      FInf.Order = CurOrder;
      FInf.Pattern = Header->OrderList[CurOrder];

      // Smart Order Skipping
      if (SkipOrds != 0)
         SkipOrds--;

      while (Row < PatternData[Header->OrderList[CurOrder]].NoOfRows) 
      {
         int LoopRow = 0;
         int CurrentLoop = 0;

         // Check that this row has never been played before
         if (AllowLoop == false && (CurOrder) < Header->SongLength)
         {
            if ((OrdPlayList[CurOrder].Rows[Row/32] &
                (1 << (Row % 32))) != 0)
            {
               CurOrder = Header->SongLength;
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
                     NRow += PatternData[Header->OrderList[NewOrder]].NoOfRows;
                     if (NRow >  0)
                        break;
                  }
               }
               else
               {
                  while (1)
                  {
                     if (NRow <  PatternData[Header->OrderList[NewOrder]].NoOfRows)
                        break;
                     NRow -= PatternData[Header->OrderList[NewOrder]].NoOfRows;
                     NewOrder++;
                     if (NewOrder > Header->SongLength)
                     {
                        NRow = 0;
                        break;
                     }
                  }
               }
               NewRow = NRow;

               // Cut all the channels notes
               for (x = 0; x < Header->NoOfChannels; x++)
               {
                  ChannelData[x].Flags &= ~CHAN_Retrig;
                  ChannelData[x].Flags |= CHAN_Cut;
               }

               // Jump to start, reset all vars
               if ((NewOrder == 0) && (NewRow == 0))
               {
                  GlobalVolume = 64;
                  GlobalFlag = 0;
                  GlissandoControl = 0;
                  PatternDelay = 0;
                  NewRow = -1;
                  
                  for (x = 0; x < Header->NoOfChannels; x++)
                  {
                     ChanDat[x].Sample = 0xFFFF;
                     ChanDat[x].ModuleChannel = x;
                  }

                  FramesARow = Header->InitialTempo;
                  FrameLength = Device->SecondsToLen (60.0 / (24.0 * ((float) Header->InitialBPM)));
                  if (FramesARow == 0) FramesARow = 4;

                  memset (ChannelData, 0, MAXNOCHANS*sizeof (CmdInfoXM));
                  memset (LastArgs, 0, MAXNOCHANS*36);
                  for (x = 0; x < Header->NoOfChannels; x++)
                     ChannelData[x].Flags |= CHAN_Cut;

                  FInf.GlobalVolume = GlobalVolume * VolMax / 64;
                  FInf.Tempo = Header->InitialBPM;
                  FInf.Speed = FramesARow;
                  FInf.SongTime = 0;
               }
               break;
            }
         }

         /* Get the pointer to the current row. This decodes the xm pattern
	    data. The last position in the pattern data is cached so that
	    lookup is fast. When we are jumping around the pattern is
	    decrypted from the begining. */
         unsigned char *DataPointer;
         if (LastOrder == CurOrder && LastRow == Row - 1) 
            DataPointer = LastPos;
         else 
	 {
            DataPointer = PatternData[Header->OrderList[CurOrder]].PatternData;
            x = 0;
            while (x != Row && DataPointer != 0) 
	    {
               for (y = 0; y < Header->NoOfChannels; y++) 
	       {
		  // Optimize if all are set on.
                  if (!(*DataPointer & 128)) 
                     DataPointer += 5;
                  else 
		  {
                     z = *DataPointer++;
                     if (z & 1) 
			DataPointer++;
                     if (z & 2) 
			DataPointer++;
                     if (z & 4) 
			DataPointer++;
                     if (z & 8) 
			DataPointer++;
                     if (z & 16) 
			DataPointer++;
		  }
	       }
               x++;
	    }

	    if (DataPointer == 0)
	       x = Row;
	 }

         // Aquire the parameter data for each channel.
         for (int Channel = 0; Channel < Header->NoOfChannels && DataPointer != 0; Channel++) 
	 {
	    // Optimize if all are set on
            if (!(*DataPointer & 128)) 
	    {
               ChannelData[Channel].NoteParm = *(DataPointer++);
               ChannelData[Channel].InstrumentParm = *(DataPointer++);
               ChannelData[Channel].VolumeParm = *(DataPointer++);
               ChannelData[Channel].EffectParm = *(DataPointer++);
               ChannelData[Channel].EffectDataParm = *(DataPointer++);
	    } 
	    else 
	    {
               z = *(DataPointer++);
               ChannelData[Channel].NoteParm = 0;
               ChannelData[Channel].InstrumentParm = 0;
               ChannelData[Channel].VolumeParm = 0;
               ChannelData[Channel].EffectParm = 0;
               ChannelData[Channel].EffectDataParm = 0;

               if (z & 1) 
		  ChannelData[Channel].NoteParm = *(DataPointer++);
               if (z & 2) 
		  ChannelData[Channel].InstrumentParm = *(DataPointer++);
               if (z & 4) 
		  ChannelData[Channel].VolumeParm = *(DataPointer++);
               if (z & 8) 
		  ChannelData[Channel].EffectParm = *(DataPointer++);
               if (z & 16) 
		  ChannelData[Channel].EffectDataParm = *(DataPointer++);
	    }
	 }
	 
	 // Stash the position of the next row
         LastPos = DataPointer;
         LastOrder = CurOrder;
         LastRow = Row;

         //Loop through each frame
         for (int Frame = 0; Frame < FramesARow; Frame++) 
	 {
            int Channel;
            FInf.Frame = Frame;

            for (Channel = 0; Channel < Header->NoOfChannels; Channel++) 
	    {
               CmdInfoXM *Data = &ChannelData[Channel];
               int Note = Data->NoteParm;
               int Instrument = Data->InstrumentParm;
               int Volume = Data->VolumeParm;
               int Effect = Data->EffectParm;
               int EffectData = Data->EffectDataParm;

	       // Deal with 00 info bytes.
               if (EffectData == 0) 
	       {
		  // 1 indicates the info byte is per effect per channel
                  if (RepeatInfo[Effect] == 1) 
		     EffectData = LastArgs[Channel][Effect];
		  
		  /* 2 indicates the infobyte high nibble is part of the
		     command. If SRI = 0 then 00 is a valid info byte. */
                  if (RepeatInfo[Effect] == 2 && 
		      SpecialRepeatInfo[EffectData >> 4] == 1) 
		     EffectData = SpecialLastArgs[Channel][EffectData >> 4];
	       } 
	       else 
	       {
		  // Stash the non 0 infobyte
                  if (RepeatInfo[Effect] == 2) 
		     SpecialLastArgs[Channel][EffectData >> 4] = EffectData;
		  else 
		     LastArgs[Channel][Effect] = EffectData;
	       }

               // Check for a note change
               if (Note == 97 && Frame == 0)
               {
		  // 97 = Note off
                  Note = 0;
                  Data->KeyState = 0;
                  Data->TimeOff = Data->TimeCounter;
               }
               else
                  if (Note != 0 && Frame == 0)
	          {
                     Data->Note = Note;

		     // Recompute the sub sample indexes
		     if (Data->InstData == 0)
			Data->SubSampleNo = 0;
		     else
			Data->SubSampleNo = Data->InstData->NoteMap[Data->Note-1];

		     // Range check the sample info
		     if (Data->InstData == 0 ||
			 Data->SubSampleNo >= Data->InstData->NoOfSamples)
		     {
			Data->SubSample = 0;
			Data->Sample = 0xFFFF;
			Warn("Badly formed XM sub sample");
		     }
		     else
		     {
			// Index the Sub Sample and Sample.
			Data->SubSample = &Data->InstData->Samples[Data->SubSampleNo];
			Data->Sample = SampleMappings[Data->CurInstrument]
		     	                             [Data->SubSampleNo];
		     }
		  }
	       
               // Check for an instrument change
               if (Instrument != 0 && Frame == 0 && RowRepeatFlag == 0)
               {
		  if (Instrument <= Header->NoOfInstruments &&
		      SampleMappings[Instrument-1] && Note)
		  {
		     Data->CurInstrument = Instrument - 1;

		     /* Determine the sample that matches this note. We use
		        a multilayered lookup mechanism, SampleMappings 
			contains an mapping from Inst,SubSamp pairs into
			the absolute values that the mixer uses. InstD.NoteMap
			contains the XM note transform table used to select
		        the sub sample. */
		     if (Data->CurInstrument >= Header->NoOfInstruments)
		     {
			Data->InstData = 0;
			Data->SubSampleNo = 0;
		     }
		     else
		     {
			Data->InstData = InstrumentData + Data->CurInstrument;
			Data->SubSampleNo = Data->InstData->NoteMap[Data->Note-1];
		     }
		     
		     
		     // Range check the sample info
		     if (Data->InstData == 0 || 
			 Data->SubSampleNo >= Data->InstData->NoOfSamples)
		     {
			Data->SubSample = 0;
			Data->Sample = 0xFFFF;
			Warn("Badly formed XM sample");
		     }
		     else
		     {
			// Index the Sub Sample and Sample.
			Data->SubSample = &Data->InstData->Samples[Data->SubSampleNo];
			Data->Sample = SampleMappings[Data->CurInstrument]
		     	                             [Data->SubSampleNo];
		     }
		  }
		  else
		  {
		     /* Instrument does not exist, kill the channel and
		        wipe the old instrument */
		     Data->InstData = 0;
		     Data->SubSampleNo = 0;
		     Data->SubSample = 0;

		     Data->Flags &= ~CHAN_Retrig;
		     Data->Flags |= CHAN_Cut;
		  }
		  
                  Data->Flags |= CHAN_Instrument;
                  
		  if (Data->VibratoType / 4 == 0) 
                     Data->VibratoPos = 0;

                  if (Data->TremoloType / 4 == 0) 
                     Data->TremoloPos = 0;
		  
		  if (Data->SubSample != 0)
		  {
		     Data->PanPos = (Data->SubSample->Pan - 128)*PanSpan/255;
		     Data->Volume = Data->SubSample->Volume * VolMax/64;
		     Data->FineTune = Data->SubSample->FineTune;
		  }
		  
                  Data->Flags |= CHAN_Volume | CHAN_Pan;		  
               }

               if (Note != 0 && Note != 97 && Frame == 0)
               {
                  // Fix, retrig on note off. This handles portamento
                  if ((Effect == EFF_Porta || Effect == EFF_DualPorta || 
		       Volume/0x10 == VOL_Portamento) && Data->Period != 0)
                  {
                     // Retrig if no note is playing
                     if (Data->KeyState == 0)
                     {
                        Data->Flags &= ~CHAN_Cut;
                        Data->Flags |= CHAN_Retrig | CHAN_Volume | CHAN_Pan | CHAN_Pitch;
                        ChanDat[Channel].SampleOffset = 0;
                        Data->KeyState = 1;
                        Data->VolEnvelopePos = 0;
                        Data->PanEnvelopePos = 0;
                        Data->KeyState = 1;
                     }

		     // Setup
                     if (Data->Sample < NoOfSamples)
                     {
                        int NewNote, NewPeriod;
                        NewNote = Data->Note + Data->SubSample->NoteAdjust - 1;

			// Linear frequency table 
                        if (Header->Flags & 1)
                           NewPeriod = 10*12*16*4 - NewNote*16*4 - Data->FineTune/2;
                        else
                           NewPeriod = (int)((float)BASEFREQ/pow(2,(float)NewNote/12.0 + (float)Data->FineTune/(128.0 * 12.0)));
                        Data->PortaTarget = NewPeriod;
                     }
		  }
		  else
		  {
                     int NewNote, NewPeriod;
		     
                     Data->Flags &= ~CHAN_Cut;
                     Data->Flags |= CHAN_Retrig | CHAN_Volume | CHAN_Pan | CHAN_Pitch;
                     ChanDat[Channel].SampleOffset = 0;
                     Data->VolEnvelopePos = 0;
                     Data->PanEnvelopePos = 0;
                     Data->KeyState = 1;

                     if (Data->Note > 96)
                        Data->Note = 96;

                     // Untrigger the sample
                     if (Data->SubSample == 0)
                     {
                        Data->Flags &= ~CHAN_Retrig;
                        Data->Flags |= CHAN_Cut;
                     }
                     else
                     {
                        NewNote = Data->Note + Data->SubSample->NoteAdjust - 1;

                        if (Header->Flags & 1)
                           NewPeriod = 10*12*16*4 - NewNote*16*4 - Data->FineTune/2;
                        else
                           NewPeriod = (int)((float)BASEFREQ/pow(2,(float)NewNote/12.0 + (float)Data->FineTune/(128.0*12.0)));
                        Data->Period = NewPeriod;
                     }
                  }
               }
	       
               if (Data->PeriodShift != 0) 
		  Data->Flags |= CHAN_Pitch;
               Data->PeriodShift = 0;
	       
               if (Data->VolumeShift != 0)
		  Data->Flags |= CHAN_Volume;
               Data->VolumeShift = 0;
	       
               if (Data->PanShift != 0) 
		  Data->Flags |= CHAN_Pan | CHAN_Volume;
               Data->PanShift = 0;

               //Check for a volume change/volume effect
               if (Volume >= 0x10 && Volume <= 0x50 && Frame == 0) 
	       {
		  Data->Volume = (Volume - 0x10) * VolMax / 64;
		  Data->Flags |= CHAN_Volume;
               } 
	       else 
		  switch (Volume / 0x10) 
	          {
                     case VOL_None:
		     break;
		     
		     case VOL_VolumeSlideDown:
		     if (!Frame) 
			break;
		     Data->Volume -= (Volume - VOL_VolumeSlideDown*0x10)*VolMax/64;
		     if (Data->Volume < 0)
			 Data->Volume = 0;
		     Data->Flags |= CHAN_Volume;
		     break;
		     
		     case VOL_VolumeSlideUp:
		     if (!Frame) 
			break;
		     Data->Volume += (Volume - VOL_VolumeSlideUp*0x10) * VolMax / 64;
		     if (Data->Volume > VolMax) 
			Data->Volume = VolMax;
		     Data->Flags |= CHAN_Volume;
		     break;
		     
                     case VOL_FineVolumeSlideDown:
		     Data->Volume -= (Volume - VOL_FineVolumeSlideDown*0x10) * VolMax/64/FramesARow;
		     if (FramesARow > 1 && Frame == FramesARow - 1)
			Data->Volume -= (Volume - VOL_FineVolumeSlideDown*0x10) * VolMax / 64 - ((Volume - VOL_FineVolumeSlideDown*0x10) * VolMax / 64 / FramesARow) * FramesARow;
		     
		     if (Data->Volume < 0) Data->Volume = 0;
		     Data->Flags |= CHAN_Volume;
		     break;

		     case VOL_FineVolumeSlideUp:
		     Data->Volume += (Volume - VOL_FineVolumeSlideUp*0x10) * VolMax / 64 / FramesARow;
		     if (FramesARow > 1 && Frame == FramesARow - 1)
			Data->Volume += (Volume - VOL_FineVolumeSlideUp*0x10) * VolMax / 64 - ((Volume - VOL_FineVolumeSlideUp*0x10) * VolMax / 64 / FramesARow) * FramesARow;
		     if (Data->Volume > VolMax) Data->Volume = VolMax;
		     Data->Flags |= CHAN_Volume;
		     break;
		     
		     case VOL_Vibrato:
		     if (Volume - VOL_Vibrato * 0x10) 
			Data->VibratoSize = Volume - VOL_Vibrato*0x10;
		     Data->PeriodShift += VibratoTypes[Data->VibratoType%4][Data->VibratoPos%256] * (Data->VibratoSize + 1)/64;

		     if (!Frame) 
			break;
		     Data->VibratoPos += Data->VibratoSpeed * 4;
		     Data->Flags |= CHAN_Pitch;
		     break;
		     
		     case VOL_SetVibratoSpeed:
		     if (Frame) 
			break;
		     Data->VibratoSpeed = Volume - VOL_SetVibratoSpeed*0x10;
		     break;
		     
		     case VOL_SetPanning:
		     if (Frame) 
			break;
		     Data->PanPos = (2*(Volume - VOL_SetPanning*0x10) - 15)* PanSpan / 15 / 2;
		     Data->Flags |= CHAN_Pan | CHAN_Volume;
		     break;
		     
		     case VOL_PanSlideLeft:
		     if (!Frame) 
			break;
		     Data->PanPos -= (Volume - VOL_PanSlideLeft*0x10) * PanSpan / 255;
		     if (Data->PanPos < -PanMax) 
			Data->PanPos = -PanMax;
		     Data->Flags |= CHAN_Pan | CHAN_Volume;
		     break;
		     
		     case VOL_PanSlideRight:
		     if (!Frame) 
			break;
		     Data->PanPos += (Volume - VOL_PanSlideRight*0x10) * PanSpan / 255;
		     if (Data->PanPos > PanMax) 
			Data->PanPos = PanMax;
		     Data->Flags |= CHAN_Pan | CHAN_Volume;
		     break;
		     
		     case VOL_Portamento:
		     if (Frame != 0 && Data->PortaTarget) 
		     {
			if (Data->Period < Data->PortaTarget) 
			{
                           Data->Period += (Volume - VOL_Portamento*0x10)*4*0x10;
                           if (Data->Period > Data->PortaTarget) 
			      Data->Period = Data->PortaTarget;
			} 
			else 
			{
                           Data->Period -= (Volume - VOL_Portamento*0x10)*4*0x10;
                           if (Data->Period < Data->PortaTarget)
			      Data->Period = Data->PortaTarget;
			}
			Data->Flags |= CHAN_Pitch;
                   }
                   break;
               }

               //Act upon effects
               switch (Effect) 
	       {
                  case EFF_Arpeggio:
		  if (EffectData != 0) 
		  {
		     if (Data->ArpeggioCounter == 1) 
		     {
			Data->PeriodShift = (int)(Data->Period / pow (1.059463094, EffectData & 15) - Data->Period);
			Data->Flags |= CHAN_Pitch;
		     } 
		     else 
			if (Data->ArpeggioCounter == 2) 
		        {
			   Data->PeriodShift = (int)(Data->Period / pow (1.059463094, EffectData >> 4) - Data->Period);
			   Data->Flags |= CHAN_Pitch;
			}
		     
		     Data->ArpeggioCounter++;
		     if (Data->ArpeggioCounter == 3) 
			Data->ArpeggioCounter = 0;
		  }
		  break;
		  
                  case EFF_SlideUp:
		  if (Frame != 0)
		  {
		     Data->Period -= EffectData * 4;
		     
		     if (Data->Period < 57)
			Data->Period = 57;
		     Data->Flags |= CHAN_Pitch;
		  }
		  break;
		  
                  case EFF_SlideDown:
		  if (Frame != 0)
		  {
		     Data->Period += EffectData * 4;
		     
		     if (Data->Period > 0x7FFF)
			Data->Period = 0x7FFF;
		     Data->Flags |= CHAN_Pitch;
		  }
		  break;
		  
                  case EFF_Porta:
		  if (Frame != 0 && Data->PortaTarget)
		  {
		     if (Data->Period > Data->PortaTarget)
		     {
			Data->Period -= EffectData*4;
			if (Data->Period < Data->PortaTarget)
			   Data->Period = Data->PortaTarget;
		     }
		     else
		     {
			Data->Period += EffectData*4;
			if (Data->Period > Data->PortaTarget)
			   Data->Period = Data->PortaTarget;
		     }
		     Data->Flags |= CHAN_Pitch;
		  }
		  break;
		  
                  case EFF_Vibrato:
		  if (EffectData & 15) 
		     Data->VibratoSize = EffectData & 15;
		  if (EffectData >> 4) 
		     Data->VibratoSpeed = EffectData >> 4;
		  Data->PeriodShift += VibratoTypes[Data->VibratoType%4][Data->VibratoPos%256] * (Data->VibratoSize + 1) / 64;
		  Data->VibratoPos += Data->VibratoSpeed * 4;
		  Data->Flags |= CHAN_Pitch;
		  break;
		  
                  case EFF_DualPorta:
		  if (Frame != 0) 
		  {
		     if ((EffectData & 15) == 0) 
			Data->Volume += (EffectData >> 4)*VolMax/64;
		     if ((EffectData & ~15) == 0) 
			Data->Volume -= (EffectData & 15)*VolMax/64;
		     boundv(0,Data->Volume,VolMax);
		     Data->Flags |= CHAN_Volume;
		     if (Data->Period > Data->PortaTarget)
		     {
			Data->Period -= LastArgs[Channel][EFF_Porta]*4;
			if (Data->Period < Data->PortaTarget)
			   Data->Period = Data->PortaTarget;
		     }
		     else
		     {
			Data->Period += LastArgs[Channel][EFF_Porta]*4;
			if (Data->Period > Data->PortaTarget)
			   Data->Period = Data->PortaTarget;
		     }
		     Data->Flags |= CHAN_Pitch;
		  }
		  break;
		  
                  case EFF_DualVibrato:
		  if (Frame != 0) {
		     if ((EffectData & 15) == 0) 
			Data->Volume += (EffectData >> 4)*VolMax/64;
		     if ((EffectData & ~15) == 0) 
			Data->Volume -= (EffectData & 15)*VolMax/64;
		     boundv(0,Data->Volume,VolMax);
		     Data->Flags |= CHAN_Volume;
		     Data->PeriodShift += VibratoTypes[Data->VibratoType%4][Data->VibratoPos%256] * (Data->VibratoSize + 1) / 64;
		     Data->VibratoPos += Data->VibratoSpeed * 4;
		     Data->Flags |= CHAN_Pitch;
		  }
		  break;
		  
                  case EFF_Tremolo:
		  Data->TremoloSize = EffectData & 15;
		  Data->TremoloSpeed = EffectData >> 4;
		  Data->VolumeShift += VibratoTypes[Data->TremoloType%4][Data->TremoloPos%256] * (Data->TremoloSize + 1) * VolMax/64 / 64;
		  if (Frame == 0) 
		     break;
		  Data->TremoloPos += Data->TremoloSpeed * 4;
		  Data->Flags |= CHAN_Volume;
		  break;
		  
                  case EFF_Pan:
		  if (Frame == 0) 
		  {
		     Data->PanPos = (EffectData - 128) * PanSpan / 255;
		     Data->Flags |= CHAN_Pan | CHAN_Volume;
		  }
		  break;
		  
                  case EFF_SetSampleOffset:
		  if (Frame == 0) 
		     ChanDat[Channel].SampleOffset = EffectData * 0x100;
		  break;
		  
                  case EFF_VolumeSlide:
		  if (Frame != 0) 
		  {
		     if ((EffectData & 15) == 0) 
			Data->Volume += (EffectData >> 4) * VolMax/64;
		     if ((EffectData & ~15) == 0) 
			Data->Volume -= (EffectData & 15) * VolMax/64;
		     boundv(0,Data->Volume,VolMax);
		     Data->Flags |= CHAN_Volume;
		  }
		  break;
		  
                  case EFF_SetOrder:
		  if (Frame == 0)
		     NewOrder = EffectData;
		  break;
		  
                  case EFF_SetVolume:
		  if (Frame == 0)
		  {
		     Data->Volume = EffectData*VolMax/64;
		     if (Data->Volume > VolMax) 
			Data->Volume = VolMax;
		     Data->Flags |= CHAN_Volume;
		  }
		  break;
		  
                  case EFF_PatternBreak:
		  NewOrder = CurOrder + 1;
		  NewRow = (EffectData & 0xF) + 10*(EffectData >> 4);
		  break;
		  
                  case EFF_Special:
		  switch (EffectData >> 4) 
		  {
		     case EFF_FinePortaUp:
		     if (Frame == 0) 
		     {
			Data->Period -= EffectData & 15;
			Data->Flags |= CHAN_Pitch;
		     }
		     break;
		     
		     case EFF_FinePortaDown:
		     if (Frame == 0) 
		     {
			Data->Period += EffectData & 15;
			Data->Flags |= CHAN_Pitch;
		     }
		     break;

		     case EFF_SetGlissando:
		     if (Frame == 0) 
			GlissandoControl = EffectData;
		     break;
		     
		     case EFF_SetVibrato:
		     if (Frame == 0) 
			Data->VibratoType = EffectData;
		     break;
		     
		     case EFF_SetFineTune:
		     if (Frame == 0) 
		     {
			Data->FineTune = EffectData & 15;
			if (Data->FineTune > 7) 
			   Data->FineTune -= 16;
			Data->Flags |= CHAN_Pitch;
		     }
		     break;
		     
		     case EFF_Loop:
		     if (Frame == 0) 
		     {
			if ((EffectData & 15) == 0)
			{
			   LoopRow = Row;
			   break;
			}
			if (CurrentLoop == 0) 
			{
			   CurrentLoop = EffectData & 15;
			   NewRow = LoopRow;
			   break;
			}
			if (--CurrentLoop) 
			{
			   NewRow = LoopRow;
			}
		     }
		     break;
		     
		     case EFF_Tremolo:
		     if (Frame == 0) 
			Data->TremoloType = EffectData;
		     break;

		     case EFF_Retrig:
		     if (Frame % ((EffectData & 15) + 1) == 0)
		     {
			Data->Flags &= ~CHAN_Cut;
			Data->Flags |= CHAN_Retrig;
			ChanDat[Channel].SampleOffset = 0;
		     }
		     break;

		     case EFF_FineVolumeSlideUp:
		     if (Frame == 0) 
		     {
			Data->Volume += (EffectData & 15)*VolMax/64;
			if (Data->Volume > VolMax) 
			   Data->Volume = VolMax;
			Data->Flags |= CHAN_Volume;
		     }
		     break;
		     
		     case EFF_FineVolumeSlideDown:
		     if (Frame == 0) 
		     {
			Data->Volume -= (EffectData & 15) * VolMax/64;
			if (Data->Volume < 0) 
			   Data->Volume = 0;
			Data->Flags |= CHAN_Volume;
		     }
		     break;
		     
		     case EFF_NoteCut:
		     if (Frame == (EffectData & 15))
		     {
			Data->Flags &= ~CHAN_Retrig;
			Data->Flags |= CHAN_Cut;
		     }
		     break;
		     
		     case EFF_NoteDelay:
		     if (Frame < (EffectData & 15))
		     {
			Data->Flags &= ~CHAN_Cut;
			Data->Flags &= ~CHAN_Retrig;
		     }
		     
		     if (Frame == (EffectData & 15))
		     {
			Data->Flags &= ~CHAN_Cut;
			Data->Flags |= CHAN_Retrig;
		     }
		     break;

		     case EFF_PatternDelay:
		     if (Frame == 0) 
			if (PatternDelay == 0) 
			   PatternDelay = (EffectData & 15) + 1;
		     break;
		  }
		  break;
		  
                  case EFF_SetSpeed:
		  if (Frame == 0 && EffectData) 
		  {
		     if (EffectData == 0) 
			break;
		     if (EffectData < 32) 
		     {
			FramesARow = EffectData;
			FInf.Speed = FramesARow;
		     } 
		     else 
		     {
			FrameLength = Device->SecondsToLen(60.0/(24.0*((float)EffectData)));
			FInf.Tempo = EffectData;
		     }
		  }
		  break;
		  
                  case EFF_SetGlobalVolume:
		  if (Frame == 0)
		  {
		     GlobalVolume = EffectData;
		     if (GlobalVolume > 64) 
			GlobalVolume = 64;
		     FInf.GlobalVolume = GlobalVolume * VolMax / 64;
		     GlobalFlag |= 1;
		  }
		  break;
		  
                  case EFF_GlobalVolumeSlide:
		  if (Frame != 0) 
		  {
		     if ((EffectData & 15) == 0) 
			GlobalVolume += EffectData >> 4;
		     else 
			if ((EffectData & ~15) == 0) 
			   GlobalVolume -= EffectData & 15;
		     
		     boundv(0,GlobalVolume,64);
		     GlobalFlag |= 1;
		     FInf.GlobalVolume = GlobalVolume * VolMax / 64;
		  }
		  break;
		  
                  case EFF_KeyOff :
		  if (Frame == 0) 
		  {
		     Data->KeyState = 0;
		     Data->TimeOff = Data->TimeCounter;
		  }
		  break;
		  
                  case EFF_SetEnvelopePosition:
		  if (Frame == 0) 
		     Data->VolEnvelopePos = Data->PanEnvelopePos = EffectData;
		  break;
		  
                  case EFF_PanSlide:
		  if (Frame != 0) 
		  {
		     if ((EffectData & 15) == 0) 
			Data->PanPos += (EffectData >> 4) * PanSpan / 255;
		     if ((EffectData & ~15) == 0) 
			Data->PanPos -= (EffectData & 15) * PanSpan / 255;
		     
		     if (Data->PanPos < -PanMax) 
			Data->PanPos = -PanMax;
		     if (Data->PanPos > PanMax) 
			Data->PanPos = PanMax;
		     Data->Flags |= CHAN_Pan | CHAN_Volume;
		  }
		  break;
		  
                  case EFF_MultiRetrig :
		  if (Data->RetrigCounter) 
		     Data->RetrigCounter--;
		  else 
		     Data->RetrigCounter = EffectData & 0xF;
                      
		  if (Data->RetrigCounter == 0) 
		  {
		     Data->RetrigCounter = EffectData & 0xF;
		     Data->Volume *= RetrigMul[EffectData >> 4];
		     Data->Volume /= 16;
		     Data->Volume += (RetrigAdd[EffectData >> 4])*VolMax/64;
		     Data->Flags |= CHAN_Volume | CHAN_Retrig;
		     boundv(0,Data->Volume,VolMax);
		     ChanDat[Channel].SampleOffset = 0;
		  }
		  break;
		  
                  case EFF_Tremor:
		  if ((EffectData & 15) + (EffectData >> 4) == 0)
		     break;
		  
		  if (Frame%((EffectData & 15) + (EffectData >> 4)) >= (EffectData & 15)) 
		     Data->VolumeShift = -1024*65536;
		  break;
		  
                  case EFF_ExtraFine:
		  switch (EffectData >> 4) 
		  {
		     case EFF_ExtraFinePortaUp:
		     if (Frame == 0) 
			Data->Period -= EffectData & 15;
		     if (Data->Period < 57) 
			Data->Period = 57;
		     break;
		     
		     case EFF_ExtraFinePortaDown:
		     if (Frame == 0) 
			Data->Period += EffectData & 15;
		     if (Data->Period > 0x7FFF) 
			Data->Period = 0x7FFF;
		     break;
		  }
		  break;
                  }

               // Apply envelopes and other instrument properties
               if (Data->Flags & CHAN_Retrig) 
		  Data->TimeCounter = 0;
	       else 
		  Data->TimeCounter++;

               if (Data->Flags & CHAN_Cut)
		  Data->Volume = 0;

	       // Apply the evelopes
               if (Data->Volume != 0 && Data->InstData != 0) 
	       {
                  InstrumentHeader *Sample = Data->InstData;

                  //Volume envelope
                  if (Sample->VolumeFlags & 1) 
		  {
		     int y;
                     for (y = 0; y < Sample->VolumePoints; y++)
                        if (Data->VolEnvelopePos < Sample->VolumeEnvelope[y*2]) 
			   break;
		     
                     if (y < Sample->VolumePoints) 
		     {
                        int VolumeRatio;
                        VolumeRatio = Sample->VolumeEnvelope[y*2-1] * (Sample->VolumeEnvelope[y*2] - Data->VolEnvelopePos);
                        VolumeRatio += Sample->VolumeEnvelope[y*2+1] * (Data->VolEnvelopePos - Sample->VolumeEnvelope[y*2-2]);

                        if (Sample->VolumeEnvelope[y*2] != Sample->VolumeEnvelope[y*2-2])
                           VolumeRatio /= (Sample->VolumeEnvelope[y*2] - Sample->VolumeEnvelope[y*2-2]);
                        
			Data->VolumeShift += Data->Volume * VolumeRatio / 64 - Data->Volume;
                        Data->Flags |= CHAN_Volume;
		     } 
		     else 
		     {
                        Data->VolumeShift += Data->Volume * Sample->VolumeEnvelope[(Sample->VolumePoints-1)*2+1] / 64 - Data->Volume;
                        Data->Flags |= CHAN_Volume;
		     }
		     
                     if ((Sample->VolumeFlags & 2) && Data->KeyState) 
			if (Data->VolEnvelopePos < Sample->VolumeEnvelope[Sample->VolumeSustain*2]) 
			   Data->VolEnvelopePos++;

                     if (!(Sample->VolumeFlags & 2) && Data->KeyState) 
			Data->VolEnvelopePos++;
                     
		     if (!Data->KeyState) Data->VolEnvelopePos++;

                     if (Sample->VolumeFlags & 4) 
			if (Data->VolEnvelopePos == Sample->VolumeEnvelope[Sample->VolumeLoopEnd*2]) 
			   Data->VolEnvelopePos = Sample->VolumeEnvelope[Sample->VolumeLoopStart*2];
		  }
                  else
		  {
		     // No evelope, a key off, decay
                     if (!Data->KeyState)
		     {
			// Just cut it (FT 2 does, againsto.xm, last pat)
			Data->Flags &= ~CHAN_Retrig;
                        Data->Flags |= CHAN_Cut;
                     }
		  }
		  
                  // Panning envelope
                  if (Sample->PanFlags & 1) 
		  {
                     for (y = 0; y < Sample->PanPoints; y++)
                        if (Data->PanEnvelopePos < Sample->PanEnvelope[y*2]) 
			   break;
		     
                     if (y < Sample->PanPoints) 
		     {
                        int PanRatio;
                        PanRatio = Sample->PanEnvelope[y*2-1] * (Sample->PanEnvelope[y*2] - Data->PanEnvelopePos);
                        PanRatio += Sample->PanEnvelope[y*2+1] * (Data->PanEnvelopePos - Sample->PanEnvelope[y*2-2]);

                        if (Sample->PanEnvelope[y*2] != Sample->PanEnvelope[y*2-2])
                           PanRatio /= (Sample->PanEnvelope[y*2] - Sample->PanEnvelope[y*2-2]);
			
                        Data->PanShift += (PanMax-abs(Data->PanPos))*(PanRatio-32)/16; 
                        Data->Flags |= CHAN_Pan | CHAN_Volume;
		     }  
		     
		     if ((Sample->PanFlags & 2) && Data->KeyState)
			if (Data->PanEnvelopePos < Sample->PanEnvelope[Sample->PanSustain*2]) 
			   Data->PanEnvelopePos++;
		     
                     if (!(Sample->PanFlags & 2) && Data->KeyState) 
			Data->PanEnvelopePos++;
                     if (!Data->KeyState) 
			Data->PanEnvelopePos++;
                     if (Sample->PanFlags & 4) 
			if (Data->PanEnvelopePos == Sample->PanEnvelope[Sample->PanLoopEnd*2]) 
			   Data->PanEnvelopePos = Sample->PanEnvelope[Sample->PanLoopStart*2];
	          }

                  // Instrument vibrato
                  if (Sample->VibratoDepth) 
		  {
                     if (Data->TimeCounter >= Sample->VibratoSweep) 
                        Data->PeriodShift += Data->Period * VibratoTypes[Sample->VibratoType][(Data->TimeCounter * Sample->VibratoRate)%256] * (Sample->VibratoDepth + 1) / 256 / 2048;//4096;
		     else
                        Data->PeriodShift += Data->Period * VibratoTypes[Sample->VibratoType][(Data->TimeCounter * Sample->VibratoRate)%256] * (Sample->VibratoDepth + 1) * Data->TimeCounter / Sample->VibratoSweep / 256 / 2048;//4096;
                     Data->Flags |= CHAN_Pitch;
		  }

                  //Instrument fadeout
                  if (Sample->FadeOut && !Data->KeyState) 
		  {
                     int CurrentVol = 0x1000;
                     CurrentVol -= Sample->FadeOut/0x10 * (Data->TimeCounter - Data->TimeOff);
		     
                     if (CurrentVol < 0) 
			CurrentVol = 0;

                     if (CurrentVol != 0x1000) 
		     {
                        Data->VolumeShift += Data->Volume * CurrentVol / 0x1000 - Data->Volume;
                        Data->Flags |= CHAN_Volume;
		     }
		  }
	       }

	       /* We are performing a skip ahead, do not commit the data to
	          the mixer. */
               if (SkipOrds != 0 || SkipRows != 0) 
		  continue;

               // Process channel variables and apply changes
               if (Data->Flags & CHAN_Instrument) 
                  ChanDat[Channel].Sample = Data->Sample;
	       
               if (Data->Flags & CHAN_Pitch) 
	       {
                  int NotePeriod;
                  NotePeriod = (Data->Period + Data->PeriodShift);

                  if (NotePeriod > 0x7FFF * 4) 
		     NotePeriod = 0x7FFF * 4;
		  
                  if (NotePeriod < 57) 
		     NotePeriod = 57;
		  
                  if (Header->Flags & 1)
                     ChanDat[Channel].Pitch = (long)(8363*pow(2,((float) (6*12*16*4 - NotePeriod) / (float) (12*16*4))));
                  else
                     ChanDat[Channel].Pitch = 8192*1712/(NotePeriod);

                  if (GlissandoControl != 0) 
		     ChanDat[Channel].Pitch = Glissando(ChanDat[Channel].Pitch, Data->FineTune);
	       }
	       
               if ((Data->Flags & CHAN_Volume) || (GlobalFlag & 1)) 
	       {
                  int CurrentVolume, CurrentPan;
		  
                  Data->Flags |= CHAN_Volume;
		  
                  CurrentVolume = (Data->Volume + Data->VolumeShift) * GlobalVolume / 64;
                  boundv(0,CurrentVolume,VolMax);
                  CurrentPan = Data->PanPos + Data->PanShift;
                  boundv(-PanMax,CurrentPan,PanMax);
		  
                  ChanDat[Channel].MainVol = CurrentVolume;
                  ChanDat[Channel].Pan = CurrentPan;
                  ChanDat[Channel].LeftVol =  CurrentVolume * (PanMax - CurrentPan) / PanSpan;
                  ChanDat[Channel].RightVol = CurrentVolume * (PanMax + CurrentPan) / PanSpan;
	       }

               if (SkipOrds == 0 && SkipRows == 0) 
	       {
                  ChanDat[Channel].Flags = Data->Flags;
                  Data->Flags = 0;
	       }
	    }

            if (Frame == 2) 
	       GlobalFlag &= ~1;

            // Let an output filter know of the current location.
            Device->SetFrameInfo(&FInf);
	    
            FInf.SongTime += FrameLength;
            if (SkipOrds == 0 && SkipRows == 0)
            {
               long Rc = 0;
               if ((Rc = Device->Compute (&ChanDat, FrameLength)) != 0) 
	       {
                  ChanDat.free();
                  delete [] ChannelData;
                  return Rc;
	       }
            }
	 }
	 
         if (PatternDelay)
            if (--PatternDelay)
	    {
	       RowRepeatFlag = 1;
	       continue;
            }

         RowRepeatFlag = 0;
         if (NewOrder != -1) 
	    break;
	 else 
	    Row++;
      }
      
      if (NewOrder == -1) 
	 CurOrder++;
      else 
	 CurOrder = NewOrder;
   }
   
   ChanDat.free();
   delete [] ChannelData;
   return PLAYOK;
}

