/* ########################################################################

   This REXX script will communicate with a running Module Player that
   support a Command Pipe.

   The Following commands are used
      Query Song  -- Returns three lines, first is the file name, second is the
                     index and third is the song title. They represent
                     the currently playing song.
      SongList EraseIndx -- Nukes 1 item in the list (by index)
      Play Next -- Plays the next song

   ########################################################################
*/
'@echo off'                       /* Don't echo commands */
say " Module Player Songlist Generator V1.0                                    Ethos"
   call OpenPlayer
   if Result <> 0 then do
      say "Error" Error
      return
   end

   if (arg(1) = '') then do
      say "Require a filename to write the songlist to."
      return
   end

   if CallPlayer("Query","SongList","") <> 0 then do
      say "Error" Error
      return
   end

   'del' arg(1)
   I = 2;
   do while (I <= Result.0)
      call lineout arg(1),Result.I
      I = I + 1
   end

return

/* ########################################################################

   Function - CallPlayer
   Eg - if (CallPlayer("Query","Song","") <> 0) then Error

   Description - This procedure sends a command message to the player.
                 An error is returned if the player doesn't understand
                 the message or something else is wrong.
                   Result - A stem containing each line of the result
                   Error - Global error code

   ########################################################################
*/
CallPlayer: procedure expose Result. Player Error
   parse arg Command, SCommand, Arg

   Error = ""
   Result.0 = 0

   /* Player not loaded */
   if (Player = "") then do
      Error = "Player not Inited"
      return 1
   end

   call lineout Player,Command SCommand Arg

   /* Get all the results */
   do while (1)
      Line = linein(Player)

      /* Error or something */
      if (pos("!!",Line) = 1) then do
         if (Line = "!! OK") then
            return 0
         Error = substr(Line,4)
         return 1
      end;

      Result.0 = Result.0 + 1
      I = Result.0
      Result.I = Line
   end

return 0

/* ########################################################################

   Function - OpenPlayer
   Eg - call OpenPlayer

   Description - This procedure opens the pipe and sets the following:
                   Player - The name of the pipe - \pipe\Player
                   Version - The pipe comminication version
                   Type - The program running on the other end, 'Text UI'
                   Error - Global Error Code

   ########################################################################
*/
OpenPlayer:procedure expose Player Version Type Error
   Error = ""

   Result = Stream("\pipe\ModulePlayer","C","OPEN");
   if (Result <> "READY:") then do
      Error = "Unable to communicate with the  Player, is it running?"
      Player = ""
      return 1
   end
   Player = "\pipe\ModulePlayer"

   /* Get the version */
   S = linein(Player)
   parse var S 'V' Version .
   Type = linein(Player)
return 0



