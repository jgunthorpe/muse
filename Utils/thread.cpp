// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   Thread - Abstracted thread framework.
   
   This version provides an empty thread framework. Usefull if you do
   not intend to use threads on a specific platform.
   
   This source is placed in the Public Domain, do with it what you will
   It was originally written by Jason Gunthorpe <jgg@gpu.srv.ualberta.ca>   
   
   ##################################################################### */
   									/*}}}*/
#include <thread.h>

// Mutex::threadMutex() - Constructor					/*{{{*/
// ---------------------------------------------------------------------
/* Constructs the mutex semaphore */
threadMutex::threadMutex()
{
}
									/*}}}*/
// Mutex::~threadMutex - Destructor					/*{{{*/
// ---------------------------------------------------------------------
/* Destroys the semaphore */
threadMutex::~threadMutex()
{
}
									/*}}}*/
// Mutex::Lock - Aquires the semaphore					/*{{{*/
// ---------------------------------------------------------------------
/* Aquires ownership of the semaphore */
void threadMutex::Lock()
{
}
									/*}}}*/
// Mutex::UnLock - Releases ownership of the semaphore			/*{{{*/
// ---------------------------------------------------------------------
/* Unlocks the semaphore */
void threadMutex::UnLock()
{
}
									/*}}}*/
// Event::Wait - Wait for an event to arrive				/*{{{*/
// ---------------------------------------------------------------------
/* Returns the number of events since the last call <0 on error */
long threadEvent::Wait()
{
   return -1;
}
									/*}}}*/
// Event::Trigger - Triggers the semaphore				/*{{{*/
// ---------------------------------------------------------------------
/* Increases the event count by 1 and triggers any threads that are blocked */
void threadEvent::Trigger()
{
}
									/*}}}*/
// Event::threadEvent - Constructor					/*{{{*/
// ---------------------------------------------------------------------
/* Constructs the semaphore */
threadEvent::threadEvent()
{
}
									/*}}}*/
// Event::~threadEvent - Destructor				        /*{{{*/
// ---------------------------------------------------------------------
/* Destructs the semaphore */
threadEvent::~threadEvent()
{
}
									/*}}}*/
// threadSleep - Sleeps a number of miliseconds				/*{{{*/
// ---------------------------------------------------------------------
/* Sleeps some number of milliseconds. Some platforms have problems with
   this. */
void threadSleep(long)
{
}
									/*}}}*/
