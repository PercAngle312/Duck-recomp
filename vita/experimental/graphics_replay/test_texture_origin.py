"""Compile actual prepared renderer stride selection against one-row fixtures."""
import argparse
from pathlib import Path
import subprocess
import tempfile
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--source',type=Path,required=True)
a=p.parse_args();s=a.source.read_text()
start=s.index('static uint32_t GetEffectiveLineSize(')
end=s.index('\nconst uint8_t* Interpreter::TextureDecodeAddress',start)
with tempfile.TemporaryDirectory() as directory:
    root=Path(directory)
    (root/'texture_origin_generated.h').write_text(s[start:end])
    here=Path(__file__).resolve().parent
    subprocess.run(['c++','-std=c++17','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
                    '-I'+str(root),'-I'+str(here),str(here/'test_texture_origin.cpp'),'-o',str(root/'test')],check=True)
    subprocess.run([str(root/'test')],check=True)
