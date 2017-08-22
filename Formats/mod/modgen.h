// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   MODGeneral - This is a generic class to handle all ProTracker formats.
   
   This basically encapsulates the player portion. The conversion of 
   pattern data is done by the derived class. 
   
   ##################################################################### */
									/*}}}*/
#ifndef MODGEN_H
#define MODGEN_H

#ifndef FORMATBS_H
#include <formatbs.h>
#endif

struct museMODElement
{
   octet Sample;
   unsigned long Period;
   octet Effect;
   octet EffectParam;
   octet Volume;
};

#define SequenceMODElement Sequence<museMODElement>

class museMODGeneric : public museFormatBase
{
   public:

   virtual long GetRowElements(SequenceMODElement *Elements,
			       unsigned long Row,unsigned long Pattern) = 0;
   virtual void GetOrderList(unsigned char **List,unsigned long *Count) = 0;
   virtual void GetMODPan(unsigned char *Pan) = 0;
   virtual octet GetInitValues(octet *Speed,octet *Tempo,octet *GlobalVol) = 0;
   virtual unsigned short GetRowsAPattern() {return 64;};

   virtual long Play(museOutputBase *Device,musePlayerControl *Control);
};

#endif
