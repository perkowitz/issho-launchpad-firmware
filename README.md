
# Issho firmware for the Novation Launchpad Pro mk1

This is my own sequencer firmware for the Launchpad Pro, based on the 
[official Novation firmware](https://github.com/dvhdr/launchpad-pro). See that repo
for instructions on how to develop and install custom firmware.

## Flow

This repo includes my sequencer **FLOW**. You can check out and build the code, or
download the sysex file in the release. The release manual includes installation instructions.

**FLOW** is an 8-stage sequencer. Each stage can play different notes, be of different duration, and repeat itself multiple times. All of these effects are created by placing "markers" on the stage. Markers include specifying notes, sharps/flats, octave up/down, extended duration, repeats, and so on.

With multiple extends and repeats, sequences can be of irregular lengths, with complex, shifting rhythms. The sequence can be free running or regularly reset to provide more predictability.
