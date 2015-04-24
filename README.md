wispy maps
==========

As in https://www.flickr.com/photos/walkingsf/sets/72157628616533659

Cartogram tool
--------------

Requires the "cart" cartogram tool with my slight modifications
at https://github.com/ericfischer/cart

Put that in a sibling directory to this one and

    ( cd ../cart; make cart interp )

Making the density map
----------------------

With a file containing lines in this format:

    user date time lat,lon

For example,

    curl -O http://sfgeo.org/data/tourist-local/1-RESIDENT.gz

Accumulate a grid of density:

    cc -g -Wall -O3 -o gather-density gather-density.c -lm
    zcat 1-RESIDENT.gz | ./gather-density > density

The end of the output will look like

    1000 1000
    51.366408 -0.324268 51.584092 0.025234

Keep those numbers. You will need them.

Making the cartogram
--------------------

Then make a cartogram, using the "1000 1000" part as the bounds:

    ../cart/cart 1000 1000 density density.out

Making the image
----------------

    cc -g -Wall -O3 -o mapback mapback.c -lm -ljpeg

    zcat 1-RESIDENT.gz | ./plot-motion | ./invert 1000 1000 51.366408 -0.324268 51.584092 0.025234 | ../cart/interp 1000 1000 density.out | ./mapback 1000 1000 density.out 

And at the end you should get a file called out.jpg with the wispy image.
