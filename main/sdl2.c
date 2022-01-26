
#include <SDL2/SDL.h>

#include <exnes.h>
#include <cpu6502.h>
#include <ppu.h>
/*
gcc  sdl2.c -lSDL2 -I../include -D _DEBUG_ -g -o exnes.exe
*/

typedef struct{
    SDL_Window*window;
    SDL_Renderer*renderer;
    SDL_Event event;
    SDL_Texture *screen;
    int quit;
    SDL_Rect screen_rect;
    uint16_t screen_data[512*512];
}sdl2_t;

sdl2_t win;

#include <stdio.h>

static void* fdata(const char*path,int *size){
    int sz = 0;
    FILE*file = fopen(path,"rb");
    fseek(file,0,SEEK_END);
    sz = ftell(file);
    if(size) *size = sz;
    fseek(file,0,SEEK_SET);
    void* d = calloc(1,sz);
    fread(d,sz,1,file);
    fclose(file);
    return d;
}

void init(){
    win.screen_rect.x = 0;
    win.screen_rect.y = 0;
    win.screen_rect.w = 256;
    win.screen_rect.h = 240;
    win.window = SDL_CreateWindow("nes",100,100,800,480,SDL_WINDOW_SHOWN|SDL_WINDOW_OPENGL);
    win.renderer = SDL_CreateRenderer(win.window,2,SDL_RENDERER_PRESENTVSYNC);
    win.screen = SDL_CreateTexture(win.renderer,SDL_PIXELFORMAT_BGR555,SDL_TEXTUREACCESS_STREAMING,win.screen_rect.w,win.screen_rect.h);
}

void* get_pixels_line(struct exnes_t*nes,int line){
    /*获得目标数据*/
    return win.screen_data + line * win.screen_rect.w;
}

static int render(exnes_t*nes){
    SDL_Rect rect;
    memcpy(&rect,&win.screen_rect,sizeof(rect));
    rect.x = 0;rect.y = 0;

    /*更新数据*/
    SDL_UpdateTexture(win.screen,&rect,win.screen_data,2*rect.w);
    SDL_RenderCopy(win.renderer,win.screen,0,&win.screen_rect);
    SDL_RenderPresent(win.renderer);
    return 0;
}

static int process_other(exnes_t*nes){
    SDL_PollEvent(&win.event);
    if(win.event.type==SDL_QUIT){
        exit(0);
    }
    const u8 *key = SDL_GetKeyboardState(0);
    u8 *P1 = &nes->input[0];
    *P1 = 0;
    *P1 |= key[SDL_SCANCODE_DOWN  ]?exnes_input_Down :0;
    *P1 |= key[SDL_SCANCODE_UP    ]?exnes_input_Up   :0;
    *P1 |= key[SDL_SCANCODE_RIGHT ]?exnes_input_Right:0;
    *P1 |= key[SDL_SCANCODE_LEFT  ]?exnes_input_Left :0;
    *P1 |= key[SDL_SCANCODE_RETURN]?exnes_input_start:0;
    *P1 |= key[SDL_SCANCODE_Z     ]?exnes_input_A    :0;
    *P1 |= key[SDL_SCANCODE_X     ]?exnes_input_B    :0;
    *P1 |= key[SDL_SCANCODE_SPACE ]?exnes_input_select:0;

    return 0;
}

unsigned char* data = 0;
void init_nes(){
    if(data){
        exnes_t raw_nes;
        exnes_t *nes = &raw_nes;
        memset(nes,0,sizeof(*nes));
        exnes_init_rom(nes,data);
        nes->render = render;
        nes->get_pixels_line = get_pixels_line;
        nes->err_data = stderr;
        nes->errorf = (void*)fprintf;
        nes->process_other = process_other;
        exnes_exec(nes);
    }
    else{
        win.quit = 1;
    }
}

void loop(){
    SDL_Rect rect;

    while(!win.quit){
        SDL_RenderClear(win.renderer);
        SDL_RenderCopy(win.renderer,win.screen,0,&win.screen_rect);
        SDL_PollEvent(&win.event);
        if(win.event.type==SDL_QUIT){
            win.quit = 1;
        }
        SDL_RenderPresent(win.renderer);
    }
}

#undef main
int main(int argc,char**argv){
    int sz;
    data = fdata(argv[1],&sz);
    if(data==0){
        fprintf(stderr,"open file error:%s\n",argv[1]);
        exit(-1);
    }
    init();
    init_nes();
    loop();
}

