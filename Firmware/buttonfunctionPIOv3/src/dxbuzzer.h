#ifndef DX_BUZZER_H_FILE
#define DX_BUZZER_H_FILE

#include <Arduino.h>

class dxbuzzer
{
private:
    int tempo = 180;
    int buzzerPin = 23;
    int melody[6] = {659, 8, 3951, 4, 3322, 8};
    // sizeof gives the number of bytes, each int value is composed of two bytes (16 bits)
    // there are two values per note (pitch and duration), so for each note there are four bytes
    int notes = sizeof(melody) / sizeof(melody[0]) / 2;
    // this calculates the duration of a whole note in ms
    int wholenote = (60000 * 4) / tempo;
    int divider = 0, noteDuration = 0;

public:
    
// dxbuzzer();

void playSong();

};

// dxbuzzer::dxbuzzer()
// {
// }

/*
 * Plays a sound of duration ~1s on the buzzer.
 * 
 * Used to denote the machine being ready upon powerup, as well as
 * to inform the user when a dispense is completed.
 */
void dxbuzzer::playSong()
{
  // iterate over the notes of the melody.
  // Remember, the array is twice the number of notes (notes + durations)
  for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2)
  {

    // calculates the duration of each note
    divider = melody[thisNote + 1];
    if (divider > 0)
    {
      // regular note, just proceed
      noteDuration = (wholenote) / divider;
    }
    else if (divider < 0)
    {
      // dotted notes are represented with negative durations!!
      noteDuration = (wholenote) / abs(divider);
      noteDuration *= 1.5; // increases the duration in half for dotted notes
    }

    // we only play the note for 90% of the duration, leaving 10% as a pause
    tone(buzzerPin, melody[thisNote], noteDuration * 0.9);

    // Wait for the specief duration before playing the next note.
    delay(noteDuration);

    // stop the waveform generation before the next note.
    noTone(buzzerPin);
  }
}

#endif