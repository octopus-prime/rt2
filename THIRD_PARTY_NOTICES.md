# Third-party notices

The root [LICENSE](LICENSE) covers rt2's own code, not the independently
licensed libraries, fonts or model assets below. System dependencies are
not vendored here, except for RPly.

## Boost

Boost.QVM, Boost.Geometry, Boost.Math, Boost.GIL and other Boost headers are
used under the **Boost Software License 1.0**. The full license text is in
[LICENSE](LICENSE); retain the individual upstream copyright notices when
redistributing Boost source.

Source: <https://www.boost.org/>.

## RPly 1.1.3

Copyright (C) 2003-2013 Diego Nehab. **MIT license**.

RPly is vendored unchanged in [third_party/rply](third_party/rply).
The full copyright, permission and warranty notices are preserved at the end
of both [rply.c](third_party/rply/rply.c) and [rply.h](third_party/rply/rply.h).
Include those notices with source and binary distributions containing RPly,
including distributions of the statically linked rt2 executable.

Imported from the author's earlier
[gamma-ray-2 mesh implementation](https://github.com/octopus-prime/gamma-ray-2/tree/master/source/surface/primitive/mesh).
The SOR and text implementations also adapt work from that project by
[octopus-prime](https://github.com/octopus-prime); the RPly license remains
separate from the project license.

## FreeType

This software is based in part on the work of the FreeType Team.
Portions of this software are copyright (c) 2025 The FreeType Project
(https://freetype.org). All rights reserved.

FreeType is used under the **FreeType Project License (FTL)**, not its
alternative GPL license. The credit above reflects FreeType 2.14.2 used for
the current build. Retain this acknowledgement with binary distributions;
update the year for other FreeType releases as appropriate.

Full license: <https://gitlab.freedesktop.org/freetype/freetype/-/blob/VER-2-14-2/docs/FTL.TXT>.
If redistributing FreeType source, include its unaltered `FTL.TXT`, retain
source copyright notices, and document any modifications. FreeType source
is not included in this repository.

## libpng

PNG output uses Boost.GIL and the system libpng. The current build uses
libpng 1.6.57 under the following license. If bundling another version,
include the notices supplied with that version, including legacy notices
where applicable. Source: <http://www.libpng.org/pub/png/libpng.html>.

### PNG Reference Library License version 2

Copyright (c) 1995-2026 The PNG Reference Library Authors.
Copyright (c) 2018-2026 Cosmin Truta.
Copyright (c) 2000-2002, 2004, 2006-2018 Glenn Randers-Pehrson.
Copyright (c) 1996-1997 Andreas Dilger.
Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.

The software is supplied "as is", without warranty of any kind,
express or implied, including, without limitation, the warranties
of merchantability, fitness for a particular purpose, title, and
non-infringement. In no event shall the Copyright owners, or
anyone distributing the software, be liable for any damages or
other liability, whether in contract, tort or otherwise, arising
from, out of, or in connection with the software, or the use or
other dealings in the software, even if advised of the possibility
of such damage.

Permission is hereby granted to use, copy, modify, and distribute
this software, or portions hereof, for any purpose, without fee,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you
   must not claim that you wrote the original software. If you
   use this software in a product, an acknowledgment in the product
   documentation would be appreciated, but is not required.
2. Altered source versions must be plainly marked as such, and must
   not be misrepresented as being the original software.
3. This Copyright notice may not be removed or altered from any
   source or altered source distribution.

## zlib

libpng uses zlib under the **zlib license**. Source: <https://zlib.net/>.
The notice below is reproduced from the installed distribution's copyright
file; preserve the notices from the version you redistribute.

Copyright (C) 1995-2013 Jean-loup Gailly and Mark Adler

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

## DejaVu Sans

The demo and tests load the system's DejaVu Sans font; no font binary is
included here. DejaVu changes are in the public domain; the underlying
Bitstream Vera font carries the following notice.
Source: <https://dejavu-fonts.github.io/>.

Copyright (c) 2003 by Bitstream, Inc. All Rights Reserved.
Bitstream Vera is a trademark of Bitstream, Inc.
DejaVu changes are in public domain.

Permission is hereby granted, free of charge, to any person obtaining a copy
of the fonts accompanying this license ("Fonts") and associated
documentation files (the "Font Software"), to reproduce and distribute the
Font Software, including without limitation the rights to use, copy, merge,
publish, distribute, and/or sell copies of the Font Software, and to permit
persons to whom the Font Software is furnished to do so, subject to the
following conditions:

The above copyright and trademark notices and this permission notice shall
be included in all copies of one or more of the Font Software typefaces.

The Font Software may be modified, altered, or added to, and in particular
the designs of glyphs or characters in the Fonts may be modified and
additional glyphs or characters may be added to the Fonts, only if the fonts
are renamed to names not containing either the words "Bitstream" or the word
"Vera".

This License becomes null and void to the extent applicable to Fonts or Font
Software that has been modified and is distributed under the "Bitstream
Vera" names.

The Font Software may be sold as part of a larger software package but no
copy of one or more of the Font Software typefaces may be sold by itself.

THE FONT SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF COPYRIGHT, PATENT,
TRADEMARK, OR OTHER RIGHT. IN NO EVENT SHALL BITSTREAM OR THE GNOME
FOUNDATION BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, INCLUDING
ANY GENERAL, SPECIAL, INDIRECT, INCIDENTAL, OR CONSEQUENTIAL DAMAGES,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
THE USE OR INABILITY TO USE THE FONT SOFTWARE OR FROM OTHER DEALINGS IN THE
FONT SOFTWARE.

Except as contained in this notice, the names of Gnome, the Gnome
Foundation, and Bitstream Inc., shall not be used in advertising or
otherwise to promote the sale, use or other dealings in this Font Software
without prior written authorization from the Gnome Foundation or Bitstream
Inc., respectively. For further information, contact: fonts at gnome dot
org.

## Fantasy Dragon model and rendered images

**Fantasy Dragon by Artec Group inc.** (<https://www.artec3d.com/>),
source: <https://www.artec3d.com/3d-models/fantasy-dragon>.

The supplied asset is licensed under **Creative Commons Attribution 3.0
Unported (CC BY 3.0)**:
<https://creativecommons.org/licenses/by/3.0/>.
Full legal terms: <https://creativecommons.org/licenses/by/3.0/legalcode>.
Its accompanying `mesh/fantasy-dragon-ply/license.txt` contains the author's
notice and full license; keep that file with any redistributed mesh.
The asset directory is ignored by Git, so this attribution is also recorded
here independently.

The demos scale, rotate and translate the model, generate smooth normals,
and render it with a gold material. Geometry coordinates in the supplied
PLY file are not rewritten. When publishing demo images, include a credit
such as:

> Contains "Fantasy Dragon" by Artec Group inc.,
> https://www.artec3d.com/3d-models/fantasy-dragon,
> licensed under CC BY 3.0, https://creativecommons.org/licenses/by/3.0/.
> Rendered in rt2 with generated normals, transformed instances and a gold material.

Using Boost, libpng or zlib does not put their software licenses on the
generated PNGs. The dragon's attribution obligations still apply to images
depicting it. Font files and other assets you substitute may have different
terms; check them separately.

## Distribution scope

Ship this notice and the RPly notices with rt2 binary releases. When bundling
libraries or assets, also ship their version-specific licenses and copyright
notices. The current Linux executable additionally links system C/C++ runtimes
and FreeType's bzip2/Brotli dependencies; these are not vendored here.
If packaging those libraries rather than relying on system packages, include
their own licenses as well. This file is not a license inventory for a whole
operating-system or runtime bundle.
