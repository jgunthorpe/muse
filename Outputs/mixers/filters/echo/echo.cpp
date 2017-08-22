/* ########################################################################

   Echos

   ######################################################################## */

#include <Muse.h>
#include <Echo.hc>

#define swap(a,b) { signed long *c; c = a; a = b; b = c; }
//#define min(a,b) (a<b?a:b)

#define SYSCALL _System

extern "C"
{
    void SYSCALL EchoMix1m ( signed long *Pos, unsigned long MixSize, signed long **PosBuf, signed long *Scale );
    void SYSCALL EchoMix2m ( signed long *Pos, unsigned long MixSize, signed long **PosBuf, signed long *Scale );
    void SYSCALL EchoMix3m ( signed long *Pos, unsigned long MixSize, signed long **PosBuf, signed long *Scale );
    void SYSCALL EchoMix4m ( signed long *Pos, unsigned long MixSize, signed long **PosBuf, signed long *Scale );
    void SYSCALL EchoMix5m ( signed long *Pos, unsigned long MixSize, signed long **PosBuf, signed long *Scale );
    void SYSCALL EchoMix6m ( signed long *Pos, unsigned long MixSize, signed long **PosBuf, signed long *Scale );
    void SYSCALL EchoMix7m ( signed long *Pos, unsigned long MixSize, signed long **PosBuf, signed long *Scale );
    void SYSCALL EchoMix8m ( signed long *Pos, unsigned long MixSize, signed long **PosBuf, signed long *Scale );
    void SYSCALL EchoMix1s ( signed long *Pos, unsigned long MixSize, signed long **PosBuf, signed long *Scale );
    void SYSCALL EchoMix2s ( signed long *Pos, unsigned long MixSize, signed long **PosBuf, signed long *Scale );
    void SYSCALL EchoMix3s ( signed long *Pos, unsigned long MixSize, signed long **PosBuf, signed long *Scale );
    void SYSCALL EchoMix4s ( signed long *Pos, unsigned long MixSize, signed long **PosBuf, signed long *Scale );
    void SYSCALL EchoMix5s ( signed long *Pos, unsigned long MixSize, signed long **PosBuf, signed long *Scale );
    void SYSCALL EchoMix6s ( signed long *Pos, unsigned long MixSize, signed long **PosBuf, signed long *Scale );
    void SYSCALL EchoMix7s ( signed long *Pos, unsigned long MixSize, signed long **PosBuf, signed long *Scale );
    void SYSCALL EchoMix8s ( signed long *Pos, unsigned long MixSize, signed long **PosBuf, signed long *Scale );

    void SYSCALL FeedbackMixM ( signed long *ToPos, signed long *FromPos, unsigned long Length, signed long Scale );
    void SYSCALL FeedbackMixS ( signed long *ToPos, signed long *FromPos, unsigned long Length, signed long Scale );
}

museEchoFilter::museEchoFilter()
{
    Buffer = 0;
    FeedbackBuffer = 0;
    BufferLength = 0;
    FeedbackLength = 0;
    NoOfEchos = 0;
    NoOfFeedbacks = 0;
    Echos = 0;

    DosCreateMutexSem(0,&Mutex,0,0);
}

museEchoFilter::~museEchoFilter()
{
    delete [] FeedbackBuffer;
    delete [] Buffer;
    delete [] Echos;
    DosCloseMutexSem(Mutex);
}

void museEchoFilter::SetupEcho ( int SetNoOfEchos, InputEcho *SetEchos, float Allow )
{
    int x, y, z;
    unsigned long MaxSize;
    unsigned long OldBufferLength = 0, OldFeedback = 0, OldAllowance;

    if ( ( SetNoOfEchos < 1 || SetNoOfEchos > 256 ) && SetNoOfEchos ) return;
//    for ( x = 0; x < SetNoOfEchos; x++ ) if ( SetEchos[x].Delay <= 0 ) return;
    
    DosRequestMutexSem(Mutex,-1);

    if ( NoOfEchos )
    {
        OldBufferLength = BufferLength;
        OldAllowance = Allowance;
    }

    if ( NoOfFeedbacks )
    {
        OldFeedback = FeedbackLength;
        OldAllowance = Allowance;
    }

    if ( NoOfEchos + NoOfFeedbacks )
    {
        delete [] Echos;
        NoOfEchos = 0;
        NoOfFeedbacks = 0;
    }

    if ( !SetNoOfEchos )
    {
        if ( OldBufferLength ) delete [] Buffer;
        if ( OldFeedback ) delete [] FeedbackBuffer;
        DosReleaseMutexSem(Mutex);
        return;
    }

    MaxSize = 0;
    Echos = new Echo[SetNoOfEchos];
    y = 0;
    for ( x = 0; x < SetNoOfEchos; x++ )
    {
        if ( SetEchos[x].Flags & ECHO_Feedback ) continue;
        if ( SetEchos[x].Delay <= 0.001 || (SetEchos[x].ScaleL == 0 && SetEchos[x].ScaleR == 0) ) continue;

        unsigned long Delay;
        Delay = Owner->SecondsToLen(SetEchos[x].Delay);

        if ( Owner->MStereo ) Delay *= 2;

        if ( Delay > MaxSize ) MaxSize = Delay;

        Echos[y].Delay = Delay;
        Echos[y].ScaleL = SetEchos[x].ScaleL * 65536;
        Echos[y].ScaleR = SetEchos[x].ScaleR * 65536;
        Echos[y].Flags = SetEchos[x].Flags;
        if ( Echos[x].Flags & ECHO_Surround ) Echos[y].ScaleR *= -1;
        y++;
    }

    NoOfEchos = y;
    BufferLength = MaxSize;

    MaxSize = 0;
    for ( x = 0; x < SetNoOfEchos; x++ )
    {
        if ( !(SetEchos[x].Flags & ECHO_Feedback) ) continue;
        if ( SetEchos[x].Delay <= 0 || (SetEchos[x].ScaleL == 0 && SetEchos[x].ScaleR == 0) ) continue;

        unsigned long Delay;
        Delay = Owner->SecondsToLen(SetEchos[x].Delay);

        if ( Owner->MStereo ) Delay *= 2;

        if ( Delay > MaxSize ) MaxSize = Delay;

        Echos[y].Delay = Delay;
        Echos[y].ScaleL = SetEchos[x].ScaleL * 65536;
        Echos[y].ScaleR = SetEchos[x].ScaleR * 65536;
        Echos[y].Flags = SetEchos[x].Flags;
        if ( Echos[x].Flags & ECHO_Surround ) Echos[y].ScaleR *= -1;
        y++;
    }

    NoOfFeedbacks = y - NoOfEchos;
    FeedbackLength = MaxSize;

    if ( NoOfEchos + NoOfFeedbacks == 0 )
    {
        delete [] Echos;
        DosReleaseMutexSem(Mutex);
        return;
    }

    Allowance = Owner->SecondsToLen(Allow);
    if ( Owner->MStereo ) Allowance *= 2;
    FeedbackLength += Allowance;
    BufferLength += Allowance;
    
    if ( OldBufferLength )
    {
        signed long *OldBuffer;

        OldBuffer = Buffer;
        Buffer = new signed long[BufferLength];
        if ( OldBufferLength >= BufferLength )
        {
            unsigned long Len;

            Len = ((OldBufferLength-BufferLength)+BufferPos)%OldBufferLength;
            memcpy ( Buffer, OldBuffer+Len, min(BufferLength,OldBufferLength-Len)*sizeof(signed long) );
            if ( OldBufferLength-Len < BufferLength )
            {
                memcpy ( Buffer+(OldBufferLength-Len), OldBuffer, (BufferLength-(OldBufferLength-Len))*sizeof(signed long) );
            }
        } else {
//            memset ( Buffer, 0, BufferLength*sizeof(signed long) );
            memset ( Buffer, 0, (BufferLength-OldBufferLength)*sizeof(signed long) );
            memcpy ( Buffer+(BufferLength-OldBufferLength), OldBuffer+BufferPos, (OldBufferLength-BufferPos)*sizeof(signed long) );
            memcpy ( Buffer+(OldBufferLength-BufferPos)+(BufferLength-OldBufferLength), OldBuffer, BufferPos*sizeof(signed long) );
        }
        delete [] OldBuffer;

    } else {
        Buffer = new signed long[BufferLength];
        memset ( Buffer, 0, BufferLength*sizeof(signed long) );
    }

    if ( OldFeedback )
    {
        signed long *OldFeedbackBuffer;

        OldFeedbackBuffer = FeedbackBuffer;
        FeedbackBuffer = new signed long[FeedbackLength];
        if ( OldFeedback >= FeedbackLength )
        {
            unsigned long Len;

            Len = ((OldFeedback-FeedbackLength)+FeedbackPos)%OldFeedback;
            memcpy ( FeedbackBuffer, OldFeedbackBuffer+Len, min(FeedbackLength,OldFeedback-Len)*sizeof(signed long) );
            if ( OldFeedback-Len < FeedbackLength )
            {
                memcpy ( FeedbackBuffer+(OldFeedback-Len), OldFeedbackBuffer, (FeedbackLength-(OldFeedback-Len))*sizeof(signed long) );
            }
        } else {
            memcpy ( FeedbackBuffer, OldFeedbackBuffer+FeedbackPos, (OldFeedback-FeedbackPos)*sizeof(signed long) );
            memcpy ( FeedbackBuffer+(OldFeedback-FeedbackPos), OldFeedbackBuffer, FeedbackPos*sizeof(signed long) );
            memset ( FeedbackBuffer+OldFeedback, 0, (FeedbackLength-OldFeedback)*sizeof(signed long) );
        }
        delete [] OldFeedbackBuffer;

    } else {
        FeedbackBuffer = new signed long[FeedbackLength];
        memset ( FeedbackBuffer, 0, FeedbackLength*sizeof(signed long) );
    }

    BufferPos = 0;
    FeedbackPos = 0;
    DosReleaseMutexSem(Mutex);
}

void museEchoFilter::Filter(signed long *Out,signed long *OutEnd)
{
    DosRequestMutexSem(Mutex,-1);
    int x;
    signed long *Pos, *PosBufL[9], *PosBufR[9], ScaleL[8], ScaleR[8];
    unsigned long Left, BufLenL[9];

    if ( NoOfEchos + NoOfFeedbacks < 1 || NoOfEchos + NoOfFeedbacks > 8 )
    {
        DosReleaseMutexSem(Mutex);
        return;
    }

    if ( NoOfEchos )
    {
        Left = OutEnd-Out;
        Pos = Out;

#define SetupEchos(x) { \
    PosBufL[x] = Buffer+(BufferPos+BufferLength-Echos[x].Delay)%BufferLength; \
    PosBufR[x] = PosBufL[x] + 1; \
    BufLenL[x] = BufferLength-(BufferPos+BufferLength-Echos[x].Delay)%BufferLength; \
    ScaleL[x] = Echos[x].ScaleL; \
    ScaleR[x] = Echos[x].ScaleR; \
    if ( (Echos[x].Flags&3) == ECHO_Flipped ) swap ( PosBufL[x], PosBufR[x] ); \
    }

        switch ( NoOfEchos )
        {
        case 8:
            SetupEchos(7);
        case 7:
            SetupEchos(6);
        case 6:
            SetupEchos(5);
        case 5:
            SetupEchos(4);
        case 4:
            SetupEchos(3);
        case 3:
            SetupEchos(2);
        case 2:
            SetupEchos(1);
        case 1:
            SetupEchos(0);
        }

//        PosBufL[8] = Buffer+(BufferPos+Allowance)%BufferLength;
//        PosBufR[8] = PosBufL[8]+1;
//        BufLenL[8] = (BufferLength-BufferPos-Allowance)%BufferLength;
        PosBufL[8] = Buffer+BufferPos;
        PosBufR[8] = PosBufL[8]+1;
        BufLenL[8] = BufferLength-BufferPos;

        while ( Left )
        {
            register signed long Cur;
            register int MixSize = Left;

            switch ( NoOfEchos )
            {
            case 8:
                if ( BufLenL[7] < MixSize ) MixSize = BufLenL[7];
            case 7:
                if ( BufLenL[6] < MixSize ) MixSize = BufLenL[6];
            case 6:
                if ( BufLenL[5] < MixSize ) MixSize = BufLenL[5];
            case 5:
                if ( BufLenL[4] < MixSize ) MixSize = BufLenL[4];
            case 4:
                if ( BufLenL[3] < MixSize ) MixSize = BufLenL[3];
            case 3:
                if ( BufLenL[2] < MixSize ) MixSize = BufLenL[2];
            case 2:
                if ( BufLenL[1] < MixSize ) MixSize = BufLenL[1];
            case 1:
            if ( BufLenL[0] < MixSize ) MixSize = BufLenL[0];
            }

            if ( BufLenL[8] < MixSize ) MixSize = BufLenL[8];
            

            switch ( NoOfEchos )
            {
            case 8:
                EchoMix8s ( Pos, MixSize, PosBufL, ScaleL );
                EchoMix8s ( Pos+1, MixSize, PosBufR, ScaleR );

                Pos += MixSize;
                break;
            case 7:
                EchoMix7s ( Pos, MixSize, PosBufL, ScaleL );
                EchoMix7s ( Pos+1, MixSize, PosBufR, ScaleR );

                Pos += MixSize;
                break;
            case 6:
                EchoMix6s ( Pos, MixSize, PosBufL, ScaleL );
                EchoMix6s ( Pos+1, MixSize, PosBufR, ScaleR );

                Pos += MixSize;
                break;
            case 5:
                EchoMix5s ( Pos, MixSize, PosBufL, ScaleL );
                EchoMix5s ( Pos+1, MixSize, PosBufR, ScaleR );

                Pos += MixSize;
                break;
            case 4:
                EchoMix4s ( Pos, MixSize, PosBufL, ScaleL );
                EchoMix4s ( Pos+1, MixSize, PosBufR, ScaleR );

                Pos += MixSize;
//            PosBuf[0] += MixSize;
/*            for ( x = 0; x < MixSize; x++ )
            {
                Cur = *Pos;
                *Pos += *PosBuf[3]++ * Scale[3] / 65536;
                *Pos += *PosBuf[2]++ * Scale[2] / 65536;
                *Pos += *PosBuf[1]++ * Scale[1] / 65536;
                *Pos++ += *PosBuf[0] * Scale[0] / 65536;
                *PosBuf[0]++ = Cur;
            }*/
                break;
            case 3:
                EchoMix3s ( Pos, MixSize, PosBufL, ScaleL );
                EchoMix3s ( Pos+1, MixSize, PosBufR, ScaleR );

                Pos += MixSize;
//            PosBuf[0] += MixSize;
/*            for ( x = 0; x < MixSize; x++ )
            {
                Cur = *Pos;
                *Pos += *PosBuf[2]++ * Scale[2] / 65536;
                *Pos += *PosBuf[1]++ * Scale[1] / 65536;
                *Pos++ += *PosBuf[0] * Scale[0] / 65536;
                *PosBuf[0]++ = Cur;
            }*/
                break;
            case 2:
                EchoMix2s ( Pos, MixSize, PosBufL, ScaleL );
                EchoMix2s ( Pos+1, MixSize, PosBufR, ScaleR );

                Pos += MixSize;
//            PosBuf[1] += MixSize;
//            PosBuf[0] += MixSize;
/*            for ( x = 0; x < MixSize; x++ )
            {
                Cur = *Pos;
                *Pos += *PosBuf[1]++ * Scale[1] / 65536;
                *Pos++ += *PosBuf[0] * Scale[0] / 65536;
                *PosBuf[0]++ = Cur;
            }*/
                break;
            case 1:
                EchoMix1s ( Pos, MixSize, PosBufL, ScaleL );
                EchoMix1s ( Pos+1, MixSize, PosBufR, ScaleR );

                Pos += MixSize;
                *PosBufL += MixSize;
                *PosBufR += MixSize;
                PosBufL[8] += MixSize;
                PosBufR[8] += MixSize;
/*            for ( x = 0; x < MixSize; x++ )
            {
                Cur = *Pos;
                *Pos++ += *PosBuf[0], Scale[0] );
                *PosBuf[0]++ = Cur;
            }*/
                break;
            }

#define RollEchos(x) { \
    BufLenL[x] -= MixSize; \
    if ( BufLenL[x] == 0 ) \
    { \
        PosBufL[x] = Buffer; \
        PosBufR[x] = Buffer+1; \
        BufLenL[x] = BufferLength; \
        if ( (Echos[x].Flags&3) == ECHO_Flipped ) swap ( PosBufL[x], PosBufR[x] ); \
    } \
    }

            switch ( NoOfEchos )
            {
            case 8:
                RollEchos(7);
            case 7:
                RollEchos(6);
            case 6:
                RollEchos(5);
            case 5:
                RollEchos(4);
            case 4:
                RollEchos(3);
            case 3:
                RollEchos(2);
            case 2:
                RollEchos(1);
            case 1:
                RollEchos(0);
            }

            BufLenL[8] -= MixSize;
            if ( BufLenL[8] == 0 )
            {
                PosBufL[8] = Buffer;
                PosBufR[8] = Buffer+1;
                BufLenL[8] = BufferLength;
            }

            Left -= MixSize;

            BufferPos += MixSize;
            if ( BufferPos > BufferLength ) BufferPos -= BufferLength;

        }

//        BufferPos = (PosBufL[8]-Buffer-Allowance)%BufferLength;
    }

    if ( NoOfFeedbacks )
    {
        Echo *Feedback = Echos+NoOfEchos;

        for ( x = 0; x < NoOfFeedbacks; x++ )
        {
            signed long *ToPos, *FromPos;
            signed long Len, From;

            //buffer continuation for both

            ToPos = OutEnd-2;
            FromPos = ToPos-Feedback->Delay;
            Len = (OutEnd-Out) - Feedback->Delay;
            if ( Len >= 0 )
            {
                if ( Feedback->Flags & ECHO_Flipped )
                {
                    FeedbackMixS ( ToPos, FromPos+1, Len, Feedback->ScaleL );
                    FeedbackMixS ( ToPos+1, FromPos, Len, Feedback->ScaleR );
                } else {
                    FeedbackMixS ( ToPos, FromPos, Len, Feedback->ScaleL );
                    FeedbackMixS ( ToPos+1, FromPos+1, Len, Feedback->ScaleR );
                }
            }

            ToPos = Out+Feedback->Delay-2;
            From = FeedbackPos+FeedbackLength;
            Len = Feedback->Delay;
            if ( Len > (OutEnd-Out) )
            {
                From -= ( Len-(OutEnd-Out) );
                Len = (OutEnd-Out);
                ToPos = OutEnd-2;
            }

            FromPos = FeedbackBuffer+From%FeedbackLength;

            FeedbackMixS ( ToPos, FromPos, min(Len,FromPos-FeedbackBuffer), Feedback->ScaleL );
            FeedbackMixS ( ToPos+1, FromPos+1, min(Len,FromPos-FeedbackBuffer), Feedback->ScaleR );
            ToPos -= min(Len,FromPos-FeedbackBuffer);
            Len -= min(Len,FromPos-FeedbackBuffer);

            if ( Len )
            {
                FeedbackMixS ( ToPos, FeedbackBuffer+FeedbackLength-2, Len, Feedback->ScaleL );
                FeedbackMixS ( ToPos+1, FeedbackBuffer+FeedbackLength-2+1, Len, Feedback->ScaleR );
            }
        }

        if ( (OutEnd-Out) > FeedbackLength )
        {
            FeedbackPos = 0;
            memcpy ( FeedbackBuffer, OutEnd-FeedbackLength, FeedbackLength*sizeof(signed long) );
        } else {
            unsigned long Len;

            Len = OutEnd-Out;

            memcpy ( FeedbackBuffer+FeedbackPos, Out, min((OutEnd-Out),FeedbackLength-FeedbackPos)*sizeof(signed long) );
            Len -= min((OutEnd-Out),FeedbackLength-FeedbackPos);
            FeedbackPos += min((OutEnd-Out),FeedbackLength-FeedbackPos);
            if ( FeedbackPos == FeedbackLength ) FeedbackPos = 0;

            if ( Len )
            {
                memcpy ( FeedbackBuffer, OutEnd-Len, Len*sizeof(signed long) );
                FeedbackPos += Len;
            }
        }
    }
    DosReleaseMutexSem(Mutex);
}

void museEchoFilter::Filterm(signed long *Out,signed long *OutEnd)
{
    DosRequestMutexSem(Mutex,-1);
    int x;
    signed long *Pos, *PosBuf[9], Scale[8];
    unsigned long Left, BufLen[9];

    if ( NoOfEchos + NoOfFeedbacks < 1 || NoOfEchos + NoOfFeedbacks > 8 )
    {
        DosReleaseMutexSem(Mutex);
        return;
    }

    if ( NoOfEchos )
    {
        Left = OutEnd-Out;
        Pos = Out;

#define SetupEchom(x) { \
    PosBuf[x] = Buffer+(BufferPos+BufferLength-Echos[x].Delay)%BufferLength; \
    BufLen[x] = BufferLength-(BufferPos+BufferLength-Echos[x].Delay)%BufferLength; \
    Scale[x] = (float) 65536 * (float) Echos[x].ScaleL; \
    }

        switch ( NoOfEchos )
        {
        case 8:
            SetupEchom(7);
        case 7:
            SetupEchom(6);
        case 6:
            SetupEchom(5);
        case 5:
            SetupEchom(4);
        case 4:
            SetupEchom(3);
        case 3:
            SetupEchom(2);
        case 2:
            SetupEchom(1);
        case 1:
            SetupEchom(0);
        }

        PosBuf[8] = Buffer+BufferPos;
        BufLen[8] = BufferLength-BufferPos;

        while ( Left )
        {
            register signed long Cur;
            register int MixSize = Left;

            switch ( NoOfEchos )
            {
            case 8:
                if ( BufLen[7] < MixSize ) MixSize = BufLen[7];
            case 7:
                if ( BufLen[6] < MixSize ) MixSize = BufLen[6];
            case 6:
                if ( BufLen[5] < MixSize ) MixSize = BufLen[5];
            case 5:
                if ( BufLen[4] < MixSize ) MixSize = BufLen[4];
            case 4:
                if ( BufLen[3] < MixSize ) MixSize = BufLen[3];
            case 3:
                if ( BufLen[2] < MixSize ) MixSize = BufLen[2];
            case 2:
                if ( BufLen[1] < MixSize ) MixSize = BufLen[1];
            case 1:
                if ( BufLen[0] < MixSize ) MixSize = BufLen[0];
            }

            switch ( NoOfEchos )
            {
            case 8:
                EchoMix8m ( Pos, MixSize, PosBuf, Scale );

                Pos += MixSize;
                break;
            case 7:
                EchoMix7m ( Pos, MixSize, PosBuf, Scale );

                Pos += MixSize;
                break;
            case 6:
                EchoMix6m ( Pos, MixSize, PosBuf, Scale );

                Pos += MixSize;
                break;
            case 5:
                EchoMix5m ( Pos, MixSize, PosBuf, Scale );

                Pos += MixSize;
                break;
            case 4:
                EchoMix4m ( Pos, MixSize, PosBuf, Scale );

                Pos += MixSize;
                break;
            case 3:
                EchoMix3m ( Pos, MixSize, PosBuf, Scale );

                Pos += MixSize;
                break;
            case 2:
                EchoMix2m ( Pos, MixSize, PosBuf, Scale );

                Pos += MixSize;
                break;
            case 1:
                EchoMix1m ( Pos, MixSize, PosBuf, Scale );

                Pos += MixSize;
                *PosBuf += MixSize;
                PosBuf[8] += MixSize;
                break;
            }

#define RollEchom(x) { \
    BufLen[x] -= MixSize; \
    if ( BufLen[x] == 0 ) \
    { \
        PosBuf[x] = Buffer; \
        BufLen[x] = BufferLength; \
    } \
    }

            switch ( NoOfEchos )
            {
            case 8:
                RollEchom(7);
            case 7:
                RollEchom(6);
            case 6:
                RollEchom(5);
            case 5:
                RollEchom(4);
            case 4:
                RollEchom(3);
            case 3:
                RollEchom(2);
            case 2:
                RollEchom(1);
            case 1:
                RollEchom(0);
            }

            BufLen[8] -= MixSize;
            if ( BufLen[8] == 0 )
            {
                PosBuf[8] = Buffer;
                BufLen[8] = BufferLength;
            }

            Left -= MixSize;
        }

        BufferPos = PosBuf[8]-Buffer;

    }

    if ( NoOfFeedbacks )
    {
        Echo *Feedback = Echos+NoOfEchos;

        for ( x = 0; x < NoOfFeedbacks; x++ )
        {
            signed long *ToPos, *FromPos;
            signed long Len, From;

            ToPos = OutEnd-1;
            FromPos = ToPos-Feedback->Delay;
            Len = (OutEnd-Out) - Feedback->Delay;
            if ( Len >= 0 )
            {
                FeedbackMixM ( ToPos, FromPos, Len, Feedback->ScaleL );
            }

            ToPos = Out+Feedback->Delay-1;
            From = FeedbackPos+FeedbackLength;
            Len = Feedback->Delay;
            if ( Len > (OutEnd-Out) )
            {
                From -= ( Len-(OutEnd-Out) );
                Len = (OutEnd-Out);
                ToPos = OutEnd-1;
            }

            FromPos = FeedbackBuffer+From%FeedbackLength;

            FeedbackMixM ( ToPos, FromPos, min(Len,FromPos-FeedbackBuffer), Feedback->ScaleL );
            ToPos -= min(Len,FromPos-FeedbackBuffer);
            Len -= min(Len,FromPos-FeedbackBuffer);

            if ( Len )
            {
                FeedbackMixM ( ToPos, FeedbackBuffer+FeedbackLength-1, Len, Feedback->ScaleL );
            }
        }

        if ( (OutEnd-Out) > FeedbackLength )
        {
            FeedbackPos = 0;
            memcpy ( FeedbackBuffer, OutEnd-FeedbackLength, FeedbackLength*sizeof(signed long) );
        } else {
            unsigned long Len;

            Len = OutEnd-Out;

            memcpy ( FeedbackBuffer+FeedbackPos, Out, min((OutEnd-Out),FeedbackLength-FeedbackPos)*sizeof(signed long) );
            Len -= min((OutEnd-Out),FeedbackLength-FeedbackPos);
            FeedbackPos += min((OutEnd-Out),FeedbackLength-FeedbackPos);
            if ( FeedbackPos == FeedbackLength ) FeedbackPos = 0;

            if ( Len )
            {
                memcpy ( FeedbackBuffer, OutEnd-Len, Len*sizeof(signed long) );
                FeedbackPos += Len;
            }
        }
    }
    DosReleaseMutexSem(Mutex);
}

#ifdef CPPCOMPILE
#include <HandLst.hc>
museEchoFilterClass *museEchoFilter::__ClassObject = new museEchoFilterClass;
museEchoFilterClass::museEchoFilterClass()
{
   MajorVersion = 1;
   MinorVersion = 0;

   museHandlerList::__ClassObject->AddFilter(this);
}
#endif
