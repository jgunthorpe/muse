
#include "Unpack.h"

const unsigned char BitMasks1[7] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40 };
const unsigned char BitMasks2[8] = { 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF };

#define SYSCALL _System

extern "C"
{
unsigned long SYSCALL HuffmanUnpackASM ( unsigned char *Packed, unsigned char *SourceEnd, unsigned char *Sample, unsigned char *DestEnd, Huffman *Tree, unsigned char StartBit );
}

inline unsigned char GetBit ( unsigned char *&Source, unsigned char &Bit )
{
    if ( Bit == 7 )
    {
        Bit = 0;
        return ( *(Source++) & 0x80 );
    } else {
        return ( *Source & BitMasks1[Bit++] );
    }
}

inline unsigned char Get8Bits ( unsigned char *&Source, unsigned char &Bit )
{
    if ( Bit > 0 )
    {
        return ( ( *( (unsigned short *) (Source++)) >> Bit ) & 255 );
    } else {
        return ( *(Source++) );
    }
}

inline unsigned char GetxBits ( unsigned char *&Source, unsigned char &Bit, unsigned char NoOfBits )
{
    if ( Bit > (8-NoOfBits) )
    {
        register unsigned char OldBit = Bit;
        Bit += NoOfBits - 8;
        return ( ( *( (unsigned short *) (Source++)) >> OldBit ) & BitMasks2[NoOfBits-1] );
    } else if ( Bit < (8-NoOfBits) ) {
        register unsigned char OldBit = Bit;
        Bit += NoOfBits;
        return ( ( *Source >> OldBit ) & BitMasks2[NoOfBits-1] );
    } else {
        Bit = 0;
        return ( *(Source++) >> (8-NoOfBits) );
    }
}

unsigned long Unpack8 ( unsigned char *Packed, unsigned char *Sample, unsigned long SourceLen, unsigned long DestLen )
{
    unsigned char *SourceEnd = Packed+SourceLen;
    unsigned char *DestEnd = Sample+DestLen;
    register unsigned char Bit = 0;

    Huffman Error = { -1, -1, false, 0, 0 };
    Huffman Tree[256];
    unsigned char Layers[128];
    register unsigned char Layer, Node, FreeNode;
    unsigned char Total;
    
    //Build up Huffman tree
    Layer = 0;
    Node = 0;
    FreeNode = 1;
    
    do {
        //Find the value of the node
        Tree[Node].Value = GetxBits(Packed,Bit,7);

        //Check if there is a left branch, if so flag it unfollowed
        if ( GetBit(Packed,Bit) ) Tree[Node].Left = NODE_UNFOLLOWED;
        else Tree[Node].Left = NODE_NONE;

        //Check if there is a right branch, if so flag it unfollowed
        if ( GetBit(Packed,Bit) ) Tree[Node].Right = NODE_UNFOLLOWED;
        else Tree[Node].Right = NODE_NONE;
        
        //If there are no child nodes, flag this as a leaf
        if ( Tree[Node].Left == NODE_NONE && Tree[Node].Right == NODE_NONE ) Tree[Node].Children = false;
        else Tree[Node].Children = true;
        
    CheckNode:
        //If there is a left branch, follow it.
        if ( Tree[Node].Left == NODE_UNFOLLOWED )
        {
            Layers[Layer++] = Node;
            Tree[Node].Left = FreeNode;
            Node = FreeNode++;
            continue;
        }

        //There is no left branch, check the right branch.
        if ( Tree[Node].Right == NODE_UNFOLLOWED )
        {
            Layers[Layer++] = Node;
            Tree[Node].Right = FreeNode;
            Node = FreeNode++;
            continue;
        }

        //No branches, to follow. We must backtrack.
        //If the layer is 0, we are done.
        if ( Layer == 0 ) break;
        //We can still backtrack, go back and check if we can follow another node.
        Node = Layers[--Layer];
        goto CheckNode;
        
    } while ( true );

    //Init delta
    Total = 0;
    
    //Now go through the packed data
    return ( HuffmanUnpackASM ( Packed, SourceEnd, Sample, DestEnd, Tree, Bit ) );
/*
    while ( Packed < SourceEnd && Sample < DestEnd )
    {
        register unsigned char Sign;
        register unsigned char Node;
        
        //Get the sign
        if ( GetBit(Packed,Bit) ) Sign = 0xFF;
        else Sign = 0;

        //Get the base of the tree
        Node = 0;
        //Keep getting bits until we hit a leaf
        while ( Tree[Node].Children )
        {
            if ( Bit == 7 )
            {
                Bit = 0;
                if ( *(Packed++) & 0x80 ) Node = Tree[Node].Right;
                else Node = Tree[Node].Left;
                continue;
            } else {
                if ( *Packed & BitMasks1[Bit++] ) Node = Tree[Node].Right;
                else Node = Tree[Node].Left;
                continue;
            }

            //Error condition, break
//            if ( Node == -1 ) break;
        }

        //Add the delta and store
        Total += Tree[Node].Value ^ Sign;
        *Sample++ = Total;
    }
*/
//    if ( Sample < DestEnd && DestEnd - Sample <= 16 )
//    {
//        unsigned char Value;
//
//        Value = Sample[-1];
//        while ( Sample < DestEnd ) *(Sample++) = Value;
//        return ( false );
//    }
    
    //Check if the data is decoded successfully, return the result
//    if ( ( Packed > SourceEnd || ( Packed == SourceEnd && Bit ) ) && Sample < DestEnd ) return true;
//    else return false;
}
