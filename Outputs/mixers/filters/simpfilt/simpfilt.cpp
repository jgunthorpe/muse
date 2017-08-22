/* ########################################################################

   The simple output filters

   ########################################################################
*/
#include <muse.h>
#include <simpfilt.h>

/*
   Clip/Scale Filter (default)
*/
void museScaleFilter::Filter16(unsigned short *Out,unsigned short *OutEnd,signed long *In)
{
   signed long SC = Owner->ScalingConstant;

   for (;Out < OutEnd; In++, Out++)
     *Out = min(max((*In)/SC,-1*0x7FFF),0x7FFF);
}

void museScaleFilter::Filter16m(unsigned short *Out,unsigned short *OutEnd,signed long *In)
{
  Filter16(Out,OutEnd,In);
}

void museScaleFilter::Filter8(unsigned char *Out,unsigned char *OutEnd,signed long *In)
{
   signed long SC = Owner->ScalingConstant;

   for (;Out < OutEnd; In++, Out++)
     *Out = min(max((*In)/SC,-1*0x7F),0x7F) + 0x80;
}

void museScaleFilter::Filter8m(unsigned char *Out,unsigned char *OutEnd,signed long *In)
{
   Filter8(Out,OutEnd,In);
}

/*
   Noisy Clip Filter
*/
void museNoisyClipFilter::Filter16(unsigned short *Out,unsigned short *OutEnd,signed long *In)
{
   signed long SC = Owner->ScalingConstant;

   for (;Out < OutEnd; In++, Out++)
     *Out = (*In)/SC;
}

void museNoisyClipFilter::Filter16m(unsigned short *Out,unsigned short *OutEnd,signed long *In)
{
  Filter16(Out,OutEnd,In);
}

void museNoisyClipFilter::Filter8(unsigned char *Out,unsigned char *OutEnd,signed long *In)
{
   signed long SC = Owner->ScalingConstant;

   for (;Out < OutEnd; In++, Out++)
     *Out = (*In)/SC;
}

void museNoisyClipFilter::Filter8m(unsigned char *Out,unsigned char *OutEnd,signed long *In)
{
   Filter8(Out,OutEnd,In);
}

/* ########################################################################

   This is the light filter transform
      Works by averaging this byte with the last byte.

   ########################################################################
*/
void museLightFilter::Filter16(unsigned short *Out,unsigned short *OutEnd,signed long *In)
{
   signed long NewLeft;
   signed long NewRight;
   signed long Sc = Owner->ScalingConstant*2;
   for (;Out < OutEnd; In += 2, Out += 2)
   {
      NewLeft = (*In);
      NewRight = (In[1]);

      *Out = min(max((NewLeft + FilterLeft)/Sc,-1*0x7FFF),0x7FFF);
      Out[1] = min(max((NewRight + FilterRight)/Sc,-1*0x7FFF),0x7FFF);
      FilterLeft = NewLeft;
      FilterRight = NewRight;
   }
}

void museLightFilter::Filter16m(unsigned short *Out,unsigned short *OutEnd,signed long *In)
{
   signed long NewLeft;
   signed long Sc = Owner->ScalingConstant*2;
   for (;Out < OutEnd; In++, Out++)
   {
      NewLeft = (*In);

      *Out = min(max((NewLeft + FilterLeft)/Sc,-1*0x7FFF),0x7FFF);
      FilterLeft = NewLeft;
   }
}

void museLightFilter::Filter8(unsigned char *Out,unsigned char *OutEnd,signed long *In)
{
   signed long NewLeft;
   signed long NewRight;
   signed long Sc = Owner->ScalingConstant*2;
   for (;Out < OutEnd; In += 2, Out += 2)
   {
      NewLeft = (*In);
      NewRight = (In[1]);

      *Out = min(max((NewLeft + FilterLeft)/Sc,-1*0x7F),0x7F) + 0x80;
      Out[1] = min(max((NewRight + FilterRight)/Sc,-1*0x7F),0x7F) + 0x80;
      FilterLeft = NewLeft;
      FilterRight = NewRight;
   }
}

void museLightFilter::Filter8m(unsigned char *Out,unsigned char *OutEnd,signed long *In)
{
   signed long NewLeft;
   signed long Sc = Owner->ScalingConstant*2;
   for (;Out < OutEnd; In++, Out++)
   {
      NewLeft = (*In);

      *Out = min(max((NewLeft + FilterLeft)/Sc,-1*0x7F),0x7F) + 0x80;
      FilterLeft = NewLeft;
   }
}

/* ########################################################################

   This is the heavy filter transform

     4/4 N - 1/4 N + 1/4 L
      1/4 average of the last byte and the current byte.

   ########################################################################
*/
void museLight2Filter::Filter16(unsigned short *Out,unsigned short *OutEnd,signed long *In)
{
   signed long NewLeft;
   signed long NewRight;
   signed long ScalingConstant = Owner->ScalingConstant;
   for (;Out < OutEnd; In += 2, Out += 2)
   {
      NewLeft = (*In)/ScalingConstant;
      NewRight = (In[1])/ScalingConstant;

      *Out = min(max(NewLeft - (NewLeft - FilterLeft)/4,-1*0x7FFF),0x7FFF);
      Out[1] = min(max(NewRight - (NewRight - FilterRight)/4,-1*0x7FFF),0x7FFF);
      FilterLeft = NewLeft;
      FilterRight = NewRight;
   }
}

void museLight2Filter::Filter16m(unsigned short *Out,unsigned short *OutEnd,signed long *In)
{
   signed long NewLeft;
   signed long ScalingConstant = Owner->ScalingConstant;
   for (;Out < OutEnd; In++, Out++)
   {
      NewLeft = (*In)/ScalingConstant;

      *Out = min(max(NewLeft - (NewLeft - FilterLeft)/4,-1*0x7FFF),0x7FFF);
      FilterLeft = NewLeft;
   }
}

void museLight2Filter::Filter8(unsigned char *Out,unsigned char *OutEnd,signed long *In)
{
   signed long NewLeft;
   signed long NewRight;
   signed long ScalingConstant = Owner->ScalingConstant;
   for (;Out < OutEnd; In += 2, Out += 2)
   {
      NewLeft = (*In)/ScalingConstant;
      NewRight = (In[1])/ScalingConstant;

      *Out = min(max(NewLeft - (NewLeft - FilterLeft)/4,-1*0x7F),0x7F) + 0x80;
      Out[1] = min(max(NewRight - (NewRight - FilterRight)/4,-1*0x7F),0x7F) + 0x80;
      FilterLeft = NewLeft;
      FilterRight = NewRight;
   }
}

void museLight2Filter::Filter8m(unsigned char *Out,unsigned char *OutEnd,signed long *In)
{
   signed long NewLeft;
   signed long ScalingConstant = Owner->ScalingConstant;
   for (;Out < OutEnd; In++, Out++)
   {
      NewLeft = (*In)/ScalingConstant;

      *Out = min(max(NewLeft - (NewLeft - FilterLeft)/4,-1*0x7F),0x7F) + 0x80;
      FilterLeft = NewLeft;
   }
}

/* ########################################################################

   This is the heavy deep filter transform

     2/4 N + 1/4 L + 1/4 L2
      1/4 average of the last byte and the current byte.

   ########################################################################
*/
void museHeavyDeepFilter::Filter16(unsigned short *Out,unsigned short *OutEnd,signed long *In)
{
   signed long NewLeft;
   signed long NewRight;
   signed long ScalingConstant = Owner->ScalingConstant;
   for (;Out < OutEnd; In += 2, Out += 2)
   {
      NewLeft = (*In)/ScalingConstant;
      NewRight = (In[1])/ScalingConstant;

      *Out = min(max(NewLeft/2 + (PFilterLeft + FilterLeft)/4,-1*0x7FFF),0x7FFF);
      Out[1] = min(max(NewRight/2 + (PFilterRight + FilterRight)/4,-1*0x7FFF),0x7FFF);

      PFilterLeft = FilterLeft;
      PFilterRight = FilterRight;
      FilterLeft = NewLeft;
      FilterRight = NewRight;
   }
}

void museHeavyDeepFilter::Filter16m(unsigned short *Out,unsigned short *OutEnd,signed long *In)
{
   signed long NewLeft;
   signed long ScalingConstant = Owner->ScalingConstant;
   for (;Out < OutEnd; In++, Out++)
   {
      NewLeft = (*In)/ScalingConstant;

      *Out = min(max(NewLeft/2 + (PFilterLeft + FilterLeft)/4,-1*0x7FFF),0x7FFF);

      PFilterLeft = FilterLeft;
      FilterLeft = NewLeft;
   }
}

void museHeavyDeepFilter::Filter8(unsigned char *Out,unsigned char *OutEnd,signed long *In)
{
   signed long NewLeft;
   signed long NewRight;
   signed long ScalingConstant = Owner->ScalingConstant;
   for (;Out < OutEnd; In += 2, Out += 2)
   {
      NewLeft = (*In)/ScalingConstant;
      NewRight = (In[1])/ScalingConstant;

      *Out = min(max(NewLeft/2 + (PFilterLeft + FilterLeft)/4,-1*0x7F),0x7F) + 0x80;
      Out[1] = min(max(NewRight/2 + (PFilterRight + FilterRight)/4,-1*0x7F),0x7F) + 0x80;

      PFilterLeft = FilterLeft;
      PFilterRight = FilterRight;
      FilterLeft = NewLeft;
      FilterRight = NewRight;
   }
}

void museHeavyDeepFilter::Filter8m(unsigned char *Out,unsigned char *OutEnd,signed long *In)
{
   signed long NewLeft;
   signed long ScalingConstant = Owner->ScalingConstant;
   for (;Out < OutEnd; In++, Out++)
   {
      NewLeft = (*In)/ScalingConstant;

      *Out = min(max(NewLeft/2 + (PFilterLeft + FilterLeft)/4,-1*0x7F),0x7F) + 0x80;

      PFilterLeft = FilterLeft;
      FilterLeft = NewLeft;
   }
}

/* ########################################################################

   This is the heavy deep filter transform

     2/4 N + 1/4 L + 1/4 L2
      1/4 average of the last byte and the current byte.

   ########################################################################
*/
void museHeavyDeep2Filter::Filter16(unsigned short *Out,unsigned short *OutEnd,signed long *In)
{
   signed long NewLeft;
   signed long NewRight;
   signed long ScalingConstant = Owner->ScalingConstant;
   for (;Out < OutEnd; In += 2, Out += 2)
   {
      NewLeft = (*In)/ScalingConstant;
      NewRight = (In[1])/ScalingConstant;

      *Out = min(max((NewLeft + PFilterLeft + FilterLeft)/3,-1*0x7FFF),0x7FFF);
      Out[1] = min(max((NewRight + PFilterRight + FilterRight)/3,-1*0x7FFF),0x7FFF);

      PFilterLeft = FilterLeft;
      PFilterRight = FilterRight;
      FilterLeft = NewLeft;
      FilterRight = NewRight;
   }
}

void museHeavyDeep2Filter::Filter16m(unsigned short  *Out,unsigned short *OutEnd,signed long *In)
{
   signed long NewLeft;
   signed long ScalingConstant = Owner->ScalingConstant;
   for (;Out < OutEnd; In++, Out++)
   {
      NewLeft = (*In)/ScalingConstant;

      *Out = min(max((NewLeft + PFilterLeft + FilterLeft)/3,-1*0x7FFF),0x7FFF);

      PFilterLeft = FilterLeft;
      FilterLeft = NewLeft;
   }
}

void museHeavyDeep2Filter::Filter8(unsigned char *Out,unsigned char *OutEnd,signed long *In)
{
   signed long NewLeft;
   signed long NewRight;
   signed long ScalingConstant = Owner->ScalingConstant;
   for (;Out < OutEnd; In += 2, Out += 2)
   {
      NewLeft = (*In)/ScalingConstant;
      NewRight = (In[1])/ScalingConstant;

      *Out = min(max((NewLeft + PFilterLeft + FilterLeft)/3,-1*0x7F),0x7F) + 0x80;
      Out[1] = min(max((NewRight + PFilterRight + FilterRight)/3,-1*0x7F),0x7F) + 0x80;

      PFilterLeft = FilterLeft;
      PFilterRight = FilterRight;
      FilterLeft = NewLeft;
      FilterRight = NewRight;
   }
}

void museHeavyDeep2Filter::Filter8m(unsigned char *Out,unsigned char *OutEnd,signed long *In)
{
   signed long NewLeft;
   signed long ScalingConstant = Owner->ScalingConstant;
   for (;Out < OutEnd; In++, Out++)
   {
      NewLeft = (*In)/ScalingConstant;

      *Out = min(max((NewLeft + PFilterLeft + FilterLeft)/3,-1*0x7F),0x7F) + 0x80;

      PFilterLeft = FilterLeft;
      FilterLeft = NewLeft;
   }
}

/* ########################################################################

   This is the heavy deep 3 filter transform

     1/4 N + 1/4 N + 1/4 L
      1/4 average of the last byte and the current byte.

   ########################################################################
*/
void museHeavyDeep3Filter::Filter16(unsigned short *Out,unsigned short *OutEnd,signed long *In)
{
   signed long NewLeft;
   signed long NewRight;
   signed long ScalingConstant = Owner->ScalingConstant;
   for (;Out < OutEnd; In+= 2, Out += 2)
   {
      NewLeft = (*In)/ScalingConstant;
      NewRight = (In[1])/ScalingConstant;

      *Out = min(max((NewLeft + PFilterLeft + P2FilterLeft + FilterLeft)/4,-1*0x7FFF),0x7FFF);
      Out[1] = min(max((NewRight + PFilterRight + P2FilterRight + FilterRight)/4,-1*0x7FFF),0x7FFF);

      P2FilterLeft = PFilterLeft;
      P2FilterRight = PFilterRight;
      PFilterLeft = FilterLeft;
      PFilterRight = FilterRight;
      FilterLeft = NewLeft;
      FilterRight = NewRight;
   }
}

void museHeavyDeep3Filter::Filter16m(unsigned short *Out,unsigned short *OutEnd,signed long *In)
{
   signed long NewLeft;
   signed long ScalingConstant = Owner->ScalingConstant;
   for (;Out < OutEnd; In++, Out++)
   {
      NewLeft = (*In)/ScalingConstant;

      *Out = min(max((NewLeft + PFilterLeft + P2FilterLeft + FilterLeft)/4,-1*0x7FFF),0x7FFF);

      P2FilterLeft = PFilterLeft;
      PFilterLeft = FilterLeft;
      FilterLeft = NewLeft;
   }
}

void museHeavyDeep3Filter::Filter8(unsigned char *Out,unsigned char *OutEnd,signed long *In)
{
   signed long NewLeft;
   signed long NewRight;
   signed long ScalingConstant = Owner->ScalingConstant;
   for (;Out < OutEnd; In += 2, Out += 2)
   {
      NewLeft = (*In)/ScalingConstant;
      NewRight = (In[1])/ScalingConstant;

      *Out = min(max((NewLeft + PFilterLeft + P2FilterLeft + FilterLeft)/4,-1*0x7F),0x7F) + 0x80;
      Out[1] = min(max((NewRight + PFilterRight + P2FilterRight + FilterRight)/4,-1*0x7F),0x7F) + 0x80;

      P2FilterLeft = PFilterLeft;
      P2FilterRight = PFilterRight;
      PFilterLeft = FilterLeft;
      PFilterRight = FilterRight;
      FilterLeft = NewLeft;
      FilterRight = NewRight;
   }
}

void museHeavyDeep3Filter::Filter8m(unsigned char *Out,unsigned char *OutEnd,signed long *In)
{
   signed long NewLeft;
   signed long ScalingConstant = Owner->ScalingConstant;
   for (;Out < OutEnd; In++, Out++)
   {
      NewLeft = (*In)/ScalingConstant;

      *Out = min(max((NewLeft + PFilterLeft + P2FilterLeft + FilterLeft)/4,-1*0x7F),0x7F) + 0x80;

      P2FilterLeft = PFilterLeft;
      PFilterLeft = FilterLeft;
      FilterLeft = NewLeft;
   }
}

museScaleFilterClass *museScaleFilter::Meta = new museScaleFilterClass;
museScaleFilterClass::museScaleFilterClass()
{
   MajorVersion = 1;
   MinorVersion = 0;
}

museNoisyClipFilterClass *museNoisyClipFilter::Meta = new museNoisyClipFilterClass;
museNoisyClipFilterClass::museNoisyClipFilterClass()
{
   MajorVersion = 1;
   MinorVersion = 0;
}

museLightFilterClass *museLightFilter::Meta = new museLightFilterClass;
museLightFilterClass::museLightFilterClass()
{
   MajorVersion = 1;
   MinorVersion = 0;
}

museLight2FilterClass *museLight2Filter::Meta = new museLight2FilterClass;
museLight2FilterClass::museLight2FilterClass()
{
   MajorVersion = 1;
   MinorVersion = 0;
}

museHeavyDeepFilterClass *museHeavyDeepFilter::Meta = new museHeavyDeepFilterClass;
museHeavyDeepFilterClass::museHeavyDeepFilterClass()
{
   MajorVersion = 1;
   MinorVersion = 0;
}

museHeavyDeep2FilterClass *museHeavyDeep2Filter::Meta = new museHeavyDeep2FilterClass;
museHeavyDeep2FilterClass::museHeavyDeep2FilterClass()
{
   MajorVersion = 1;
   MinorVersion = 0;
}

museHeavyDeep3FilterClass *museHeavyDeep3Filter::Meta = new museHeavyDeep3FilterClass;
museHeavyDeep3FilterClass::museHeavyDeep3FilterClass()
{
   MajorVersion = 1;
   MinorVersion = 0;
}
