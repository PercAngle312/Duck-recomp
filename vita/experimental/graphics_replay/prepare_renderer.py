"""Apply reviewed integration fixes to a build copy of the pinned renderer.

Original dependency checkout is preserved. Callers add returned Source and its
Fast3D/include before the dependency include paths and compile all renderer TUs
against those same headers.
"""
from pathlib import Path
import shutil, subprocess

def prepare(dependency: Path, output: Path) -> Path:
    dependency=dependency.resolve(); output=output.resolve()
    here=Path(__file__).resolve().parent
    for protected in (dependency, here.parents[2]):
        if output==protected or protected in output.parents:
            raise ValueError('Renderer build copies must remain outside source repositories')
        if output in protected.parents:
            raise ValueError('Renderer output may not contain a source repository')
    source=Path('Source/SysVita/HLEGraphics/Accurate')
    output.mkdir(parents=True,exist_ok=True)
    shutil.copytree(dependency/source, output/source, dirs_exist_ok=True)
    patch=here/'renderer.patch'
    subprocess.run(['git','apply','--check',str(patch)],cwd=output,check=True)
    subprocess.run(['git','apply',str(patch)],cwd=output,check=True)
    options=here/'graphics-options.patch'
    subprocess.run(['git','apply','--check',str(options)],cwd=output,check=True)
    subprocess.run(['git','apply',str(options)],cwd=output,check=True)
    validation=here/'texture-validation.patch'
    subprocess.run(['git','apply','--check',str(validation)],cwd=output,check=True)
    subprocess.run(['git','apply',str(validation)],cwd=output,check=True)
    return output/'Source'

if __name__=='__main__':
    import argparse
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('dependency',type=Path);p.add_argument('output',type=Path)
    a=p.parse_args();print(prepare(a.dependency,a.output))
