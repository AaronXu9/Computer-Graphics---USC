Program's features:
1. use input from the keyboard to change render mode, no matter what is lowercase 
or uppercase.
   p -- point
   l -- wireframe
   t -- solid triangle

2. use input from the mouse to rotate the heightfield.
3. press CTRL on the keyboard, use mouse to translate the heightfield
4. press SHIFT on the keyboard, use mouse to scale the heightfield.
5. heightmap image is ./heightmap/Heightmap-color;

Extra:
1. Use element arrays and glDrawElements.
2. Support color (ImageIO::getBytesPerPixel == 3) in input images.
3. Color the vertices based on color values taken from another image of equal size.
   