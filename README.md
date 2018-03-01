# drameBOX
## The Drame band audio loop-player with midi-clock output
      
   For Teensy 3.5 ( and 3.6 but not yet tested ) with teensyduino.
   
   Use of a modified version of the standard Teensy audio lib.
   
   **THAT's IMPORTANT**: "play_sd_raw.h" and "play_sd_raw.cpp" in the teensy 
   audio lib, **must** be replaced by modified versions for this use.
   
   Copy teensy "Audio" folder in your personal arduino libraries folder
   
   Teensy Audio lib can be found at *.../arduino-1.x.x/hardware/teensy/avr/libraries/Audio*
   
   Rename the originals play_sd_raw.ccp and .h and copy modified ones.
   
   ### PURPOSE:
   Play and loop a sample file from SDcard and send midiclock. 
   
   Midiclock is generated using the lenght of the sample
   and the filename informations of total nb of measure (bar) 
   and nb of quarter note by measure.
   
   ### FILENAME CONVENTION:
   Filename must follow this scheme: N_MM_QQ.raw 
   where N is the number of the preset (1 to 8), 
   MM (01 to 99) is the nb of measure (bar) in the sample,
   QQ (01 to 99) number of quarter note by mesure.
   (dont forget zero's for 01 to 09 !!!)
   
   ### FILE TYPE
   Audio file must be "raw audio": signed-16bits PCM little-endian mono 44100Hz
   
   SD card must have good tranfert-speed to avoid
   hangs in audio (and clock).
   SDHC seems good for that.
   
   ### INPUT/OUTPUTs
   midi-in receive control functions (by note or control change)
   
   midi-out send midiclock, start and stop.
   
   audio-out (mono dacs1)
   
   ### ON BOARD CONTROLS
   -selector: select the sample to be played (1-8)
   if a sample is already playing, the selected one 
   will be play right at the end, then midiclock change 
   accordingly.
   
   -start/stop button: start playing sample at start,  
   stop do it, but midiclock continue in free wheeling mode.
   
   ### MIDI CONTROLS
   -previous: select the previous sample to be played at start or when 
   an already running sample reach end.
   
   -next: select the next sample to be played at start or when 
   an already running sample reach end.
   
   -start/stop: guess what, it start or stop playing the sample
   
   -mute: mute the playing sample
   
   ### SETTINGS @ boot
   -choose number of ticks by quarter note
   
   Selector position determine the number
   of ticks adding 8 ticks by position
   (example: position 3 -> 24 ticks)
   
   **Before** power on, place selector on the choosen
   number of ticks.
   
   Press start button. Keep Press.
   
   Power ON, wait for leds until setting is saved in EEPROM.
   
   Release start button
   
   
   ### NOTE
   to use with usb-midi uncomment all "//usbMIDI ..."
   and compile with the teensy midi usb handler option
  ```
     ____     __          ______              _       ____ ___  ___  ___
    / __/_ __/ /___ _____/_  __/______  ___  (_)___  |_  // _ \/ _ \/ _ \
   / _// // / __/ // / __// / / __/ _ \/ _ \/ / __/ _/_ </ // / // / // /
  /_/  \_,_/\__/\_,_/_/  /_/ /_/  \___/_//_/_/\__/ /____/\___/\___/\___/ 
                                                                         
with love, for DRAME.
```
  
