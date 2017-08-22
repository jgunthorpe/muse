// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   Flags - Global Definitions of flags

   This is a global list of all imposed min/max's and delta flags used
   by the output device.
   
   Volumes are stored to 16 bit precision, panning has a range of +-16k.
   Only the digital mixer is capable of redering sound to the precisions
   described in this file, and then only 8 bit samples can be rendered
   to this accuracy.
   
   Please note that file formats are responsible for setting the pan
   value and the left/right/main channel vlaues. In some situations the
   left/right value may be ignored, in others it might not.
   
   ##################################################################### */
									/*}}}*/
#ifndef FLAGS_H
#define FLAGS_H

// Instrument Flags
#define INST_Loop (1 << 0)
#define INST_16Bit (1 << 1)
#define INST_Signed (1 << 2)
#define INST_PingPong ((1 << 3) | INST_Loop)
#define INST_Reverse (1 << 4)

/* Channel State Flags
   Setting the instrument flag will force a retigger if the instrument has
   changed, Volume and Pan are considered the same.
*/
#define CHAN_Instrument (1 << 0)
#define CHAN_Volume (1 << 1)
#define CHAN_Pan CHAN_Volume
#define CHAN_Cut (1 << 2)
#define CHAN_Retrig (1 << 3)
#define CHAN_Pitch (1 << 4)
#define CHAN_NewLoop (1 << 5)
#define CHAN_Free (1 << 31)

// Effect Filter Flags
#define EFX_Volume CHAN_Volume
#define EFX_Pitch CHAN_Pitch
#define EFX_Pan CHAN_Pan
#define EFX_Speed 0

/* - <----------------------------|-----------------------------> +
  Left                          Center                          Right
*/
#define PanMax 16384L
#define PanSpan 32768L
#define PanStep 2048L
#define PanSurround PanSpan
#define VolMax 65536L
#define MaxEfxPitch 500.0
#define MaxEfxSpeed 500.0
#define TimeNull 0xFFFFFFFF
#define LongNull 0xFFFFFFFF
#define ShortNull 0xFFFF
#define CharNull 0xFF

// musePlayerControl
#define PlayAdvance 1
#define PlayJump 2
#endif
