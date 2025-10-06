# Embed logo_png.png into firmware as a C array and auto-include it
import os
from pathlib import Path

def generate_header(input_path: Path, out_header: Path):
    data = input_path.read_bytes()
    with out_header.open('w', newline='\n') as f:
        f.write('#pragma once\n')
        f.write('#include <cstddef>\n')
        f.write('extern const unsigned char g_logo_png[];\n')
        f.write('extern const size_t g_logo_png_len;\n')
    cfile = out_header.with_suffix('.c')
    with cfile.open('w', newline='\n') as f:
        f.write('#include <stddef.h>\n')
        f.write('const unsigned char g_logo_png[] = {\n')
        for i, b in enumerate(data):
            if i % 12 == 0:
                f.write('    ')
            f.write(f'0x{b:02x},')
            if i % 12 == 11:
                f.write('\n')
            else:
                f.write(' ')
        f.write('\n};\n')
        f.write(f'const size_t g_logo_png_len = {len(data)};\n')
from SCons.Script import Import
Import('env')

# Generate at import time so the files exist before compilation
project_dir = Path(env['PROJECT_DIR'])
input_png = project_dir / 'logo_png.png'
if input_png.exists():
    out_dir = project_dir / 'src' / 'generated'
    out_dir.mkdir(parents=True, exist_ok=True)
    header = out_dir / 'logo_png.h'
    generate_header(input_png, header)
    # Ensure compiler can find the header (though src/ is included by default)
    env.Append(CPPPATH=[str(out_dir)])
    print(f'[embed_asset] Embedded {input_png.name} ({input_png.stat().st_size} bytes)')
else:
    print('[embed_asset] logo_png.png not found; skipping embed')
