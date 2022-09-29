# Escape the Room

Author: David Zelong Fan

Design: An escape room game that imitates natural movement and allows the player to traverse the room. There are TWO endings.

Text Drawing: Text is rendered at runtime. A .ttf file is used to generate a FreeType face, used to generate a harfbuzz font. We then use harfbuzz codrpoints to generate glyphs in FreeType, and position the glyphs using harfbuzz position values. (The position doesn't really work and I ran out of time, so I forced everything into upper case).

Choices: The game stores all the states in a single finite state machine. I really wanted to try this, but (as expected) this is not a good structure. Every new item would double the number of states, so I gave up on that idea.

Screen Shot:

![Screen Shot](screenshot.png)

How To Play:
Press WASD to make choices and progress through the game. There are two endings, one ending is better than the other.


Sources: 
https://fonts.google.com/specimen/Roboto

https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c

https://learnopengl.com/In-Practice/Text-Rendering

https://learnopengl.com/Getting-started/Shaders

https://stackoverflow.com/questions/735204/convert-a-string-in-c-to-upper-case

https://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c

This game was built with [NEST](NEST.md).

