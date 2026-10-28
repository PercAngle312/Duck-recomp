"""Validate production edge-AA shader under surfaceless Mesa GLES, not Vita GL."""
import ctypes as C,ctypes.util as U,os,re,ast,json
from pathlib import Path
os.environ['EGL_PLATFORM']='surfaceless';os.environ['LIBGL_ALWAYS_SOFTWARE']='1'
e=C.CDLL(U.find_library('EGL'));void=C.c_void_p;integer=C.c_int;uint=C.c_uint
for name,restype,args in [('eglGetDisplay',void,[void]),('eglInitialize',uint,[void,C.POINTER(integer),C.POINTER(integer)]),('eglBindAPI',uint,[uint]),('eglChooseConfig',uint,[void,C.POINTER(integer),C.POINTER(void),integer,C.POINTER(integer)]),('eglCreatePbufferSurface',void,[void,void,C.POINTER(integer)]),('eglCreateContext',void,[void,void,void,C.POINTER(integer)]),('eglMakeCurrent',uint,[void,void,void,void]),('eglGetProcAddress',void,[C.c_char_p])]:
 f=getattr(e,name);f.restype=restype;f.argtypes=args
d=e.eglGetDisplay(None);major=integer();minor=integer();assert e.eglInitialize(d,C.byref(major),C.byref(minor))
assert e.eglBindAPI(0x30A0)
attrs=(integer*15)(0x3033,1,0x3040,4,0x3024,8,0x3023,8,0x3022,8,0x3021,8,0x3038,0,0)
config=void();n=integer();assert e.eglChooseConfig(d,attrs,C.byref(config),1,C.byref(n)) and n.value
surface=e.eglCreatePbufferSurface(d,config,(integer*5)(0x3057,64,0x3056,64,0x3038));ctx=e.eglCreateContext(d,config,None,(integer*3)(0x3098,2,0x3038));assert surface and ctx and e.eglMakeCurrent(d,surface,surface,ctx)
def gl(name,result,*args):
 p=e.eglGetProcAddress(name.encode());assert p,name
 return C.CFUNCTYPE(result,*args)(p)
create=gl('glCreateShader',uint,uint);source=gl('glShaderSource',None,uint,integer,C.POINTER(C.c_char_p),C.POINTER(integer));compile=gl('glCompileShader',None,uint);status=gl('glGetShaderiv',None,uint,uint,C.POINTER(integer));log=gl('glGetShaderInfoLog',None,uint,integer,C.POINTER(integer),void)
s=Path(__file__).with_name('presentation.h').read_text();s=s.split('const char *sources[]={',1)[1].split('};',1)[0]
parts=re.findall(r'"(?:[^"\\]|\\.)*"',s);sources=[ast.literal_eval(parts[0]),''.join(ast.literal_eval(p) for p in parts[1:])]
stages=[]
for kind,text in zip([0x8b31,0x8b30],sources):
 sh=create(kind);p=C.c_char_p(text.encode());source(sh,1,C.byref(p),None);compile(sh);ok=integer();status(sh,0x8b81,C.byref(ok))
 b=C.create_string_buffer(8192);log(sh,len(b),None,b);assert ok.value,b.value.decode();stages.append(sh)
program=gl('glCreateProgram',uint)();attach=gl('glAttachShader',None,uint,uint)
for sh in stages:attach(program,sh)
bind=gl('glBindAttribLocation',None,uint,uint,C.c_char_p);bind(program,0,b'position');bind(program,1,b'texcoord');gl('glLinkProgram',None,uint)(program);ok=integer();gl('glGetProgramiv',None,uint,uint,C.POINTER(integer))(program,0x8b82,C.byref(ok));assert ok.value
use=gl('glUseProgram',None,uint);use(program)
getuniform=gl('glGetUniformLocation',integer,uint,C.c_char_p);gl('glUniform1i',None,integer,integer)(getuniform(program,b'image'),0);gl('glUniform2f',None,integer,C.c_float,C.c_float)(getuniform(program,b'texel'),1/32,1/32)
tex=uint();gl('glGenTextures',None,integer,C.POINTER(uint))(1,C.byref(tex));gl('glBindTexture',None,uint,uint)(0x0de1,tex)
param=gl('glTexParameteri',None,uint,uint,integer)
for name,value in [(0x2801,0x2601),(0x2800,0x2601),(0x2802,0x812f),(0x2803,0x812f)]:param(0xde1,name,value)
vertices=(C.c_float*24)(-1,-1,0,1,1,-1,1,1,-1,1,0,0,-1,1,0,0,1,-1,1,1,1,1,1,0)
attr=gl('glVertexAttribPointer',None,uint,integer,uint,uint,integer,void);enable=gl('glEnableVertexAttribArray',None,uint)
for i,off in [(0,0),(1,8)]:enable(i);attr(i,2,0x1406,0,16,C.cast(C.byref(vertices,off),void))
upload=gl('glTexImage2D',None,uint,integer,integer,integer,integer,integer,uint,uint,void);draw=gl('glDrawArrays',None,uint,integer,integer);read=gl('glReadPixels',None,integer,integer,integer,integer,uint,uint,void)
def render(data):
 raw=C.create_string_buffer(bytes(data));upload(0xde1,0,0x1908,32,32,0,0x1908,0x1401,raw);draw(4,0,6)
 out=C.create_string_buffer(64*64*4);read(0,0,64,64,0x1908,0x1401,out);return out.raw
flat=render(bytes([80,120,160,255])*1024);assert all(abs(flat[i*4+c]-[80,120,160,255][c])<=1 for i in range(4096) for c in range(4))
# Horizontal two-colour source checks the presentation's explicit V inversion.
pattern=bytearray()
for y in range(32):
 for x in range(32):pattern.extend([255,0,0,255] if y<16 else [0,0,255,255])
out=render(pattern);assert out[:3]==bytes([0,0,255]) and out[-4:-1]==bytes([255,0,0])
renderer=gl('glGetString',C.c_char_p,uint)(0x1f01).decode();print(json.dumps({'shader_compiled_and_linked':True,'flat_colour_preserved':True,'source_vertical_orientation_verified':True,'renderer':renderer,'Vita_shader_compiler_tested':False}))
# Compare a diagonal edge with the same shader's AA branch disabled. Only
# controlled fixture source changes; the production shader remains untouched.
diagonal=bytearray()
for y in range(32):
 for x in range(32):diagonal.extend([240,240,240,255] if x>y else [20,20,20,255])
edge=render(diagonal)
bypass=sources[1].replace('if(hi-lo>max(0.03125,hi*0.125))','if(false)')
assert bypass!=sources[1]
sh=create(0x8b30);p=C.c_char_p(bypass.encode());source(sh,1,C.byref(p),None);compile(sh);status(sh,0x8b81,C.byref(ok));assert ok.value
baseline_program=gl('glCreateProgram',uint)();attach(baseline_program,stages[0]);attach(baseline_program,sh);bind(baseline_program,0,b'position');bind(baseline_program,1,b'texcoord');gl('glLinkProgram',None,uint)(baseline_program);gl('glGetProgramiv',None,uint,uint,C.POINTER(integer))(baseline_program,0x8b82,C.byref(ok));assert ok.value;use(baseline_program)
gl('glUniform1i',None,integer,integer)(getuniform(baseline_program,b'image'),0);loc=getuniform(baseline_program,b'texel');gl('glUniform2f',None,integer,C.c_float,C.c_float)(loc,1/32,1/32)
baseline=render(diagonal);changed=sum(edge[i:i+4]!=baseline[i:i+4] for i in range(0,len(edge),4));assert 0<changed<1024
assert gl('glGetError',uint)()==0
print(json.dumps({'diagonal_pixels_changed':changed,'total_pixels':4096,'AA_is_not_passthrough':True,'Vita_image_quality_verified':False}))
