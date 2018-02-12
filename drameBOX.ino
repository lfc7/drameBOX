/*******************************************************************
 *            DRAME BOX
 *            
 *  for teensy 3.5 ( and 3.6 but not tested )
 *  use modified non standard audio lib
 *  THAT's IMPORTANT "play_sd_raw.h" and "play_sd_raw.cpp" in audio lib
 *  are modified versions for this use.
 *  rename the originals and copy modified ones.
 *  
 *  PURPOSE:
 *  Play and loop a sample file from SDcard and send midiclock. 
 *  midiclock is generated using the lenght of the sample
 *  and the filename informations of total nb of measure (bar) 
 *  and nb of quarter note by measure.
 *  
 *  FILENAME CONVENTION:
 *  filename must follow this scheme: N_MM_QQ.raw 
 *  where N is the number of the preset (1 to 8), 
 *  MM (01 to 99) is the nb of measure (bar) in the sample,
 *  QQ (01 to 99) number of quarter note by mesure.
 *  (dont forget zero's for 01 to 09 !!!)
 *  
 *  FILE TYPE
 *  audio file must be raw audio PCM signed 16 bits
 *  
 *  SD card must have good tranfert speed to avoid
 *  hangs in audio (and clock).
 *  SDHC seems good for that.
 *  
 *  INPUT/OUTPUTs
 *  midi in receive control functions (by note or control change)
 *  midi out send midiclock
 *  audio out (mono dacs1)
 *  
 *  ON BOARD CONTROLS
 *  selector: select the sample to be played (1-8)
 *  if a sample is already playing, the selected one 
 *  will be play right at the end, then midiclock change 
 *  accordingly.
 *  
 *  start/stop button: start playing sample at start 
 *  stop do it, but midiclock continue in free wheeling mode.
 *  
 *  MIDI CONTROLS
 *  previous: select the previous sample to be played at start or when 
 *  an already running sample reach end.
 *  
 *  next: select the next sample to be played at start or when 
 *  an already running sample reach end.
 *  
 *  start/stop: guess what, it start or stop playing the sample
 *  
 *  mute: mute the playing sample
 *  
 *  SETTINGS @ boot
 *  choose number of ticks by quarter note
 *  
 *  selector position determine the number
 *  of ticks adding 8 ticks by position
 *  (example: position 3 -> 24 ticks)
 *  before power on, place selector on the choosen
 *  number of ticks.
 *  At power on, hold start button, that's done!
 *  setting is saved in EEPROM.
 *  
 *  NOTE
 *  to use with usb-midi uncomment all "//usbMIDI ..."
 *  and compile with the teensy midi usb handler option
 * *******************************************************************
 
   ____     __          ______              _       ____ ___  ___  ___ 
  / __/_ __/ /___ _____/_  __/______  ___  (_)___  |_  // _ \/ _ \/ _ \
 / _// // / __/ // / __// / / __/ _ \/ _ \/ / __/ _/_ </ // / // / // /
/_/  \_,_/\__/\_,_/_/  /_/ /_/  \___/_//_/_/\__/ /____/\___/\___/\___/ 
                                                                       
with love, for DRAME.
 * *******************************************************************
 */
#include <MIDI.h>
#include <midi_Defs.h>

#include <EEPROM.h>

#include <Bounce.h>

#include "Audio.h"

// GUItool: begin automatically generated code
AudioPlaySdRaw           playSdRaw1;     //xy=185.0104103088379,670.0104475021362
AudioMixer4              mixer1;         //xy=390.01047134399414,700.8993835449219
AudioOutputAnalogStereo  dacs1;          //xy=581.0103797912598,706.8994798660278
AudioConnection          patchCord1(playSdRaw1, 0, mixer1, 0);
AudioConnection          patchCord2(mixer1, 0, dacs1, 0);

// GUItool: end automatically generated code

/* MIDI SETTINGS */
#define MIDI_CHAN     MIDI_CHANNEL_OMNI //listen to all channels

#define MIDI_PREV_NOTE 0x01 //  increment audio file
#define MIDI_NEXT_NOTE 0x02 //  decrement audio file
#define MIDI_START_NOTE 0x03 // start/stop
#define MIDI_MUTE_NOTE 0x04 //  mute/unmute 

#define MIDI_VOLUME_CC 0x07 //  audio level
#define MIDI_START_CC 0x45  //  start/stop
#define MIDI_NEXT_CC 0x46 //    decrement audio file
#define MIDI_PREV_CC 0x47 //    increment audio file
#define MIDI_MUTE_CC 0x48 //    mute/unmute 

/* HARDWARE SETTINGS */
#define FILE_SELECTOR A0
#define VOLUME        A1
#define START_BUTT    16
#define SYNC_OUTPUT   17

#define LED1 25
#define LED2 26
#define LED3 27
#define LED4 28
#define LED5 29
#define LED6 30
#define LED7 31
#define LED8 32
//

//SOME OBJECTS INITS
Bounce start_butt = Bounce(START_BUTT, 5); //5ms of bounce

using namespace midi;
MIDI_CREATE_DEFAULT_INSTANCE();

//Gobals VARIABLEs*********************************************************

//TODO should use a struct
char   audioFile_filename[8][13];
double audioFile_tickevery[8] = {0}; //in micros
double audioFile_totaltime[8];


uint8_t tickDivision = 24;

double tickevery;
double totaltime; 
uint32_t ticksElapsed; //elapsedMicros
uint32_t nextTickMicros = 0;
uint8_t tickCounter = 0;

int fileSelector = 0;
int actualPlaying = 0;
int audioVolume = 0;
bool flag_stop = false;
bool mute_flag = false;

uint8_t leds[8] = { LED1, LED2, LED3, LED4, LED5, LED6, LED7, LED8 };


//ROUTINES *******************************************************

//init sd and get needed files if exist
//return true on success (ie: at least one correctly named file was found on sdcard)
bool getFilesNeeded()
{
  bool files_OK = false;
  uint8_t tickDivider=EEPROM.read(0);
  
  if (!SD.begin(BUILTIN_SDCARD)) { //BUILTIN_SDCARD
    //("initialization failed!");
    return files_OK;
  }

  //("initialization done.");
  File root;
  root = SD.open("/");

  while ( true ) {

    File entry =  root.openNextFile();
    if (! entry) {
      // no more files
      entry.close();
      break;
    }

    if (entry.isDirectory()) {
      //do nothing
      //files must be on root directory
      continue;
    } else {
      // here it found a file
      //copy filename
      char filefound[13] = {0};
      size_t destination_size = sizeof (filefound);
      snprintf(filefound, destination_size, "%s", entry.name());

      //filename: N_MM_QQ. where N is the number of the preset, MM (01 to 99) is the nb of mesure the sample contain, QQ (01 to 99) number of quarter by mesure

      //filename should start with 1 to 8 only
      if ( filefound[0] >= 49 and filefound[0] <= 56 and filefound[1] == '_') //ascii 49=1 et 56=8
      {
        int filenumber = filefound[0] - 49;
        char *strptr = filefound;
        snprintf(audioFile_filename[filenumber], destination_size, "%s", filefound);
        //next nb of mesure
        strptr += 2;
        if ( filefound[1] == '_' and atoi(strptr))
        {
          //next is nb of quarter by mesure
          strptr += 3;
          if ( filefound[4] == '_' and atoi(strptr) )
          {
            //so filename struct is ok
            files_OK = true;

            ledON(filenumber);

            //re-use ptr
            strptr = filefound;
            strptr += 2;
            int mesure = atoi(strptr);
            strptr += 3;
            int quarter = atoi(strptr);
            uint32_t audiofilesize = entry.size();
            //44.11764706B * 2 /ms so:
            audioFile_totaltime[filenumber] = ( audiofilesize * 1000.0 ) / (  44.11764706 * 2.0 ); //in  uS ... should give a 1ms precision? //220.5882353
            //n ticks by quarter note so:
            uint32_t TotalOfTicks = uint32_t(mesure) * uint32_t(quarter) * uint32_t(tickDivider);//24UL;
            audioFile_tickevery[filenumber] = audioFile_totaltime[filenumber] / double(TotalOfTicks);
            
            //debug
            /*
            Serial.print("filename:");
            Serial.println(filefound);
            Serial.print("totalTime:");
            Serial.println(audioFile_totaltime[filenumber]);
            Serial.print("mesure:");
            Serial.println(mesure);
            Serial.print("quarter:");
            Serial.println(quarter);
            Serial.print("TotalTicks:");
            Serial.println(TotalOfTicks);
            Serial.print("tickevery:");
            Serial.println(audioFile_tickevery[filenumber]);
            Serial.println("--------------------------------");
            */
          }
        }
      }
      // here we've got a file but it is not named as it should
      //so do nothing
    }
    entry.close();
  }

  //  SD.close();
  return files_OK;

}

//select with hardware selector the next file to be played 
void rotSelectFile()
{
  ///**
  int selector = analogRead(FILE_SELECTOR) >> 7;
  if ( selector != fileSelector )
  {
    if (audioFile_filename[selector][0] != 0)
    {
      fileSelector = selector;
      if ( playSdRaw1.isPlaying() )
      {
        playSdRaw1.playNextAtEnd(audioFile_filename[selector]);
        ledAllOFF();
       // ledON(actualPlaying);
        ledON(selector);
      }

    }
  }
  //**/
  //fileSelector = 0;
}

//select by midi the next file to be played 
void midiSelectFile(bool inc)
{
  int selector = fileSelector;
  if ( inc )
  {
    selector++;
    if (selector >= 8)selector = 0;
  } else {
    if (selector <= 0)selector = 8;
    selector--;
  }

  if (audioFile_filename[selector][0] != 0)
  {
    playSdRaw1.playNextAtEnd(audioFile_filename[selector]);
    fileSelector = selector;
    ledON(selector);
  }

}

//set volume with ext volume pedal // not USEFULL for now, NOT IN USE
void setVolume()
{

  int vol = analogRead(VOLUME) >> 3;
  if ( vol != audioVolume )
  {
    audioVolume = vol;
    float gain0 = float(vol) / 128.0;
    mixer1.gain(0, gain0);
  }
}

void startStop()
{
  flag_stop = !flag_stop;
  if ( flag_stop ) {
    playSdRaw1.stop();
    midiStop();
    ledAllOFF();
  } else {
    playSdRaw1.play(audioFile_filename[fileSelector]);
    playSdRaw1.loop(true);
    //reload cste
    tickevery = audioFile_tickevery[fileSelector];
    totaltime = audioFile_totaltime[fileSelector];
    midiStart();
    midiTick();
    ticksElapsed = micros();
    nextTickMicros = tickevery;
    actualPlaying=fileSelector;
    ledAllOFF();
    ledON(fileSelector);
  }
}

void handleNoteOn(byte channel, byte pitch, byte velocity)
{

   switch (pitch) {
    case  MIDI_START_NOTE: //START
      startStop();
      break;
 
    case MIDI_PREV_NOTE: //decrement audio file
      midiSelectFile(false);
      break;
      
    case MIDI_NEXT_NOTE: //increment audio file
      midiSelectFile(true);
      break;
      
    case MIDI_MUTE_NOTE: //mute demute audio file
      mute_flag = !mute_flag;
      mixer1.gain(0, float(mute_flag));
      break;
      
    default:
      // statements
      break;
  }

}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
  // Do something when the note is released.
  // Note that NoteOn messages with 0 velocity are interpreted as NoteOffs.

}

void handleControlChange(byte channel, byte number, byte value)
{
    switch (number) {
    case  MIDI_START_CC: //START
      startStop();
      break;
 
    case MIDI_VOLUME_CC:
      mixer1.gain(0, float(value) / 128.0);
      break;
      
    case MIDI_PREV_CC: //decrement audio file
      midiSelectFile(false);
      break;
      
    case MIDI_NEXT_CC: //increment audio file
      midiSelectFile(true);
      break;
      
    case MIDI_MUTE_CC: //mute demute audio file
      mute_flag = !mute_flag;
      mixer1.gain(0, float(mute_flag));
      break;
      
    default:
      // statements
      break;
    }
 

}

void midiTick()
{
  MIDI.sendRealTime( Clock );
  //usbMIDI.sendRealTime( Clock );
  //usbMIDI.send_now();
 
  if (tickCounter == 0)
  {
    digitalWriteFast( SYNC_OUTPUT , true );
    ledON(actualPlaying);
  }

  if (tickCounter == 3)
  {
    digitalWriteFast( SYNC_OUTPUT , false );
   ledOFF(actualPlaying);
    
  }

  tickCounter++;
  if (tickCounter >= tickDivision )tickCounter = 0;

}

void midiStart()
{
  MIDI.sendRealTime( Stop );
  //usbMIDI.sendRealTime( Stop );
  
  MIDI.sendRealTime( Start );
  //usbMIDI.sendRealTime( Start );
  
  //usbMIDI.send_now();
  tickCounter = 0;
}

void midiStop()
{
  MIDI.sendRealTime( Stop );
  //usbMIDI.sendRealTime( Stop );
 
}

void ledON(int lednb)
{
  digitalWriteFast( leds[lednb] , true );
}

void ledOFF(int lednb)
{
  digitalWriteFast( leds[lednb] , false );
}

void ledAllOFF()
{
  for (int i = 0; i < 8 ; i++)
  {
    digitalWriteFast( leds[i] , false );
  }
}

void ledAllON()
{
  for (int i = 0; i < 8 ; i++)
  {
    digitalWriteFast( leds[i] , true );
  }
}

void FatalError()
{
  while (true) //loop forever
  {
    ledAllON();
    delay(500);
    ledAllOFF();
    delay(750);
  }
}

void setup()
{
  //
  AudioMemory(10);

  //init I/O
  
  //init 1 buttons (start/stop)
  pinMode( START_BUTT, INPUT_PULLUP);

  //init 2 outputs : led , sync_output
  pinMode( LED_BUILTIN, OUTPUT );
  pinMode( SYNC_OUTPUT, OUTPUT );

  //init 8 outputs :  leds around selector
  for (int i = 0; i < 8 ; i++)
  {
    pinMode( leds[i], OUTPUT );
  }

  //init 2 ADC ( file selector (8 values) and mute (in fact volume) (also midi 127 values)
  fileSelector = analogRead(FILE_SELECTOR) >> 7;
  audioVolume = analogRead(VOLUME) >> 3; // not in use HARDWARE TODO

  delay(100);

  // config of number of tick/ quarter 
  if(!digitalRead(START_BUTT))
  {
    tickDivision = 8 + (8 * (analogRead(FILE_SELECTOR) >> 7));
    EEPROM.write(0, tickDivision);
  }else{
    tickDivision=EEPROM.read(0);
  }

  //init SD and get audiofile list
  if ( ! getFilesNeeded() )FatalError();

  //set midi loopback fct
  MIDI.setHandleNoteOn(handleNoteOn);  //
  //usbMIDI.setHandleNoteOn(handleNoteOn);
  
  // Do the same for NoteOffs
  MIDI.setHandleNoteOff(handleNoteOff);
  //usbMIDI.setHandleNoteOff(handleNoteOff);

  MIDI.setHandleControlChange(handleControlChange);
  //usbMIDI.setHandleControlChange(handleControlChange);

  // Initiate MIDI communications, 
  MIDI.begin(MIDI_CHAN);


  //init some var
  rotSelectFile();
  tickevery = audioFile_tickevery[fileSelector];
  
  //set initial volume;
  mixer1.gain(0, 1.0);

}

void loop()
{
  ///reset ticks when audio loop
  if ( playSdRaw1.isPlaying() )
  {
    if (playSdRaw1.didItLoop())
    {
      ticksElapsed = micros();
      //reload cste
      tickevery = audioFile_tickevery[fileSelector];
      totaltime = audioFile_totaltime[fileSelector];
      //reset ticks

      nextTickMicros = audioFile_tickevery[fileSelector];
      tickCounter = 0;
      actualPlaying=fileSelector;
      ledAllOFF();
      //ledON(fileSelector);
    }
  }

  ///tick when it's need
  if ( ( micros() - ticksElapsed ) >= nextTickMicros)
  {
    if (playSdRaw1.isPlaying())
    {
      if ( nextTickMicros < totaltime )
      {
        midiTick();
        nextTickMicros += tickevery;
      }//else: do nothing.Waiting for "audioplayer" sync flag on each loop
    } else {
      //freewheeling
      midiTick();
      nextTickMicros += tickevery;
    }

  }

  ///chk button
  start_butt.update();
  
  if (start_butt.fallingEdge())
  {
    startStop();
  }

  ///chk hardware selector
  rotSelectFile();


  //check incoming midi msg
  MIDI.read();
  //usbMIDI.read();


}



// eof
