/* ########################################################################

   This file contains the mixer heigharchy.

   ########################################################################
*/
#include <muse.h>
#include <mix.h>

template class Sequence<museFilterBase *>;

museMixerBase::~museMixerBase()
{
}

museMixerClass *museMixerBase::Meta = new museMixerClass;
museMixerClass::museMixerClass()
{
   MajorVersion = 1;
   MinorVersion = 0;
}

museOutputFilterBase::~museOutputFilterBase()
{
}

museOFilterClass *museOutputFilterBase::Meta = new museOFilterClass;
museOFilterClass::museOFilterClass()
{
   MajorVersion = 1;
   MinorVersion = 0;
}

museFilterBase::~museFilterBase()
{
}

museFilterClass *museFilterBase::Meta = new museFilterClass;
museFilterClass::museFilterClass()
{
   MajorVersion = 1;
   MinorVersion = 0;
}

