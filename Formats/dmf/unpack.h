
#define false 0
#define true  1

#define NODE_NONE       -1
#define NODE_UNFOLLOWED -2

typedef struct _Huffman {
    signed short Left, Right;
    signed short Children;
    unsigned char Value;
    unsigned char Discard;
} Huffman;

unsigned long Unpack8 ( unsigned char *Packed, unsigned char *Sample, unsigned long SourceLen, unsigned long DestLen );
