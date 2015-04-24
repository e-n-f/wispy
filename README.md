wispy maps
==========

Requires the "cart" cartogram tool from http://www-personal.umich.edu/~mejn/cart/

Making the density map
----------------------

With a file containing lines in this format:

    user date time lat,lon

For example,

    curl -O http://sfgeo.org/data/tourist-local/1-RESIDENT.gz

Accumulate a grid of density:

    cc -g -Wall -O3 -o gather-density gather-density.c -lm
    zcat 1-RESIDENT.gz | ./gather-density > density

