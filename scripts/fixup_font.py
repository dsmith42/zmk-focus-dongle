#!/usr/bin/env python3
"""Make an lv_font_conv output compile inside this module.

Two edits, both of which the generated file needs every time. Kept out of
gen_dinish.sh so the shell script does not have to nest a heredoc per font.
"""

import sys

p = sys.argv[1]
s = open(p).read()

# lv_font_conv emits a conditional include that resolves to "lvgl/lvgl.h" unless
# LV_LVGL_H_INCLUDE_SIMPLE is defined, which Zephyr does not define -- and that
# path is not on the include path here, so the file fails to compile.
old = '#ifdef LV_LVGL_H_INCLUDE_SIMPLE\n#include "lvgl.h"\n#else\n#include "lvgl/lvgl.h"\n#endif'
assert old in s, "lv_font_conv include block changed shape; update fixup_font.py"
s = s.replace(old, '#include <lvgl.h>')

# lv_font_conv derives BOTH the include guard and the font variable from the
# output filename -- the guard uppercased, the variable as written. They collide
# whenever the filename is already all-caps, and then "#define X 1" turns the
# definition below into "const lv_font_t 1 = {". DINish_Medium_32 happens not to
# collide; MPLUS1_JP_20 would. Rename unconditionally rather than depending on
# the case of a filename someone picks later.
stem = p.rsplit('/', 1)[-1][:-2]
guard = stem.upper()
safe = 'FONT_GUARD_' + guard
assert '#define %s 1' % guard in s, "guard macro changed shape; update fixup_font.py"
s = s.replace('#ifndef %s' % guard, '#ifndef %s' % safe)
s = s.replace('#define %s 1' % guard, '#define %s 1' % safe)
s = s.replace('#if %s' % guard, '#if %s' % safe)

open(p, 'w').write(s)
