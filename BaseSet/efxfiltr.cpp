// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   EffectFilter - Effect patterndata filter

   ##################################################################### */
									/*}}}*/
// Include Files							/*{{{*/
#include <muse.h>
#include <efxfiltr.h>
   									/*}}}*/

// Force template instantiation
template class Sequence<museChanEfx>;

// museOutputLink - MPD Interception					/*{{{*/
// ---------------------------------------------------------------------
/* This class redirects all member calls to the next class down the link
   chain. By deriving from this class and then overriding compute for 
   instance you can sniff/alter the pattern data. */
museOutputLink::museOutputLink()
{
   NextLink = 0;
}
unsigned long museOutputLink::SecondsToLen(float Seconds)
{
   if (NextLink == 0)
      return 0;
   return NextLink->SecondsToLen(Seconds);
}
void museOutputLink::SetChannelMask(Sequencebool *Mask)
{
   if (NextLink == 0)
      return;
   NextLink->SetChannelMask(Mask);
}
void museOutputLink::SetVolumeScale(unsigned short Factor)
{
   if (NextLink == 0)
      return;
   NextLink->SetVolumeScale(Factor);
}
void museOutputLink::LoadSamples(SequenceSample *Samples)
{
   if (NextLink == 0)
      return;
   NextLink->LoadSamples(Samples);
}
long museOutputLink::Compute(SequenceChannel *Channels, unsigned long Len)
{
   if (NextLink == 0)
      return 1;
   return NextLink->Compute(Channels,Len);
}
void museOutputLink::StopNotes()
{
   if (NextLink == 0)
      return;
   NextLink->StopNotes();
}
long museOutputLink::InitPlay(char **Error)
{
   if (NextLink == 0)
   {
      *Error = "No Linkage";
      return 1;
   }
   return NextLink->InitPlay(Error);
}
long museOutputLink::StopPlay()
{
   if (NextLink == 0)
      return 1;
   return NextLink->StopPlay();
}
void museOutputLink::StopWhenDone()
{
   if (NextLink == 0)
      return;
   NextLink->StopWhenDone();
}
void museOutputLink::PausePlay()
{
   if (NextLink == 0)
      return;
   NextLink->PausePlay();
}
void museOutputLink::ResumePlay()
{
   if (NextLink == 0)
      return;
   NextLink->ResumePlay();
}
void museOutputLink::ForceCompError()
{
   if (NextLink == 0)
      return;
   NextLink->ForceCompError();
}
unsigned long museOutputLink::SetMaxChannels(unsigned long Count)
{
   if (NextLink == 0)
      return 1;
   return NextLink->SetMaxChannels(Count);
}
char *museOutputLink::GetCurOptionDesc()
{
   if (NextLink == 0)
      return "UnLinked";
   return NextLink->GetCurOptionDesc();
}
long museOutputLink::GetMaxChannels()
{
   if (NextLink == 0)
      return 0;
   return NextLink->GetMaxChannels();
}
void museOutputLink::SetChanVolPot(unsigned long Pot)
{
   if (NextLink == 0)
      return;
   NextLink->SetChanVolPot(Pot);
}
void museOutputLink::SetFrameInfo(museFrameInfo *Frame)
{
   if (NextLink == 0)
      return;
   NextLink->SetFrameInfo(Frame);
}
unsigned long museOutputLink::Sync(unsigned long Time)
{
   if (NextLink == 0)
      return 0;
   return NextLink->Sync(Time);
}
float museOutputLink::LenToSeconds(unsigned long Len)
{
   if (NextLink == 0)
      return 0;
   return NextLink->LenToSeconds(Len);
}
   									/*}}}*/

// PrintFilter::musePrintFilter - Constructor				/*{{{*/
// ---------------------------------------------------------------------
/* Init the members, -1 SoloChan is show all chans */
musePrintFilter::musePrintFilter()
{
   SoloChan = -1;
}
									/*}}}*/
// PrintFilter::Compute - Prints the Pattern Data			/*{{{*/
// ---------------------------------------------------------------------
/* This simply dumps the raw pattern data with printf. It is very
   usefull for debugging fileformats. */
long musePrintFilter::Compute(SequenceChannel *Channels, unsigned long Len)
{
   unsigned char Active[256];
   memset(Active,0,sizeof(Active));

   long Rc = museOutputLink::Compute(Channels,Len);

   SequenceChannel::iterator InCur = Channels->begin();
   unsigned char MaxNotes = 0;
   char Draw = 0;
   for (;InCur != Channels->end(); InCur++)
   {
      if ((InCur->Flags & (~CHAN_Free)) == 0)
         continue;
      Active[InCur->ModuleChannel]++;
      MaxNotes = max(Active[InCur->ModuleChannel],MaxNotes);
      Draw = 1;
   }
   
   char S[20];
   sprintf(S,"%4lx: ",Len);
   printf("%s",S);

   if (Draw == 0)
   {
      printf("\n");
      return Rc;
   }
   
   int SLen = strlen(S) - 4;
   memset(S,' ',strlen(S));
   for (int INote = 0; INote != MaxNotes; INote++)
   {
      if (INote != 0)
      {
         sprintf(S+SLen,"%4x> ",INote);
         printf("%s",S);
      }

      int Dead = 0;
      memset(Active,0,sizeof(Active));
      for (InCur = Channels->begin(); InCur != Channels->end(); InCur++)
      {
	 if (InCur - Channels->begin() != SoloChan && SoloChan != -1)
	    continue; 
	 
         if ((InCur->Flags & (~CHAN_Free)) == 0)
         {
            Dead++;
            continue;
         }

         Active[InCur->ModuleChannel]++;
         if (Active[InCur->ModuleChannel] - 1 != INote)
            continue;

         for (;Dead != 0;Dead--)
            printf("                ");

         char Flags[5];
         memset(Flags,'-',sizeof(Flags));
         Flags[sizeof(Flags) - 1] = 0;

         if ((InCur->Flags & CHAN_Instrument) != 0)
            Flags[0] = 'I';

         if ((InCur->Flags & CHAN_Cut) != 0)
            Flags[1] = 'C';

         if ((InCur->Flags & CHAN_Retrig) != 0)
            Flags[1] = 'R';

         if ((InCur->Flags & CHAN_Free) != 0)
            if (Flags[1] == ' ')
               Flags[1] = '!';

         if ((InCur->Flags & CHAN_Pitch) != 0)
            Flags[2] = 'P';

         if ((InCur->Flags & CHAN_Volume) != 0)
            Flags[3] = 'V';

         if (InCur->Pan == PanSurround)
            printf("%s%2lx %5lu %2lxS ",Flags,InCur->Sample,InCur->Pitch,(InCur->MainVol*0xFF)/VolMax);
         else
            printf("%s%2lx %5lu %2lx%lx ",Flags,InCur->Sample,InCur->Pitch,(InCur->MainVol*0xFF)/VolMax,((InCur->Pan+PanMax)*0xF)/PanSpan);
      }
      printf("\n");
   }

   return Rc;
};
									/*}}}*/

// EffectFilter::museEffectFilter - Constructor				/*{{{*/
// ---------------------------------------------------------------------
/* Sets the default values for all the elements and readies the class for
   playback. */
museEffectFilter::museEffectFilter()
{
   Normal.Volume = 100;
   Normal.Pitch = 100;
   Normal.PanBalance = 0;
   Normal.PanDepth = PanMax;
   Normal.PanCenter = 0;
   Normal.Disabled = false;

   Reset();
   Flags = 0;

   ChanEfx.construct();
   ChanState.construct();
}
									/*}}}*/
// EffectFilter::Reset - Performs a complete reset of the filter	/*{{{*/
// ---------------------------------------------------------------------
/* Returns everything to it's default values */
void museEffectFilter::Reset()
{
   Volume = 100;
   Pitch = 100;
   Speed = 100;
   ISpeed = 100;
   PanBalance = 0;
   PanDepth = PanMax;
   PanCenter = 0;
   Current = Normal;

   Flags |= EFX_Pitch | EFX_Pan | EFX_Volume;
}
									/*}}}*/
// EffectFilter::~museEffectFilter - Destructor          		/*{{{*/
// ---------------------------------------------------------------------
/* Frees the sequences */
museEffectFilter::~museEffectFilter()
{
   ChanEfx.free();
   ChanState.free();
}
									/*}}}*/
// EffectFilter::SetChanged - Commits the changes made 			/*{{{*/
// ---------------------------------------------------------------------
/* This commits the changes made to the classes variables. It range
   checks them and forces compute to tell the output device that
   those elements have changed */
void museEffectFilter::SetChanged(unsigned long Flags)
{
   Mutex.Lock();
   
   this->Flags |= Flags;

   Current.Volume = boundv(((float)0),Volume,((float)100.0));
   Current.Pitch = boundv(((float)0),Pitch,((float)MaxEfxPitch));
   ISpeed = boundv(((float)0),Speed,((float)MaxEfxSpeed));
   Current.PanBalance = boundv(-1*PanMax,PanBalance,PanMax);
   Current.PanDepth = boundv(-1*PanSpan,PanDepth,PanSpan);
   Current.PanCenter = boundv(-1*PanMax,PanCenter,PanMax);

   Mutex.UnLock();
}
									/*}}}*/
// EffectFilter::SetChanEfx - Sets the channel effect map		/*{{{*/
// ---------------------------------------------------------------------
/* Like the global settings each channel has it's own.  */
void museEffectFilter::SetChanEfx(SequenceChanEfx *Efx)
{
   Mutex.Lock();
   
   Flags |= EFX_Pitch | EFX_Pan | EFX_Volume;
   for (SequenceChanEfx::iterator I = Efx->begin(); I != Efx->end(); I++)
   {
      boundv(((float)0),I->Volume,((float)100.0));
      boundv(((float)0),I->Pitch,((float)MaxEfxPitch));
      boundv(-1*PanMax,I->PanBalance,PanMax);
      boundv(-1*PanSpan,I->PanDepth,PanSpan);
      boundv(-1*PanMax,I->PanCenter,PanMax);
   }

   ChanEfx.Duplicate(*Efx);

   Mutex.UnLock();
}
									/*}}}*/
// EffectFilter::InitChanEfx - Fills the passed ChanEFX struct normal	/*{{{*/
// ---------------------------------------------------------------------
/* This is used to fill the channel EFX structure with normal '0' values.
   If the resulting structure is passed back to SetChanEfx it will
   have no effect on the output. */
void museEffectFilter::InitChanEfx(SequenceChanEfx *Efx)
{
   for (museChanEfx *I = Efx->begin(); I != Efx->end(); I++)
      *I = Normal;
}
									/*}}}*/
// EffectFilter::Compute - Apply the Changes to the pattern data	/*{{{*/
// ---------------------------------------------------------------------
/* This does all the work. */
long museEffectFilter::Compute(SequenceChannel *Channels, unsigned long Len)
{
   Mutex.Lock();

   // Replicate locally
   float Volume = Current.Volume;
   float Pitch = Current.Pitch;
   float Speed = ISpeed;
   long PanBalance = Current.PanBalance;
   long PanDepth = Current.PanDepth;
   long PanCenter = Current.PanCenter;
   unsigned long Flags = this->Flags;
   this->Flags = 0;

   Mutex.UnLock();

   // Sequence guarentees 0ing of data
   ChanState[Channels->size()].Flags = 0;
   SequenceChannel::iterator InCur = Channels->begin();
   SequenceChannel::iterator ChCur = ChanState.begin();
   SequenceChanEfx::iterator EfxCur = ChanEfx.begin();

   for (;InCur != Channels->end(); InCur++, ChCur++)
   {
      // Use the default settings
      if (InCur->ModuleChannel >= ChanEfx.size())
         EfxCur = &Normal;
      else
      {
	 if (ChanEfx[InCur->ModuleChannel].Disabled == 2)
	    EfxCur = &Normal;
	 else
	    EfxCur = ChanEfx.begin() + InCur->ModuleChannel;
      }
      
      ChCur->ModuleChannel = InCur->ModuleChannel;

      // Generate a flag mask
      ChCur->Flags = InCur->Flags | Flags;
      if (ChCur->Flags == 0)
         continue;

      if ((ChCur->Flags & CHAN_Instrument) != 0)
         ChCur->Sample = InCur->Sample;

      if ((ChCur->Flags & CHAN_Retrig) != 0)
         ChCur->SampleOffset = InCur->SampleOffset;

      if ((ChCur->Flags & CHAN_Pitch) != 0)
         ChCur->Pitch = (long)((InCur->Pitch*(Pitch*EfxCur->Pitch))/(100.0*100.0));

      if ((ChCur->Flags & CHAN_Volume) != 0)
      {
         if (InCur->MainVol == 0 || EfxCur->Disabled == 1)
         {
            ChCur->MainVol = 0;
            ChCur->LeftVol = 0;
            ChCur->RightVol = 0;
            continue;
         }

         // Compute pan location
/*       Note, Overflows with far right pan point
         long Balance;
         if (InCur->LeftVol > InCur->RightVol)
            Balance = PanSpan - (InCur->LeftVol*PanSpan)/InCur->MainVol;
         else
            Balance = (InCur->RightVol*PanSpan)/InCur->MainVol;
         Balance -= PanMax;
*/
         long Balance = InCur->Pan;
         ChCur->Pan = InCur->Pan;

         // Surround mode
         if (InCur->LeftVol < 0 ||  InCur->RightVol < 0 || ChCur->Pan == PanSurround)
         {
            if (PanDepth == PanMax && PanCenter == 0 && PanBalance == 0)
               if (EfxCur->PanDepth == PanMax && EfxCur->PanCenter == 0 && EfxCur->PanBalance == 0)
               {
                  ChCur->MainVol = (long)((InCur->MainVol*Volume*EfxCur->Volume)/(100.0*100.0));
                  ChCur->LeftVol = (long)((InCur->LeftVol*Volume*EfxCur->Volume)/(100.0*100.0));
                  ChCur->RightVol = (long)((InCur->RightVol*Volume*EfxCur->Volume)/(100.0*100.0));
                  continue;
               }
            Balance = 0;
         }

         // Apply pan separation (depth)
         Balance = (Balance*((PanDepth*EfxCur->PanDepth)/PanMax))/PanMax;
         Balance = min(PanMax,max(-1*PanMax,Balance));

         // Apply Pan Center
         if (EfxCur->PanCenter > 0)
            Balance = EfxCur->PanCenter + (Balance*(PanMax - EfxCur->PanCenter))/PanMax;
         else
            Balance = EfxCur->PanCenter + (Balance*(PanMax + EfxCur->PanCenter))/PanMax;

         if (PanCenter > 0)
            Balance = PanCenter + (Balance*(PanMax - PanCenter))/PanMax;
         else
            Balance = PanCenter + (Balance*(PanMax + PanCenter))/PanMax;

         // Compute main volume
         float VolMain = (InCur->MainVol*Volume*EfxCur->Volume)/100.0/100.0;
         float VolLeft;
         float VolRight;

         /* Apply other Pan Center transform, make the volume fade down from
            100% to 50% as the pan progresses */
         if (EfxCur->PanCenter > 0)
            VolMain = (VolMain*(PanSpan - EfxCur->PanCenter))/PanSpan;
         else
            VolMain = (VolMain*(PanSpan + EfxCur->PanCenter))/PanSpan;

         if (PanCenter > 0)
            VolMain = (VolMain*(PanSpan - PanCenter))/PanSpan;
         else
            VolMain = (VolMain*(PanSpan + PanCenter))/PanSpan;

         // Apply balance tranforms
         Balance += PanMax;
         if (PanBalance > 0)
         {
            VolLeft = ((VolMain*(PanSpan - Balance))/PanSpan*(PanMax-PanBalance))/PanMax;
            VolRight = (VolMain*Balance)/PanSpan;
         }
         else
         {
            VolRight = ((VolMain*Balance)/PanSpan*(PanMax+PanBalance))/PanMax;
            VolLeft = VolMain*(PanSpan - Balance)/PanSpan;
         }

         // Compute volume
         ChCur->MainVol = (long)(VolLeft + VolRight);
         ChCur->LeftVol = (long)VolLeft;
         ChCur->RightVol = (long)VolRight;
         while (1)
         {
            // Both are 0, leave it be.
            if (VolRight == 0 && VolLeft == 0)
               break;

      /*      // See if the current value is within tolerances
            int V = (VolLeft + VolRight)*InCur->Balance/0xF - VolLeft;
            if (V < 2 && V > -2)
               break;

            V = (VolLeft + VolRight)*(0xF - InCur->Balance)/0xF - VolRight;
            if (V < 2 && V > -2)
               break;
      */
            // Compute it
            if (VolRight == 0)
            {
               ChCur->Pan = -1*PanMax;
               break;
            }

            if (VolLeft == 0)
            {
               ChCur->Pan = PanMax;
               break;
            }

            ChCur->Pan = (long)(PanSpan/(VolLeft/VolRight + 1) - PanMax);
            break;
         }
      }
   }

   // Dup dead channel info
   long Rc = 0;
   do
   {
      if (Speed == 0)
      {
         Rc = museOutputLink::Compute(&ChanState,0);

         // Reload the local speed
	 Mutex.Lock();
         boundv(((float)0),this->Speed,((float)MaxEfxSpeed));
         Speed = this->Speed;
	 Mutex.UnLock();
      }
      else
         Rc = museOutputLink::Compute(&ChanState,(long)((Len*100)/Speed));
   }
   while ((Rc == 0) && (Speed == 0));

   InCur = Channels->begin();
   ChCur = ChanState.begin();
   for (;InCur != Channels->end(); InCur++, ChCur++)
      InCur->Flags = ChCur->Flags;
   return Rc;
}
									/*}}}*/
