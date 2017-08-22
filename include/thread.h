// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   Thread - An abstracted thread framework.

   This module provides the basic building blocks required for threads 
   to exist. Mutex and Event semaphors are provided as classes. Other
   thread functions are stand alone funcs. 
 
   A mutex (mutul exclusion) semaphore is a simple blocking device. A
   thread may aquire the mutex preventing any other thread from getting
   it. It may lock it any number of times, only when the lock count is
   0 can other threads lock it.
   
   An event semaphore is used to wait for an event to occure. The thread
   will be unblocked and wait() will return the number of times the
   event has been triggered since the last time wait() returned.
   
   All the classes will free the semaphore when they go out of scope.
   
   This source is placed in the Public Domain, do with it what you will
   It was originally written by Jason Gunthorpe <jgg@gpu.srv.ualberta.ca>   
   
   ##################################################################### */
									/*}}}*/
#ifndef THREAD_H
#define THREAD_H

class threadMutex;
class threadEvent;

class threadMutex
{
   public:

   void Lock();
   void UnLock();
   
   threadMutex();
   ~threadMutex();
};

class threadEvent
{
   public:
   
   long Wait();
   void Trigger();

   threadEvent();
   ~threadEvent();
};

void threadSleep(long Time);

#endif
