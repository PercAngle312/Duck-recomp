"""Host models, not Vita hardware emulation. Requires recovered recomp headers."""
import argparse, pathlib, subprocess, tempfile
p=argparse.ArgumentParser();p.add_argument('--headers',type=pathlib.Path,required=True);a=p.parse_args()
r=pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='duck-audio-') as out:
 for test, sources in [('stream',['tests/audio_stream.c']),('backend',['tests/audio_vita.c','src/os_ai.c','src/audio_resample.c']),('ai',['tests/os_ai.c','src/os_ai.c','src/audio_resample.c']),('gap',['tests/audio_gap.c','src/audio_resample.c'])]:
  exe=pathlib.Path(out)/test
  subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Wno-misleading-indentation','-pthread','-I'+str(r/'tests/fake_sdk'),'-I'+str(r/'include'),'-I'+str(a.headers),*[str(r/s) for s in sources],'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True,timeout=30)
