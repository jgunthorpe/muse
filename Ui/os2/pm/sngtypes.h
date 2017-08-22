/* ########################################################################

   Node Type classes

   ########################################################################
*/
#ifndef SNGTYPES_H
#define SNGTYPES_H

class DisksType: public NodeType
{
   public:
   static int ID;

   virtual int Refresh(iterator Head);
   DisksType(SongList *Owner) : NodeType(Owner) {};
};

class DiskType: public NodeType
{
   public:
   static int ID;

   virtual int Refresh(iterator Head);
   DiskType(SongList *Owner) : NodeType(Owner) {Append = ':';};
};

class DirectoryType: public NodeType
{
   public:
   static int ID;

   virtual int Refresh(iterator Head);
   DirectoryType(SongList *Owner) : NodeType(Owner) {Append = '\\';};
};

#endif
