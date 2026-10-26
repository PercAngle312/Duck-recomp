"""Link a private translated Duck Dodgers Vita executable (never publish assets).

This final-link stage accepts real private ARM EABI5 objects and the actual
Fast3D renderer objects, and refuses to produce a game ELF when either is absent.
It intentionally does not invent a game main() or use the link-only probe entry.
"""
import argparse
from pathlib import Path
import subprocess

p = argparse.ArgumentParser(description=__doc__)
p.add_argument("--vitasdk", type=Path, required=True)
p.add_argument("--runtime", type=Path, required=True,
               help="Linked 12-module ARM32 runtime object")
p.add_argument("--game-entry", type=Path, required=True,
               help="Private ARM32 entrypoint that starts the guest scheduler")
p.add_argument("--game-dir", type=Path, required=True,
               help="Private translated game ARM objects")
p.add_argument("--renderer-dir", type=Path, required=True,
               help="Fast3D/vitaGL ARM objects")
p.add_argument("--output-dir", type=Path, required=True)
a = p.parse_args()

sdk=a.vitasdk.resolve()
cc=sdk/"bin/arm-vita-eabi-g++"
nm=sdk/"bin/arm-vita-eabi-nm"
for f in (cc,nm,a.runtime,a.game_entry):
    if not f.is_file(): p.error(f"Missing required input: {f}")
game=sorted(a.game_dir.glob("*.o"))
renderer=sorted(a.renderer_dir.glob("*.o"))
if not game: p.error("No private translated game objects; refusing probe-only link")
if not renderer: p.error("No real renderer objects; refusing runtime-only link")
out=a.output_dir.resolve()
if out == Path.cwd().resolve() or Path.cwd().resolve() in out.parents:
    p.error("Private output must be outside the checked-out public repository")
out.mkdir(parents=True,exist_ok=True)
elf=out/"DuckDodgers-Vita.private.elf"
cmd=[str(cc),"-Wl,--no-undefined",str(a.game_entry),str(a.runtime),
     *map(str,game),*map(str,renderer),"-lm","-lstdc++","-o",str(elf)]
with (out/"link-command.txt").open("w") as log: log.write(" ".join(cmd)+"\n")
with (out/"link.log").open("w") as log:
    result=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT)
if result.returncode:
    elf.unlink(missing_ok=True)
    raise SystemExit(f"Private game link failed; see {out/'link.log'}")
symbols=subprocess.check_output([str(nm),"-u",str(elf)],text=True)
(out/"undefined.txt").write_text(symbols)
strong=[line for line in symbols.splitlines() if line.split()[:1]==["U"]]
if strong:
    elf.unlink(missing_ok=True)
    raise SystemExit(f"Refusing incomplete ELF: {len(strong)} strong imports")
print(f"Private full game ELF linked at {elf}. Not yet packaged or gameplay-verified.")
