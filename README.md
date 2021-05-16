
# Issho firmware for the Novation Launchpad Pro mk1

This repo contains new firmware for the Novation Launchpad Pro, based on the 
[official Novation firmware](https://github.com/dvhdr/launchpad-pro). See that repo
for instructions on how to develop and install custom firmware.


## Flow

This repo includes my sequencer **FLOW**. You can check out and build the code, or
download the sysex file in the release. The release manual includes installation instructions.

**FLOW** is an 8-stage sequencer. Each stage can play different notes, be of different duration, and repeat itself multiple times. All of these effects are created by placing "markers" on the stage. Markers include specifying notes, sharps/flats, octave up/down, extended duration, repeats, and so on.

With multiple extends and repeats, sequences can be of irregular lengths, with complex, shifting rhythms. The sequence can be free running or regularly reset to provide more predictability.

[Latest Manual](https://github.com/perkowitz/issho-launchpad-firmware/releases/download/v0.3/FLOW.Manual.v0.3.pdf)

[Latest Sysex file](https://github.com/perkowitz/issho-launchpad-firmware/releases/download/v0.3/issho_launchpad_flow.syx)


## Poke

Poke is a simple game that can be played on the launchpad. See [rules](Poke.md).


## Releases

[v0.3: more performance features in FLOW](https://github.com/perkowitz/issho-launchpad-firmware/releases/tag/v0.3)
- Adds Jump mode, allowing manual selection of next stage by tapping pads.
- Adds Flow mode, sequencing multiple patterns.
- Removes PDF files from repo (they will be linked to each release).

[v0.2: performance updates for FLOW](https://github.com/perkowitz/issho-launchpad-firmware/releases/tag/v0.2)
- Adds the shuffle feature.
- Copying shuffled pattern preserves shuffled order.
- Adds length control.
- Some rearrangement and recoloring of controls.
- Manual PDFs added to repo.

[v0.1: initial version of FLOW](https://github.com/perkowitz/issho-launchpad-firmware/releases/tag/v0.1)
- Implements basic features of FLOW sequencer.
- Missing internal tempo change, some performance features, pattern copy, some pattern modifiers.
- Release includes sysex file to load firmware and a full PDF manual.

