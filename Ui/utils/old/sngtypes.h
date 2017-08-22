/* ########################################################################

   Node Type classes

   ########################################################################
*/
#ifndef SNGTYPES_H
#define SNGTYPES_H

class DisksType: public SongType
{
   public:

   virtual void Refresh(Song *Head);
   DisksType(SongTree *Owner) : SongType(Owner) {};
};

class DiskType: public SongType
{
   public:

   virtual void Refresh(Song *Head) {};
   DiskType(SongTree *Owner) : SongType(Owner) {};
};

class DirectoryType: public SongType
{
   public:

   virtual void Refresh(Song *Head);
   DirectoryType(SongTree *Owner) : SongType(Owner) {};
};

#endif
