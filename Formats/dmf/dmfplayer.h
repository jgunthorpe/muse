#ifndef GENERIC
#define GENERIC

typedef struct museSongSample
{
  octet *Sample;
  unsigned long Flags;
  unsigned long LoopBegin;
  unsigned long LoopEnd;
  unsigned long SampleEnd;
  string Name;
  unsigned long Center;
  octet Volume;
} museSongSample;
typedef Sequence<museSongSample> SequenceSongSample;

class museOutputBase;
class museDMFPlayer
{
  public :

     virtual long Play(museOutputBase* Device);

  private :

   virtual long Glissando ( long Frequency, long FineTune );

};
#endif
