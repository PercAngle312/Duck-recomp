"""Specialize pinned private DSP command prefixes, retaining original tails."""
from pathlib import Path
import hashlib
EXPECTED='34b83278f6ea63b0a8c0b84ccb3794d942e345f7129951108e90b42f5fec9d35'
def patch(source: Path,output: Path):
 if hashlib.sha256(source.read_bytes()).hexdigest()!=EXPECTED:raise ValueError('Unrecognized private DSP source; refusing specialization')
 s=source.read_text()
 edits={
 'L_11AC:':'    duck_audio_clear_prefix(dmem,r1,r2);\n',
 'L_135C:':'    duck_audio_move_prefix(dmem,r2,r3,r1);\n',
 'L_12E4:':'    duck_audio_interleave_prefix(dmem);r1=16;r2+=352;r3+=352;r4+=704;\n',
 'L_1CA8:':'    duck_audio_mix_prefix(dmem,r22,r23,r21,(int16_t)rsp.vpu.r[31].element(5),(int16_t)r26,&rsp.vpu.r[28].u128,&rsp.vpu.r[29].u128);\n',
 }
 # Insert BEFORE loop labels: a loop backedge must not re-run its prefix.
 for anchor,code in edits.items():
  if s.count('\n'+anchor+'\n')!=1:raise ValueError('DSP command anchor mismatch')
  s=s.replace('\n'+anchor+'\n','\n'+code+anchor+'\n')
 output.write_text('#include "duck_vita_audio_commands.h"\n'+s)
