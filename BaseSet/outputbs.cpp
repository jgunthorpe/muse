// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   OutputBase - Base class for all Muse Output handlers

   ##################################################################### */
   									/*}}}*/
// Include Files 							/*{{{*/
#include <muse.h>
#include <efxfiltr.h>
   									/*}}}*/

// Force instantiation of these templates
template class Sequence<museSample>;
template class Sequence<museChannel>;
template class Sequence<museOutputClass *>;

museOutputClass *museOutputBase::Meta = new museOutputClass;

// OutputBase::museOutputBase - Constructor				/*{{{*/
// ---------------------------------------------------------------------
/* Just reset the output class to it's default values */
museOutputBase::museOutputBase()
{
   RealTime = false;
   MaxChannels = 0;
   Downloader = false;
}
									/*}}}*/
// OutputBase::GetMaxChannels - Returns MaxChannels			/*{{{*/
// ---------------------------------------------------------------------
/* Simply returns the maximum number of channels supported by the device. */
long museOutputBase::GetMaxChannels()
{
   return MaxChannels;
}
   									/*}}}*/
// OutputBase::SetChanVolPot - Sets the current Volume Potential	/*{{{*/
// ---------------------------------------------------------------------
/* This is needed by the efx filter. It sets the max value that could be
   generated if all channels are at their max volume and max output. DSC
   uses this value so it doesn't rescale the volume when the master volume
   is pulled down. */
void museOutputBase::SetChanVolPot(unsigned long Pot)
{
   ChanVolPot = Pot;
}
									/*}}}*/
// OutputBase::SetFrameInfo - Sets information about the current frame	/*{{{*/
// ---------------------------------------------------------------------
/* museTimeBuffer makes use of this to buffer the current frame info along
   with the delta pattern data. */
void museOutputBase::SetFrameInfo(museFrameInfo *)
{
}
									/*}}}*/
// OutputBase::SetMaxChannels - Limit the number of channels		/*{{{*/
// ---------------------------------------------------------------------
/* For modules like IT that use a dynamic number of channels this function
   is used to set an upper limit. */
unsigned long museOutputBase::SetMaxChannels(unsigned long)
{
   return MaxChannels;
}
									/*}}}*/

// OutputClass::museOutputClass - Constructor				/*{{{*/
// ---------------------------------------------------------------------
/* Set the version number */
museOutputClass::museOutputClass()
{
   Level = 0;

   MajorVersion = 1;
   MinorVersion = 3;
}
									/*}}}*/
