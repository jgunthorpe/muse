/* ########################################################################

   This REXX script will communicate with a running Module Player that
   support a Command Pipe.

   The Following commands are supported by the interface,
      Query Song  -- Returns three lines, first is the file name, second is the
                     index and third is the song title. They represent
                     the currently playing song.
      Query NextSong -- Returns the file name and index of the next song
      Query SongList -- Returns the # of items in the first line and each
                        line after contains 1 item from the song list.
      Query LoopStyle -- Returns the current loop style,
                         param 1 = Normal, 2 = Non looped, 3 = inifite
                         loop
      Query Server -- Returns TRUE if Player is in server mode, (TRUE,FALSE)
      Play Next -- Plays the next song
      Play Last -- Plays the last song
      Play Pause -- Pause play back
      Play Resume -- Resume play back
      Play Index -- Jumps to the given index in the song list and plays,
                    returns 1 line, the filename
      Play Song -- Searches the song list for the given substring match,
                   first one is played and the name and index are returned.
      Set LoopStyle -- Sets the current loop style, argument is 0,1,2
      Set Server -- Set server mode, 0,1,FALSE,TRUE
      SongList Add -- Add items to the song list (at the end), ie
                        SongList Add *.mod *.s3m
                      Returns the new size of the list
      SongList Erase -- Nukes the entire song list
      SongList EraseIndx -- Nukes 1 item in the list (by index)
      Exit -- Terminate the player

   ########################################################################
*/
'@echo off'                       /* Don't echo commands */
say " REXX Module Command Pipe Tester V1.0                                     Ethos"
   call OpenPlayer
   if Result <> 0 then do
      say "Error" Error
      return
   end
   say "Command pipe opened to" Type || ". Command set Version" Version
   say "Type quit to exit"

   do while (1)
      Line = linein();
      if translate(Line) = "QUIT" then return

      parse var Line A B C

      if CallPlayer(A,B,C) <> 0 then do
         say "Error" Error
         iterate;
      end

      if Result.0 > 1 then
         say "---"
      else
         say "Ok"
      I = 1;
      do while (I <= Result.0)
         say Result.I
         I = I + 1
      end

      if Result.0 > 1 then
         say "---"

      if translate(Line) = "EXIT" then return
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
   say S
   parse var S 'V' Version .
   Type = linein(Player)
   say Type
return 0
