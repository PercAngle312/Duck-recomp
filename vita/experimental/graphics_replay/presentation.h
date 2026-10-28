#pragma once
#include <vitaGL.h>
#include <cstdio>
#include <cstring>
#include "settings_menu.h"
#include "duck_vita_audio_stats.h"
extern "C" void vglBufferData(GLenum,const GLvoid*);

// Restore all GL state touched by presentation. Fast3D's cached state remains
// accurate after returning; presentation never enters the guest RAM mirror.
struct DuckPresentationState {
    GLint program,buffer,framebuffer,viewport[4],scissor[4],active,texture;
    GLboolean blend,depth,cull,scissored,mask[4];GLfloat clear[4];
    struct Attrib {GLint buffer,size,type,stride,normalized,enabled;void *pointer;} attributes[2];
    DuckPresentationState(){
        glGetIntegerv(GL_CURRENT_PROGRAM,&program);glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&buffer);
        glGetIntegerv(GL_FRAMEBUFFER_BINDING,&framebuffer);glGetIntegerv(GL_VIEWPORT,viewport);
        glGetIntegerv(GL_SCISSOR_BOX,scissor);glGetIntegerv(GL_ACTIVE_TEXTURE,&active);
        glActiveTexture(GL_TEXTURE0);glGetIntegerv(GL_TEXTURE_BINDING_2D,&texture);
        blend=glIsEnabled(GL_BLEND);depth=glIsEnabled(GL_DEPTH_TEST);cull=glIsEnabled(GL_CULL_FACE);
        scissored=glIsEnabled(GL_SCISSOR_TEST);glGetBooleanv(GL_COLOR_WRITEMASK,mask);glGetFloatv(GL_COLOR_CLEAR_VALUE,clear);
        for(unsigned i=0;i<2;i++){
            auto &a=attributes[i];glGetVertexAttribiv(i,GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING,&a.buffer);
            glGetVertexAttribiv(i,GL_VERTEX_ATTRIB_ARRAY_SIZE,&a.size);glGetVertexAttribiv(i,GL_VERTEX_ATTRIB_ARRAY_TYPE,&a.type);
            glGetVertexAttribiv(i,GL_VERTEX_ATTRIB_ARRAY_STRIDE,&a.stride);glGetVertexAttribiv(i,GL_VERTEX_ATTRIB_ARRAY_NORMALIZED,&a.normalized);
            glGetVertexAttribiv(i,GL_VERTEX_ATTRIB_ARRAY_ENABLED,&a.enabled);glGetVertexAttribPointerv(i,GL_VERTEX_ATTRIB_ARRAY_POINTER,&a.pointer);
        }
    }
    static void enable(GLenum cap,bool on){if(on)glEnable(cap);else glDisable(cap);}
    ~DuckPresentationState(){
        for(unsigned i=0;i<2;i++){
            const auto &a=attributes[i];glBindBuffer(GL_ARRAY_BUFFER,a.buffer);
            glVertexAttribPointer(i,a.size,a.type,a.normalized,a.stride,a.pointer);enableAttrib(i,a.enabled);
        }
        glBindBuffer(GL_ARRAY_BUFFER,buffer);glUseProgram(program);glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
        glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);glScissor(scissor[0],scissor[1],scissor[2],scissor[3]);
        enable(GL_BLEND,blend);enable(GL_DEPTH_TEST,depth);enable(GL_CULL_FACE,cull);enable(GL_SCISSOR_TEST,scissored);
        glColorMask(mask[0],mask[1],mask[2],mask[3]);glClearColor(clear[0],clear[1],clear[2],clear[3]);
        glBindTexture(GL_TEXTURE_2D,texture);glActiveTexture(active);
    }
    static void enableAttrib(unsigned i,bool on){if(on)glEnableVertexAttribArray(i);else glDisableVertexAttribArray(i);}
};

struct DuckEdgeAA {
    GLuint program=0,vbo=0;GLint image=-1,step=-1;bool attempted=false;
    bool initialize(){
        if(attempted)return program!=0;attempted=true;
        const char *sources[]={
            "attribute vec2 position;attribute vec2 texcoord;varying vec2 uv;void main(){uv=texcoord;gl_Position=vec4(position,0.0,1.0);}",
            "precision mediump float;uniform sampler2D image;uniform vec2 texel;varying vec2 uv;"
            "vec3 sampleAt(vec2 p){return texture2D(image,clamp(p,texel*0.5,vec2(1.0)-texel*0.5)).rgb;}"
            "float luma(vec3 c){return dot(c,vec3(0.299,0.587,0.114));}"
            "void main(){vec3 c=sampleAt(uv);float lc=luma(c);"
            "float n=luma(sampleAt(uv+vec2(0.0,-texel.y)));float s=luma(sampleAt(uv+vec2(0.0,texel.y)));"
            "float e=luma(sampleAt(uv+vec2(texel.x,0.0)));float w=luma(sampleAt(uv+vec2(-texel.x,0.0)));"
            "float lo=min(lc,min(min(n,s),min(e,w)));float hi=max(lc,max(max(n,s),max(e,w)));"
            "vec3 result=c;if(hi-lo>max(0.03125,hi*0.125)){vec2 dir=vec2(-(n-s),e-w);"
            "float reduce=max((n+s+e+w)*0.03125,0.0078125);dir=clamp(dir/(min(abs(dir.x),abs(dir.y))+reduce),-4.0,4.0)*texel;"
            "vec3 a=0.5*(sampleAt(uv-dir/6.0)+sampleAt(uv+dir/6.0));"
            "vec3 b=0.5*a+0.25*(sampleAt(uv-dir*0.5)+sampleAt(uv+dir*0.5));float lb=luma(b);"
            "result=(lb<lo||lb>hi)?a:b;}gl_FragColor=vec4(result,1.0);}"};
        GLuint stages[2]={};GLuint candidate=glCreateProgram();bool ok=candidate!=0;
        for(unsigned i=0;i<2&&ok;i++){
            stages[i]=glCreateShader(i?GL_FRAGMENT_SHADER:GL_VERTEX_SHADER);glShaderSource(stages[i],1,&sources[i],nullptr);
            glCompileShader(stages[i]);GLint compiled=0;glGetShaderiv(stages[i],GL_COMPILE_STATUS,&compiled);
            if(!compiled){char log[1024]={};glGetShaderInfoLog(stages[i],sizeof log,nullptr,log);std::fprintf(stderr,"Edge AA shader unavailable: %s\n",log);ok=false;}
            else glAttachShader(candidate,stages[i]);
        }
        if(ok){glBindAttribLocation(candidate,0,"position");glBindAttribLocation(candidate,1,"texcoord");glLinkProgram(candidate);
            GLint linked=0;glGetProgramiv(candidate,GL_LINK_STATUS,&linked);ok=linked!=0;
            if(!ok){char log[1024]={};glGetProgramInfoLog(candidate,sizeof log,nullptr,log);std::fprintf(stderr,"Edge AA link failed: %s\n",log);}}
        for(auto stage:stages)if(stage)glDeleteShader(stage);
        if(!ok){if(candidate)glDeleteProgram(candidate);return false;}
        program=candidate;image=glGetUniformLocation(program,"image");step=glGetUniformLocation(program,"texel");glGenBuffers(1,&vbo);
        std::fprintf(stderr,"Edge AA presentation shader linked; visual validation still required\n");return true;
    }
    bool draw(GLuint texture,unsigned width,unsigned height,unsigned x,unsigned displayWidth,unsigned displayHeight){
        DuckPresentationState saved;
        if(!initialize()||!width||!height)return false;
        // Fast3D game FBO is inverted relative to the display: preserve the
        // same vertical orientation as its ordinary CopyFramebuffer path.
        const float vertices[]={-1,-1,0,1, 1,-1,1,1, -1,1,0,0, -1,1,0,0, 1,-1,1,1, 1,1,1,0};
        void *snapshot=vglAllocFromScratch(sizeof vertices);if(!snapshot)return false;std::memcpy(snapshot,vertices,sizeof vertices);
        glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(x,0,displayWidth,displayHeight);
        glDisable(GL_BLEND);glDisable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);glDisable(GL_SCISSOR_TEST);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
        glUseProgram(program);glBindBuffer(GL_ARRAY_BUFFER,vbo);vglBufferData(GL_ARRAY_BUFFER,snapshot);
        glEnableVertexAttribArray(0);glEnableVertexAttribArray(1);
        glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),nullptr);glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float)));
        glBindTexture(GL_TEXTURE_2D,texture);glUniform1i(image,0);glUniform2f(step,1.0f/width,1.0f/height);glDrawArrays(GL_TRIANGLES,0,6);
        return true;
    }
};

inline unsigned duck_font_row(char c,unsigned row){
    static const uint8_t letters[26][7]={
        {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},{30,17,17,17,17,17,30},
        {31,16,16,30,16,16,31},{31,16,16,30,16,16,16},{14,17,16,23,17,17,15},{17,17,17,31,17,17,17},
        {14,4,4,4,4,4,14},{7,2,2,2,2,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
        {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},{30,17,17,30,16,16,16},
        {14,17,17,17,21,18,13},{30,17,17,30,20,18,17},{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},
        {17,17,17,17,17,17,14},{17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
        {17,17,10,4,4,4,4},{31,1,2,4,8,16,31}};
    static const uint8_t digits[10][7]={{14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
        {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},{14,16,16,30,17,17,14},
        {31,1,2,4,8,8,8},{14,17,17,14,17,17,14},{14,17,17,15,1,1,14}};
    if(c>='a'&&c<='z')c-=32;
    if(c>='A'&&c<='Z')return letters[c-'A'][row];if(c>='0'&&c<='9')return digits[c-'0'][row];
    if(c=='-')return row==3?14:0;if(c==':')return row==2||row==4?4:0;
    if(c=='/')return row<5?1u<<(4-row):0;return 0;
}
inline void duck_menu_rect(int x,int y,int w,int h,float r,float g,float b){
    glScissor(x,544-y-h,w,h);glClearColor(r,g,b,1);glClear(GL_COLOR_BUFFER_BIT);
}
inline void duck_menu_text(int x,int y,const char *s,unsigned scale=3){
    for(;*s;s++,x+=6*scale)for(unsigned row=0;row<7;row++){
        unsigned bits=duck_font_row(*s,row);
        for(unsigned col=0;col<5;){
            if(!(bits&(16u>>col))){col++;continue;}unsigned begin=col;
            while(col<5&&(bits&(16u>>col)))col++;
            duck_menu_rect(x+begin*scale,y+row*scale,(col-begin)*scale,scale,0.90f,0.94f,1.0f);
        }
    }
}
inline void duck_draw_settings(const DuckSettingsMenu &menu,bool failed){
    DuckPresentationState saved;glBindFramebuffer(GL_FRAMEBUFFER,0);glEnable(GL_SCISSOR_TEST);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    duck_menu_rect(0,0,960,544,0.025f,0.035f,0.065f);duck_menu_text(72,36,"DUCK DODGERS SETTINGS",4);
    char lines[8][80];const auto &o=menu.pending;
    std::snprintf(lines[0],80,"RESOLUTION: %uX%u - %s",o.width,o.height,o.quality==0?"NATIVE":o.quality==1?"2X":"DISPLAY");
    std::snprintf(lines[1],80,"ANTIALIASING: %s",o.antialias?"EDGE AA":"OFF");
    std::snprintf(lines[2],80,"TEXTURE FILTER: %s",o.filter==0?"N64":o.filter==1?"BILINEAR":"NEAREST");
    std::snprintf(lines[3],80,"ASPECT: %s",o.widescreen?"WIDESCREEN":"4:3");
    std::snprintf(lines[4],80,"SCALING: %s",o.smooth?"SMOOTH":"SHARP");
    std::snprintf(lines[5],80,"LIVE STATS: %s",o.live_stats?"ON":"OFF");
    std::snprintf(lines[6],80,"APPLY AND SAVE");std::snprintf(lines[7],80,"CANCEL");
    for(unsigned row=0;row<8;row++){
        if(menu.row==row)duck_menu_rect(60,100+row*38,840,34,0.08f,0.24f,0.35f);
        duck_menu_text(78,107+row*38,lines[row]);
    }
    duck_menu_text(72,430,failed?"SAVE FAILED - CHANGES NOT APPLIED":"HIGHER RESOLUTION USES MORE GPU TIME",2);
    duck_menu_text(72,468,"D-PAD: CHANGE   CROSS: CHOOSE   CIRCLE: BACK",2);
    duck_menu_text(72,500,"HOLD SELECT TO OPEN - GUEST PAUSED",2);
}

// One batched draw for both backdrop and bitmap text, after guest readback.
struct DuckFpsOverlay {
    GLuint program=0,vbo=0;bool attempted=false;
    bool initialize(){
        if(attempted)return program!=0;attempted=true;
        const char *sources[]={
            "attribute vec2 position;attribute vec3 colour;varying vec3 rgb;void main(){gl_Position=vec4(position,0.0,1.0);rgb=colour;}",
            "precision mediump float;varying vec3 rgb;void main(){gl_FragColor=vec4(rgb,1.0);}"};
        GLuint candidate=glCreateProgram(),stages[2]={};bool ok=candidate!=0;
        for(unsigned i=0;i<2&&ok;i++){
            stages[i]=glCreateShader(i?GL_FRAGMENT_SHADER:GL_VERTEX_SHADER);glShaderSource(stages[i],1,&sources[i],nullptr);
            glCompileShader(stages[i]);GLint status=0;glGetShaderiv(stages[i],GL_COMPILE_STATUS,&status);ok=status!=0;
            if(ok)glAttachShader(candidate,stages[i]);
        }
        if(ok){glBindAttribLocation(candidate,0,"position");glBindAttribLocation(candidate,1,"colour");glLinkProgram(candidate);
            GLint status=0;glGetProgramiv(candidate,GL_LINK_STATUS,&status);ok=status!=0;}
        for(auto s:stages)if(s)glDeleteShader(s);
        if(!ok){if(candidate)glDeleteProgram(candidate);std::fprintf(stderr,"FPS overlay shader unavailable\n");return false;}
        program=candidate;glGenBuffers(1,&vbo);return true;
    }
    void draw(unsigned fps,unsigned frame_us,bool ready,unsigned width,unsigned height){
        DuckPresentationState saved;if(!initialize())return;
        DuckVitaAudioStats audio={};duck_vita_audio_stats(&audio);
        char lines[6][32];
        if(ready){std::snprintf(lines[0],32,"FPS %u",fps);std::snprintf(lines[1],32,"FRAME %u MS",(frame_us+500)/1000);}
        else {std::snprintf(lines[0],32,"FPS --");std::snprintf(lines[1],32,"FRAME -- MS");}
        std::snprintf(lines[2],32,"RENDER %uX%u",width,height);
        std::snprintf(lines[3],32,"AUDIO BUFFER %u MS",audio.queued_frames/48);
        std::snprintf(lines[4],32,"UNDERRUNS %u",audio.underrun_blocks);
        std::snprintf(lines[5],32,"SILENCE %u MS",audio.padded_frames/48);
        unsigned longest=0,runs=1;
        for(auto &line:lines){unsigned length=unsigned(std::strlen(line));if(length>longest)longest=length;
            for(unsigned c=0;c<length;c++)for(unsigned row=0;row<7;row++){
                unsigned bits=duck_font_row(line[c],row);
                for(unsigned col=0;col<5;){if(!(bits&(16u>>col))){col++;continue;}runs++;while(col<5&&(bits&(16u>>col)))col++;}
            }
        }
        float *vertices=(float*)vglAllocFromScratch(runs*6*5*sizeof(float));if(!vertices)return;
        unsigned used=0;
        auto rect=[&](float x,float y,float w,float h,float colour){
            const float points[12]={x,y,x+w,y,x,y+h,x,y+h,x+w,y,x+w,y+h};
            for(unsigned i=0;i<6;i++){
                float *v=vertices+(used++)*5;v[0]=points[i*2]/480.0f-1.0f;v[1]=1.0f-points[i*2+1]/272.0f;
                v[2]=v[3]=v[4]=colour;
            }
        };
        unsigned x=960-12-longest*12;
        rect(float(x-5),7,float(longest*12+10),6*18+10,0.03f);
        for(unsigned line=0;line<6;line++)for(unsigned c=0;c<std::strlen(lines[line]);c++)for(unsigned row=0;row<7;row++){
            unsigned bits=duck_font_row(lines[line][c],row);
            for(unsigned col=0;col<5;){
                if(!(bits&(16u>>col))){col++;continue;}unsigned begin=col;
                while(col<5&&(bits&(16u>>col)))col++;
                rect(float(x+c*12+begin*2),float(12+line*18+row*2),float((col-begin)*2),2,1.0f);
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,960,544);
        glDisable(GL_BLEND);glDisable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);glDisable(GL_SCISSOR_TEST);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
        glUseProgram(program);glBindBuffer(GL_ARRAY_BUFFER,vbo);vglBufferData(GL_ARRAY_BUFFER,vertices);
        glEnableVertexAttribArray(0);glEnableVertexAttribArray(1);
        glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,5*sizeof(float),nullptr);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,5*sizeof(float),(void*)(2*sizeof(float)));
        glDrawArrays(GL_TRIANGLES,0,used);
    }
};
