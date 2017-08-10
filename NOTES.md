NOTES
=====

Isometric Projections
---------------------

The "isometric" projection used in The Great Escape is, like many of its pixel-bound contemporaries, actually a _dimetric_ projection. Isometric means "having equal dimensions" e.g. 120 degrees, however the angles used by TGE are approx 117:117:126 degrees. [See http://www.cpcwiki.eu/index.php/Isometric_3D, https://en.wikipedia.org/wiki/Isometric_graphics_in_video_games_and_pixel_art]

The Great Escape stores an (x,y,z) position for each character. To place a character on-screen we must project the point into 2D. The standard fast method for computing an isometric projected point is:

    x' = x - z
    y' = y + (x + z) / 2

However The Great Escape uses:

    x' = (64 + z - x) * 2
    y' = 256 - x - z - y

(Notation: 'y' in the game code is 'z' here and 'height' in the game code is 'y' here).

This has the effect of flipping the projected coordinates horizontally and vertically as well as doubling the size of the output and shifting it around a bit. Why is it this way instead of the standard method? I'm not exactly sure but removing the division is a likely explanation.

Example: The middle home hut at <94,82..98,98> in the game map becomes <104,80 - 96,76 - 128,60 - 136,64> when projected.

In addition see that in The Great Escape the 'y' (height) value is divided by two. I'm unsure if this is intentional. It means that heights will be 'worth' half as much in terms of movement compared to the standard method.

