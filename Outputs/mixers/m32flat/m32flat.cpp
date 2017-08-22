/* ########################################################################

   This file contains the base 32 flat mixer

   ########################################################################
*/
#include <muse.h>
#include <m32flat.h>

template class Sequence<museMixer32Flat::MixChannel>;

museMixer32Flat::museMixer32Flat()
{
   MixBuffer = 0;
   MixBSize = 0;

   Chns32.construct();

   Reset();
}

museMixer32Flat::~museMixer32Flat()
{
   Chns32.free();
   delete [] MixBuffer;
};

void museMixer32Flat::Channels(int I)
{
   Chns32.free();
   Chns32[I-1].LastMain = 0;
}

void museMixer32Flat::Reset()
{
   LastMain = 0;
   LastRight = 0;
   LastLeft = 0;
   Chns32.free();
}

signed long museMixer32Flat::ComputeSConst(unsigned long Percent,unsigned long Chans)
{
   if (Percent == 0)
      Percent = 220;

   int ScalingConstant = 0;
   if (Owner->MStereo == true)
   {
      if (Owner->MBits == 16)
         ScalingConstant = (50*Chans*VolMax)/(Percent*0xFF);
      if (Owner->MBits == 8)
         ScalingConstant = (50*Chans*VolMax)/Percent;

      if (ScalingConstant == 0)
         ScalingConstant = 1;
   }
   else
   {
      if (Owner->MBits == 16)
         ScalingConstant = (100*Chans*VolMax)/Percent;
      if (Owner->MBits == 8)
         ScalingConstant = (100*0xFF*Chans*VolMax)/Percent;

   }
   if (ScalingConstant == 0)
      ScalingConstant = 1;

   return ScalingConstant;
}

void museMixer32Flat::StopChan(int Chan)
{
   if (Owner->Chns[Chan].Playing == 0)
      return;

   LastMain += Chns32[Chan].LastMain;
   LastRight += Chns32[Chan].LastRight;
   LastLeft += Chns32[Chan].LastLeft;
/*   Chns32[Chan].LastLeft = 0;
   Chns32[Chan].LastRight = 0;
   Chns32[Chan].LastMain = 0;*/
}

void museMixer32Flat::StartChan(int Chan)
{
   if (Owner->Chns[Chan].Playing == 1)
      return;

   LastMain -= Chns32[Chan].LastMain;
   LastRight -= Chns32[Chan].LastRight;
   LastLeft -= Chns32[Chan].LastLeft;
}

long museMixer32Flat::DoMix(unsigned long Len)
{
   unsigned int BLen;

   if (Owner->MStereo == 1)
      BLen = Len*2;
   else
      BLen = Len;

   // Allocate some more ram
   if (MixBSize < BLen)
   {
      MixBSize = BLen;
      delete [] MixBuffer;
      MixBuffer = new signed long[BLen*sizeof(long)+1024];
      if (MixBuffer == 0)
      {
         MixBSize = 0;
         return 2;
      }
   }

   // Do the mixing
   signed long *MStart = MixBuffer;
   signed long *MEnd = MixBuffer + BLen;
   signed long *MCur = MStart;

   if (Owner->MStereo == true)
   {
      FillDeadSt(MStart,MEnd);
      MixStereoInto(MStart,MEnd,BLen);
      for (museFilterBase **F = Owner->FilterStack.begin(); F != Owner->FilterStack.end(); F++)
         (*F)->Filter(MStart,MEnd);
   }
   else
   {
      FillDeadMono(MStart,MEnd);
      MixMonoInto(MStart,MEnd,BLen);

      for (museFilterBase **F = Owner->FilterStack.begin(); F != Owner->FilterStack.end(); F++)
         (*F)->Filterm(MStart,MEnd);
   }

/*   LastMain /= 2;
   LastRight /= 2;
   LastLeft /= 2;*/

   // Perform down sampling and scaling
   unsigned char *&OutBegin = Owner->OutBegin;
   unsigned char *&OutEnd = Owner->OutEnd;
   if (Owner->MBits == 8)
   {
      // Output scaling
      while (MCur < MEnd)
      {
         unsigned char *End = min(OutEnd,OutBegin + (MEnd - MCur));

         if (Owner->MStereo == true)
            Owner->OutFilter->Filter8(OutBegin,End,MCur);
         else
            Owner->OutFilter->Filter8m(OutBegin,End,MCur);
         MCur += End - OutBegin;
         OutBegin = End;

         if (OutBegin == OutEnd)
         {
            long Rc = 0;
            if ((Rc = Owner->GetNextBuffer(&OutBegin,&OutEnd)) != 0)
               return Rc;
         }
      }
   }

   // 16 bit output
   if (Owner->MBits == 16)
   {
      while (MCur < MEnd)
      {
         unsigned short *OBegin = (unsigned short *)OutBegin;
         unsigned short *End = min((unsigned short *)OutEnd,OBegin + (MEnd - MCur));

         if (Owner->MStereo == true)
            Owner->OutFilter->Filter16(OBegin,End,MCur);
         else
            Owner->OutFilter->Filter16m(OBegin,End,MCur);
         MCur += End - OBegin;

         OutBegin = (octet *)End;

         long Rc = 0;
         if (OutBegin >= OutEnd)
            if ((Rc = Owner->GetNextBuffer(&OutBegin,&OutEnd)) != 0)
               return Rc;
      }
   }

   return 0;
}

void museMixer32Flat::DumpBuffer()
{
   unsigned long Len = Owner->OutEnd - Owner->OutBegin;

   // Allocate some more ram
   if (MixBSize < Len)
   {
      MixBSize = Len;
      delete [] MixBuffer;
      MixBuffer = new (signed long)[Len*sizeof(long)+1024];
      if (MixBuffer == 0)
      {
         MixBSize = 0;
         return;
      }
   }

   if (Owner->MStereo == true)
   {
      FillDeadSt(MixBuffer,MixBuffer+Len);

      if (Owner->MBits == 16)
         Owner->OutFilter->Filter16((unsigned short *)Owner->OutBegin,(unsigned short *)Owner->OutEnd,MixBuffer);
      else
         Owner->OutFilter->Filter8(Owner->OutBegin,Owner->OutEnd,MixBuffer);
   }
   else
   {
      FillDeadMono(MixBuffer,MixBuffer+Len);

      if (Owner->MBits == 16)
         Owner->OutFilter->Filter16m((unsigned short *)Owner->OutBegin,(unsigned short *)Owner->OutEnd,MixBuffer);
      else
         Owner->OutFilter->Filter8m(Owner->OutBegin,Owner->OutEnd,MixBuffer);
   }
}

void museMixer32Flat::FillDeadSt(signed long *MStart, signed long *MEnd)
{
   signed long *MCur = MStart;

   if (LastLeft == 0 && LastRight == 0)
   {
      for (MCur = MStart;MCur != MEnd; MCur++)
         *MCur = 0;
   }
   else
   {
      // Fill the Mix buffer with the inactive channels
      signed long AddL = LastLeft;
      signed long AddR = LastRight;

      for (MCur = MStart;MCur != MEnd; MCur += 2)
      {
         *MCur = AddL;
         MCur[1] = AddR;
      }
   }
}

void museMixer32Flat::FillDeadMono(signed long *MStart, signed long *MEnd)
{
   signed long *MCur = MStart;

   if (LastMain == 0)
   {
      for (MCur = MStart;MCur != MEnd; MCur++)
         *MCur = 0;
   }
   else
   {
      // Fill the Mix buffer with the inactive channels
      signed long Add = LastMain;

      for (MCur = MStart;MCur != MEnd; MCur++)
         *MCur = Add;
   }
}
