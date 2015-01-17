CCHIP8
======

Chip8 emulator written in C.

This was a "fun" side-side-project that I've started during winter break of 2014/2015 for educational purposes. At first, my friend and I were planning to make a Gameboy emulator, but the number of opcodes seemed overwhelming, and my friend no longer had time to work on it with me at the moment. So I ended up making a CHIP-8 emulator in C in the meantime. 

This would have been just a fun and totally not tedious project if the documentaions behind each opcode were a bit more clear on what exactly happens on the hardware level. For example (at least as far as my search history on google goes), no rememberable documentations addressed the issues of the wrap around on the display which can cause some of the games to crash (in PONG, either players could crash the game when one's losing simply by moving further down/up than intended as it didn't bound its bars to the playable grid). In addition, since CHIP-8 was never an actual machine, but rather an interpretive programming language, there's no intended speed for the clock speed, which makes some of the game look too clunky or abnormally fast. Still, it was interesting to implement CHIP-8.

Regardless, big thanks to Laurence Muller's guide for the GLUT implementations/snippets and Matthew Mikolay's guide on in-depth details of the opcodes.