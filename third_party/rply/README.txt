RPly 1.1.3 by Diego Nehab (MIT license).

Imported unchanged from:
https://github.com/octopus-prime/gamma-ray-2/tree/master/source/surface/primitive/mesh

The full license and copyright notices are preserved in rply.c and rply.h.
Include these notices when redistributing this dependency.

RPly decodes ASCII and little-/big-endian binary PLY data. Its public value
callback uses double; the application validates and converts vertex values to
scalar_t before constructing geometry. Mesh intersection math uses scalar_t.
