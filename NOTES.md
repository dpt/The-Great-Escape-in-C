NOTES
=====

Isometric Projections
---------------------

Aug 2017

The "isometric" projection used in The Great Escape is, like many of its pixel-bound contemporaries, actually a _dimetric_ projection. Isometric means "having equal dimensions" e.g. a trio of 120 degree angles. However the angles used by TGE are approx 117:117:126 degrees. [^link1] [^link2]

The Great Escape stores an (x,y,z) map position for each character. To place a character or object on-screen we must project its point into isometric 2D. The standard fast method for computing an isometric projected point is:

    x' = x - z
    y' = y + (x + z) / 2

However The Great Escape uses:

    x' = (64 + z - x) * 2
    y' = 256 - x - z - y

(Notation note: 'y' in the game code is 'z' here and 'height' in the game code is 'y' here).

This has the effect of flipping the projected coordinates horizontally and vertically as well as doubling the size of the output and shifting it around a bit. Why is it this way instead of the standard method? I'm not exactly sure but removing the division is a likely explanation. (Well, the map's authored flipped h/z compared to the isometric projected map).

Example: The middle home hut at <94,82..98,98> in the game map becomes <104,80 - 96,76 - 128,60 - 136,64> when projected.

In addition see that in The Great Escape the 'y' (height) value is divided by two. I'm unsure if this is intentional. It means that heights will be 'worth' half as much in terms of movement compared to the standard method.

[^link1]: http://www.cpcwiki.eu/index.php/Isometric_3D
[^link2]: https://en.wikipedia.org/wiki/Isometric_graphics_in_video_games_and_pixel_art]

Coordinate Systems
------------------

Nov 2017

I'm writing this section in order to attempt to sort out in my head the naming of the coordinate systems within the game. I've failed to elucidate these usefully so far and have to fix that. Some of the existing naming is resulting in collisions that are making descriptions of the game and naming variables too hard. Also there's one variable called `floogle` which has that daft name because calling it 'isometrically projected map coordinate' seemed unwieldy.

Note: In the following a tilde `~` denotes approximate sizes and coordinates which I've not yet measured exactly.

## (1) The game map

The game map is a 2D ~(100x80) position grid, but numerically offset by ~(50,40). There's also a third dimension used at points in the code that holds heights, but it's not always present.

If the coordinate (0,0) is considered to be at the top-left of the map then the map is oriented so that the exercise yard is at the top of the map and the fence is on the left of the map.

```
             _____________
            :T. . . . . . : <-- exercise yard
            :. . . . . . .:
            : . . . . . . :T
     _______:___GG________:
    :                     :            _
    :    _______GG________=GATE=======| 
    :   :T                            |
    :   :      |H|   |H|   |H|         |
    :   :      |U|   |U|   |U|        _|
    :   :T     |T|   |T|   |T|       |  
    :   :      |3|   |2|   |1|       |
    :   :                            |
    |~~~~~~~~~~~~~~|           RRR   |
    |              |                 |
    |              '~~~~~~~~~~~~~~~~~'
```

Symbols:

* GATE = Main gate
* GG = Gates to exercise yard
* RRR = Roll call area
* T = Watchtower

Rotated 180 degrees it's a bit more familiar as the in-game map.

```
       _________________               |
      |                 |              |
      |   RRR           |______________|
      |                            :   :
      |       |H|   |H|   |H|      :   :
     _|       |U|   |U|   |U|     T:   :
    |         |T|   |T|   |T|      :   :
    |         |1|   |2|   |3|      :   :
     |                            T:   :
    _|=======GATE=~~~~~~~~GG~~~~~~~'   :
                 :                     :
                 :~~~~~~~~GG~~~:~~~~~~~'
                T: . . . . . . :
                 :. . . . . . .:
                 : . . . . . .T:
                 '~~~~~~~~~~~~~'
```

Hut 2 is the home hut.

The map spans ~(52..152,44..128). (Exact values TBC).

When viewed in-game (isometrically) the top-left accessible position is ~(128,112) in map coordinates.

The main gate ("GG") is (105..109,73..75).

### Isometric Projection

Taking the roll call area as an example ("RRR" on map diagrams above). It has map coordinates of (114..124,106..114). If this is viewed isometrically on-screen in the game, like so:

```
                         T
                      . '  ` .
                   . '         ` .
                . '                ` .
             . '                       ` .
          . '                              ` .
       . '                                     ` .
    L '                                            ` .
      ` .                                              ` R
          ` .                                         .'
              ` .                                  .'
                  ` .                           .'
                      ` .                    .'
                          ` .             .'
                              ` .      .'
                                  ` B'
```

...then (114,106) is the bottom corner (labelled 'B') and (124,114) is the top corner (labelled 'T').

Which of the game's routines perform isometric projection? `calc_vischar_iso_pos_from_vischar`, `calc_vischar_iso_pos_from_state`, `calc_exterior_item_iso_pos` and `calc_interior_item_iso_pos`.

Note that nothing seems to goes the other way: there are no routines which map from isometric space back into map coordinates. There's no mouse to click on the screen to create this requirement.

### Indoors/Outdoors

The map coordinates change in scale between the indoor and outdoor scenes. [DOCUMENT THIS]

### Naming

"Map space" is therefore anything in this 2.5D coordinate space.

## (2) The background

The background is the *picture* of the map that the characters are drawn over. There's also a foreground which is composed of masks which cut away at characters when they move behind buildings, fences, watchtowers (and other objects when in interior scenes).

Confusingly this also ends up being 'the map'. It's a 54x34 grid of references to 4x4 _supertiles_ which in turn refer to 8x8 pixel tiles, for a total of 1728x1088 pixels.

I've called this the "Game exterior map" in the disassembly, which isn't clear either.

`map_position` in the game state affects which part of the background is shown. It's a pixel offset from the top-left of the background. The game can scroll the background on a 4x4 granularity.

So if we isometrically project from map space into background space, what is that called? Something like `map_to_iso`.

## Links

https://shaunlebron.github.io/IsometricBlocks/
http://bannalia.blogspot.co.uk/2008/02/filmation-math.html



vim: wrap linebreak textwidth=0 wrapmargin=0 breakindent

