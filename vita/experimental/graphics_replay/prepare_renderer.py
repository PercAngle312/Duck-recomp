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
    origin=here/'texture-origin.patch'
    subprocess.run(['git','apply','--check',str(origin)],cwd=output,check=True)
    subprocess.run(['git','apply',str(origin)],cwd=output,check=True)
    border=here/'widescreen-border.patch'
    subprocess.run(['git','apply','--check',str(border)],cwd=output,check=True)
    subprocess.run(['git','apply',str(border)],cwd=output,check=True)
    cycle=here/'combiner-cycle.patch'
    subprocess.run(['git','apply','--check',str(cycle)],cwd=output,check=True)
    subprocess.run(['git','apply',str(cycle)],cwd=output,check=True)
    math=here/'triangle-math.patch'
    subprocess.run(['git','apply','--check',str(math)],cwd=output,check=True)
    subprocess.run(['git','apply',str(math)],cwd=output,check=True)
    batch=here/'texture-batching.patch'
    subprocess.run(['git','apply','--check',str(batch)],cwd=output,check=True)
    subprocess.run(['git','apply',str(batch)],cwd=output,check=True)
    draw=here/'draw-batch-uniform.patch'
    subprocess.run(['git','apply','--check',str(draw)],cwd=output,check=True)
    subprocess.run(['git','apply',str(draw)],cwd=output,check=True)
    fence=here/'readback-fence-renderer.patch'
    subprocess.run(['git','apply','--check',str(fence)],cwd=output,check=True)
    subprocess.run(['git','apply',str(fence)],cwd=output,check=True)
    conversion=here/'conversion-cache.patch'
    subprocess.run(['git','apply','--check',str(conversion)],cwd=output,check=True)
    subprocess.run(['git','apply',str(conversion)],cwd=output,check=True)
    scan=here/'command-scan.patch'
    subprocess.run(['git','apply','--check',str(scan)],cwd=output,check=True)
    subprocess.run(['git','apply',str(scan)],cwd=output,check=True)
    native=here/'native-geometry.patch'
    subprocess.run(['git','apply','--check',str(native)],cwd=output,check=True)
    subprocess.run(['git','apply',str(native)],cwd=output,check=True)
    return output/'Source'

if __name__=='__main__':
    import argparse
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('dependency',type=Path);p.add_argument('output',type=Path)
    a=p.parse_args();print(prepare(a.dependency,a.output))
