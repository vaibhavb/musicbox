// will play MP3's from the root of an SD card, ignoring other files
// By XTronical, www.xtronical.com, use as you wish
// Based on work and on the audio library of schreibfaul1
// See github page : https://github.com/schreibfaul1/ESP32-audioI2S
// Also has volume control via a potentiometer attached to pin 13

#include "Arduino.h"
#include "Audio.h"
#include "SD.h"
#include "FS.h"

// Digital I/O used
#define SD_CS 5
#define SPI_MOSI 23  // SD Card
#define SPI_MISO 19
#define SPI_SCK 18

#define REED_SWITCH 21

#define I2S_DOUT 25
#define I2S_BCLK 27  // I2S
#define I2S_LRC 26

Audio audio;

File RootDir;

int reedSwitchState;
bool isSongPlaying;
int loopCount;

void setup() {
  int reedSwitchState = LOW;
  bool isSongPlaying = false;
  int loopCount = 0;

  Serial.begin(57600);
  Serial.println("Setup");
  // Print some useful debug output - the filename and compilation time
  Serial.println(__FILE__);
  Serial.println("Compiled: " __DATE__ ", " __TIME__);

  pinMode(SD_CS, OUTPUT);
  pinMode(REED_SWITCH, INPUT_PULLUP);  // set ESP32 pin to input pull-up mode
  digitalWrite(SD_CS, HIGH);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  if (!SD.begin(SD_CS)) {
    Serial.println("Error talking to SD card!");
    while (true)
      ;  // end program
  }
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  RootDir = SD.open("/");
  Serial.println("setting volume");
  audio.setVolume(10);  // Check volume level and adjust if necassary
}

/* this function is called repeated by adruino, using it to check sensor states */
void loop() {
  reedSwitchState = digitalRead(REED_SWITCH);  // read state

  if (isSongPlaying) {
    // Serial.println("Song is playing");
    audio.loop();
  } else if (reedSwitchState == HIGH && !isSongPlaying) {
    /* audio_eof_mp3 will be called when current song finishes */
    // audio.loop();
    Serial.println("Set song playing = true");
    isSongPlaying = true;
    PlayNextSong();
    Serial.println("done");
  } else {
    if (loopCount % 20 == 0) {
      Serial.println("No song is playing and the box is closed");
    }
  }
  loopCount += 1;
}

/* this function is called by audio when current mp3 finishes */
void audio_eof_mp3(const char *info) {  //end of file
  /* play next song only if box is open */
  isSongPlaying = false;
  // if (reedSwitchState == HIGH){
  //   PlayNextSong();
  // }
}

void PlayNextSong() {
  bool SongFound = false;
  bool DirRewound = false;

  while (SongFound == false) {
    Serial.println("finding files");
    File entry = RootDir.openNextFile();
    if (!entry)  // no more files
    {
      Serial.println("no more files");
      if (DirRewound == true)  // If we've already rewound once then there are no songs to play in this DIR
      {
        Serial.println("No MP3 files found to play");
        entry.close();
        return;
      }
      //else we've reached the end of all files in this directory, just rewind back to beginning
      entry.close();
      Serial.println("Rewind directory");
      RootDir.rewindDirectory();  // reset back to beginning
      DirRewound = true;          // Flag that we've rewound
    } else {
      Serial.println("found file");
      if (!entry.isDirectory())  // only enter this if not a DIR
      {
        Serial.println("not dir");
        if (MusicFile(entry.name()))  // Only enter if one of the acceptable music files
        {
          //Serial.print("Playing ");Serial.println(entry.name());
          audio.connecttoSD(entry.name());  // Play the file
          Serial.println("called audio.connectoSD");
          SongFound = true;
        }
      }
    }
    Serial.println("closing");
    entry.close();
  }
}


bool MusicFile(String FileName) {
  // returns true if file is one of the supported file types, i.e. mp3,aac
  String ext;
  ext = FileName.substring(FileName.indexOf('.') + 1);
  if ((ext == "mp3") | (ext == "aac"))
    return true;
  else
    return false;
}
