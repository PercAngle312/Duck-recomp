"""Verify generated private-registry linkage with synthetic functions only."""
import argparse
import subprocess
import tempfile
from pathlib import Path
from generate_dispatch import generate

p = argparse.ArgumentParser()
p.add_argument("--recomp-include", type=Path, required=True)
a = p.parse_args()
root = Path(__file__).resolve().parent
with tempfile.TemporaryDirectory() as td:
    temp = Path(td)
    mapping = temp / "map.csv"
    output = temp / "registry.c"
    mapping.write_text("vram,symbol\n0x80002000,synthetic_second\n0x80001000,synthetic_first\n")
    assert generate(mapping, output) == 2
    (temp / "stub.c").write_text('''
#include "duck_vita_dispatch.h"
void synthetic_first(uint8_t *ram, recomp_context *ctx) { ctx->r1 += ram[0]; }
void synthetic_second(uint8_t *ram, recomp_context *ctx) { ctx->r1 += 2 * ram[0]; }
extern const DuckVitaDispatchTable duck_vita_game_dispatch;
int main(void) {
    uint8_t memory[1]={3};
    recomp_context cpu={0};
    if(duck_vita_dispatch_validate(&duck_vita_game_dispatch)) return 1;
    if(duck_vita_game_dispatch.count!=2) return 2;
    duck_vita_game_dispatch.entries[0].function(memory,&cpu);
    duck_vita_game_dispatch.entries[1].function(memory,&cpu);
    return cpu.r1==9 ? 0 : 3;
}
''')
    subprocess.run(["cc","-std=c11","-Wall","-Wextra","-Werror",
                    "-I",str(a.recomp_include),"-I",str(root/"include"),
                    str(output),str(temp/"stub.c"),str(root/"src/dispatch.c"),
                    str(root/"src/recomp_bridge.c"),str(root/"src/scheduler.c"),
                    str(root/"src/queue.c"),"-lm","-o",str(temp/"registry-test")],
                   check=True)
    subprocess.run([str(temp/"registry-test")],check=True)
    print("PASS: synthetic generated registry compiled, linked and invoked with pinned ABI")
