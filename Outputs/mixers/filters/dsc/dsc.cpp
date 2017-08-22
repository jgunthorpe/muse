/* ########################################################################

   Dynamic Scaling Control

   DSC controls the scaling factor using a number of averaging algorithms

   ########################################################################
*/
#include <Muse.h>
#include <DSC.hc>

museDSCFilter::museDSCFilter()
{
   ComputedSP = 220;
   AmpType = 4;
   AmpFactor = 90;
   AmpLowest = 0;
   SpikeCounter = 0;
   CountDown = 0;
}

void museDSCFilter::Filter(signed long *Out,signed long *OutEnd)
{
   // Dynamic Scale Control (DSC)
   if (CountDown <= OutEnd - Out)
   {
      CountDown = Owner->SecondsToLen(0.25);

      long ScalePercent = Owner->SetScale(0);

      signed long Max = *Out;
      signed long Min = *Out;
      for (;Out != OutEnd;Out++)
      {
         Max = max(Max,*Out);
         Min = min(Min,*Out);
      }
      signed long Delta = Max - Min;

      if (Max < 0) Max *= -1;
      if (Min < 0) Min *= -1;
      Max = max(Max,Min);

      long SP = ComputedSP;
      if (!(Max == 0 || Delta < 1*256*64))
      {
         SP = (((40.0*Owner->ChanVolPot)/(Max/32767.0)))/0xFF;

         if (AmpLowest == 0 || AmpLowest > SP)
            AmpLowest = SP;

         if (SP >= 110) SP -= 10;

         // Auto Spike Handling
         if (AmpType == 4)
         {
            // Wide Variance
            if (ComputedSP < SP && (ComputedSP - SP) > 500)
               SpikeCounter++;
            else
               SpikeCounter = 0;

            // Must have 0-5 spikes in a row (<1s of spike)
            if (SpikeCounter == AmpFactor/20)
               ComputedSP = SP;
         }
      }

      if (SP - (long)ComputedSP > 1000)
         SP = ComputedSP + 50;

      if (SP <= ComputedSP)
         ComputedSP = SP;
      else
         ComputedSP += (SP - ComputedSP)/10;

      if (AmpFactor <= 0)
         AmpFactor = 1;

      switch (AmpType)
      {
         // None
         case 0:
            break;

         // Slow Follow
         case 4:                  // Auto Spike
         case 1:
            if (ScalePercent > ComputedSP)
               ScalePercent = (AmpFactor*ScalePercent +
                              (100-AmpFactor)*ComputedSP)/100;
            else
               ScalePercent += (ComputedSP - ScalePercent)/AmpFactor*5;
            break;

         // Fast Follow
         case 2:
            ScalePercent = (AmpFactor*ScalePercent +
                           (100-AmpFactor)*ComputedSP)/100;
            break;

         // Downward motion only
         case 3:
            if (ScalePercent > ComputedSP)
               ScalePercent = (AmpFactor*ScalePercent +
                              (100-AmpFactor)*ComputedSP)/100;
            break;
      }

      Owner->SetScale(ScalePercent);
   }
   else
      CountDown -= OutEnd-Out;
}

void museDSCFilter::Filterm(signed long *Out,signed long *OutEnd)
{
   Filter(Out,OutEnd);
}

#ifdef CPPCOMPILE
#include <HandLst.hc>
museDSCFilterClass *museDSCFilter::__ClassObject = new museDSCFilterClass;
museDSCFilterClass::museDSCFilterClass()
{
   MajorVersion = 1;
   MinorVersion = 0;

   museHandlerList::__ClassObject->AddFilter(this);
}
#endif
