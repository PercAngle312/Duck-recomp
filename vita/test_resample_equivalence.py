"""Compare production resampler with the original scalar implementation."""
import argparse,pathlib,subprocess,tempfile
p=argparse.ArgumentParser();p.add_argument('--headers',type=pathlib.Path,required=True);a=p.parse_args()
r=pathlib.Path(__file__).resolve().parent
original=subprocess.check_output(['git','show','f7d757a8d2a037e0fe72cae5ce539bea9bf20fc2:vita/src/audio_resample.c'],cwd=r,text=True)
original=original.replace('duck_vita_audio_resample_joined','reference_joined').replace('duck_vita_audio_resample','reference_resample').replace('duck_vita_audio_gap_fade','reference_gap')
with tempfile.TemporaryDirectory(prefix='duck-resample-') as tmp:
 t=pathlib.Path(tmp);ref=t/'reference.c';ref.write_text(original);exe=t/'compare'
 subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-fsanitize=undefined,address','-I'+str(a.headers),'-I'+str(r/'include'),str(ref),str(r/'src/audio_resample.c'),str(r/'tests/resample_equivalence.c'),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True,timeout=60)
