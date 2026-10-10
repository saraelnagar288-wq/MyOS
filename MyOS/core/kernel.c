#include <stdint.h>
#include "memory.h"
#define MULTIBOOT_MAGIC 0x2BADB002u
#define FB_FLAG (1u << 12)

struct multiboot_info {
 uint32_t flags,mem_lower,mem_upper,boot_device,cmdline,mods_count,mods_addr,syms[4],mmap_length,mmap_addr,drives_length,drives_addr,config_table,boot_loader_name,apm_table,vbe_control_info,vbe_mode_info;
 uint16_t vbe_mode,vbe_interface_seg,vbe_interface_off,vbe_interface_len;
 uint64_t framebuffer_addr;
 uint32_t framebuffer_pitch,framebuffer_width,framebuffer_height;
 uint8_t framebuffer_bpp,framebuffer_type,color_info[6];
} __attribute__((packed));

static uint32_t *fb; static uint32_t pitch,sw,sh; static uint8_t launcher_open;

static void outb(uint16_t p,uint8_t v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static uint8_t inb(uint16_t p){uint8_t v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
static void serial_init(void){outb(0x3F9,0);outb(0x3FB,0x80);outb(0x3F8,3);outb(0x3F9,0);outb(0x3FB,3);outb(0x3FC,3);outb(0x3FA,0xC7);}
static void serial_puts(const char*s){while(*s)outb(0x3F8,(uint8_t)*s++);}
static void halt(void){for(;;)__asm__ volatile("hlt");}

static volatile uint16_t *vga=(volatile uint16_t*)0xB8000;
static void vga_msg(const char*s,uint8_t color){uint32_t i=0;while(*s&&i<80*25)vga[i++]=((uint16_t)color<<8)|(uint8_t)*s++;}
static void rect(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint32_t c){if(!fb||x>=sw||y>=sh)return;if(x+w>sw)w=sw-x;if(y+h>sh)h=sh-y;for(uint32_t yy=y;yy<y+h;yy++)for(uint32_t xx=x;xx<x+w;xx++)fb[yy*(pitch/4)+xx]=c;}
static void line(uint32_t x,uint32_t y,uint32_t w,uint32_t c){rect(x,y,w,1,c);}
static void frame(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint32_t c){line(x,y,w,c);line(x,y+h-1,w,c);rect(x,y,1,h,c);rect(x+w-1,y,1,h,c);}
static const uint8_t font8x8[96][8]={
#include "font8x8.inc"
};
static void ch(uint32_t x,uint32_t y,int v,uint32_t c){if(v<32||v>127)v='?';const uint8_t*g=font8x8[(uint8_t)v-32];for(uint32_t r=0;r<8;r++)for(uint32_t k=0;k<8;k++)if(g[r]&(1u<<(7-k)))rect(x+k,y+r,1,1,c);}
static void text(uint32_t x,uint32_t y,const char*s,uint32_t c){while(*s){if(*s=='\n'){y+=10;x=32;}else{ch(x,y,(unsigned char)*s,c);x+=8;}s++;}}
static void grid(uint32_t x,uint32_t y,uint32_t c){rect(x,y,7,7,c);rect(x+10,y,7,7,c);rect(x,y+10,7,7,c);rect(x+10,y+10,7,7,c);}
static void folder(uint32_t x,uint32_t y){rect(x,y+5,25,17,0xFFF4C95D);rect(x+3,y+2,11,6,0xFFFFDD8A);}
static void terminal(uint32_t x,uint32_t y){frame(x,y,27,22,0xFFB7C3D0);rect(x+5,y+9,5,2,0xFF5EEAD4);rect(x+12,y+9,8,2,0xFF5EEAD4);}
static void settings(uint32_t x,uint32_t y){rect(x+9,y,9,24,0xFFB8C2D1);rect(x,y+8,27,9,0xFFB8C2D1);rect(x+10,y+10,7,7,0xFF151A23);}
static void window(uint32_t x,uint32_t y,uint32_t w,uint32_t h,const char*t,uint32_t a){rect(x+7,y+7,w,h,0x55000000);rect(x,y,w,h,0xFF182130);frame(x,y,w,h,0xFF354155);rect(x,y,w,38,0xFF202A3A);rect(x,y,4,38,a);text(x+18,y+15,t,0xFFFFFFFF);rect(x+w-72,y+14,8,8,0xFF8794A8);rect(x+w-50,y+14,8,8,0xFF8794A8);rect(x+w-28,y+14,8,8,0xFFF87171);}
static void draw_launcher(void){uint32_t w=sw>850?560:sw-80,h=sh>650?430:sh-150,x=(sw-w)/2,y=(sh-h)/2;rect(x+8,y+8,w,h,0x55000000);rect(x,y,w,h,0xFF141B28);frame(x,y,w,h,0xFF40506A);rect(x,y,w,62,0xFF1E2736);text(x+24,y+22,"Search MyOS",0xFF8B98AB);line(x+24,y+48,w-48,0xFF344155);text(x+24,y+86,"Applications",0xFF8B98AB);rect(x+24,y+112,150,88,0xFF202A3A);folder(x+48,y+132);text(x+48,y+166,"Files",0xFFFFFFFF);rect(x+192,y+112,150,88,0xFF202A3A);terminal(x+214,y+132);text(x+214,y+166,"Terminal",0xFFFFFFFF);rect(x+360,y+112,150,88,0xFF202A3A);settings(x+382,y+132);text(x+382,y+166,"Settings",0xFFFFFFFF);text(x+24,y+238,"System",0xFF8B98AB);text(x+24,y+268,"Display",0xFFD5DCE7);text(x+24,y+290,"Network",0xFFD5DCE7);text(x+24,y+312,"Appearance",0xFFD5DCE7);text(x+24,y+356,"Press ESC to return to desktop",0xFF6EE7B7);}
static int mouse_x,mouse_y,mouse_cycle,mouse_buttons,cursor_visible;
static uint8_t mouse_packet[3]; static uint32_t cursor_saved[20*20];
static int ps2_wait_write(void){for(uint32_t i=0;i<100000;i++)if(!(inb(0x64)&2))return 1;return 0;}
static int ps2_wait_read(void){for(uint32_t i=0;i<100000;i++)if(inb(0x64)&1)return 1;return 0;}
static void mouse_write(uint8_t v){if(!ps2_wait_write())return;outb(0x64,0xD4);if(!ps2_wait_write())return;outb(0x60,v);}
static uint8_t mouse_read_timeout(void){return ps2_wait_read()?inb(0x60):0;}
static void mouse_init(void){
 if(!ps2_wait_write())return;
 outb(0x64,0xA8);
 if(!ps2_wait_write())return;
 outb(0x64,0x20);
 uint8_t cfg=mouse_read_timeout();
 cfg=(uint8_t)((cfg|2u)&(uint8_t)~0x20u);
 if(ps2_wait_write())outb(0x64,0x60);
 if(ps2_wait_write())outb(0x60,cfg);
 mouse_write(0xF6);(void)mouse_read_timeout();
 mouse_write(0xF4);(void)mouse_read_timeout();
}
static void cursor_save(int x,int y){for(int yy=0;yy<20;yy++)for(int xx=0;xx<20;xx++){int px=x+xx,py=y+yy;cursor_saved[yy*20+xx]=(px>=0&&py>=0&&(uint32_t)px<sw&&(uint32_t)py<sh)?fb[py*(pitch/4)+px]:0;}}
static void cursor_restore(int x,int y){for(int yy=0;yy<20;yy++)for(int xx=0;xx<20;xx++){int px=x+xx,py=y+yy;if(px>=0&&py>=0&&(uint32_t)px<sw&&(uint32_t)py<sh)fb[py*(pitch/4)+px]=cursor_saved[yy*20+xx];}}
static void cursor_draw(int x,int y){cursor_save(x,y);for(int i=0;i<18;i++){int px=x+i,py=y+i;if(px>=0&&py>=0&&(uint32_t)px<sw&&(uint32_t)py<sh)fb[py*(pitch/4)+px]=0xFFFFFFFF;}for(int i=0;i<12;i++){int px=x+5+i,py=y+14;if(px>=0&&py>=0&&(uint32_t)px<sw&&(uint32_t)py<sh)fb[py*(pitch/4)+px]=0xFFFFFFFF;}cursor_visible=1;}
static void cursor_hide(void){if(cursor_visible){cursor_restore(mouse_x,mouse_y);cursor_visible=0;}}
static void cursor_move(int nx,int ny){if(nx<0)nx=0;if(ny<0)ny=0;if(nx>(int)sw-20)nx=(int)sw-20;if(ny>(int)sh-20)ny=(int)sh-20;if(nx==mouse_x&&ny==mouse_y)return;cursor_hide();mouse_x=nx;mouse_y=ny;cursor_draw(mouse_x,mouse_y);}
static void mouse_poll(void){while(inb(0x64)&1){uint8_t st=inb(0x64),d=inb(0x60);if(!(st&0x20))continue;if(mouse_cycle==0){if(!(d&8))continue;mouse_packet[0]=d;mouse_cycle=1;}else if(mouse_cycle==1){mouse_packet[1]=d;mouse_cycle=2;}else{mouse_packet[2]=d;mouse_cycle=0;int dx=(int8_t)mouse_packet[1],dy=(int8_t)mouse_packet[2];cursor_move(mouse_x+dx*2,mouse_y-dy*2);uint8_t previous=mouse_buttons;mouse_buttons=mouse_packet[0]&7;if((mouse_buttons&1)&&!(previous&1)){cursor_hide();handle_click(mouse_x,mouse_y);desktop();cursor_draw(mouse_x,mouse_y);}}}}
static uint8_t show_files=1,show_editor=1,show_settings=1,show_terminal=1,show_calculator=1;
static uint8_t last_left_button=0;
static uint32_t top_y,top_h,bottom_y,bottom_h,margin,gap,files_x,editor_x,settings_x,terminal_x,calc_x,files_w,editor_w,settings_w,terminal_w,calc_w;
static void panel(uint32_t x,uint32_t y,uint32_t w,uint32_t h,const char*title,uint32_t accent){window(x,y,w,h,title,accent);if(w>20&&h>40)rect(x+1,y+38,w-2,h-39,0xFF111827);}
static void draw_files(uint32_t x,uint32_t y,uint32_t w,uint32_t h){panel(x,y,w,h,"File Explorer",0xFF60A5FA);text(x+12,y+49,"/  MyOS File System",0xFF94A3B8);line(x+10,y+65,w-20,0xFF334155);const char*names[]={"bin/","boot/","dev/","etc/","home/","lib/","README.txt"};for(uint32_t i=0;i<7;i++){uint32_t yy=y+77+i*17;if(yy+9>=y+h-5)break;if(i<6)folder(x+12,yy-2);else rect(x+13,yy,12,12,0xFF60A5FA);text(x+42,yy,names[i],i==6?0xFFCBD5E1:0xFFF4D27A);}}
static void draw_editor(uint32_t x,uint32_t y,uint32_t w,uint32_t h){panel(x,y,w,h,"Text Editor  -  README.txt",0xFF34D399);text(x+12,y+50,"1  # Welcome to MyOS",0xFF6EE7B7);text(x+12,y+70,"2",0xFF64748B);text(x+12,y+90,"3  Native x86-64 desktop",0xFFCBD5E1);text(x+12,y+110,"4  Framebuffer graphics",0xFFCBD5E1);text(x+12,y+130,"5  No browser required.",0xFFCBD5E1);line(x+12,y+148,w-24,0xFF263449);text(x+12,y+162,"UTF-8     Plain text",0xFF64748B);}
static void draw_settings(uint32_t x,uint32_t y,uint32_t w,uint32_t h){panel(x,y,w,h,"Settings",0xFFA78BFA);text(x+12,y+51,"SYSTEM INFORMATION",0xFF94A3B8);text(x+12,y+73,"OS          MyOS",0xFFE2E8F0);text(x+12,y+93,"Version     0.4.2",0xFFE2E8F0);text(x+12,y+113,"Kernel      x86-64",0xFFE2E8F0);text(x+12,y+133,"Display     Framebuffer",0xFFE2E8F0);text(x+12,y+153,"Theme       Midnight",0xFF6EE7B7);text(x+12,y+173,"Status      Running",0xFF6EE7B7);}
static void draw_terminal(uint32_t x,uint32_t y,uint32_t w,uint32_t h){panel(x,y,w,h,"Terminal",0xFF34D399);rect(x+1,y+39,w-2,h-40,0xFF050A12);text(x+12,y+52,"MyOS Shell [native]",0xFF94A3B8);text(x+12,y+72,"Welcome to MyOS.",0xFF6EE7B7);text(x+12,y+92,"Type S to open launcher.",0xFF6EE7B7);text(x+12,y+112,"user@myos:~$ _",0xFF86EFAC);}
static void draw_calculator(uint32_t x,uint32_t y,uint32_t w,uint32_t h){panel(x,y,w,h,"Calculator",0xFF60A5FA);rect(x+9,y+49,w-18,30,0xFF0B1220);text(x+18,y+60,"0",0xFFFFFFFF);const char*keys[]={"7","8","9","+","4","5","6","-","1","2","3","x","C","0",".","="};uint32_t cols=4,cellw=(w-24)/cols;for(uint32_t i=0;i<16;i++){uint32_t xx=x+9+(i%cols)*cellw,yy=y+88+(i/cols)*23;if(yy+19>y+h-4)break;rect(xx,yy,cellw-3,20,(i%4==3)?0xFF1E3A5F:0xFF202A3A);text(xx+cellw/2-3,yy+6,keys[i],0xFFE2E8F0);}}
static void draw_taskbar(void){uint32_t ty=sh-42;rect(0,ty,sw,42,0xFF080E1A);line(0,ty,sw,0xFF29364A);rect(10,ty+7,32,28,0xFF34D399);grid(17,ty+14,0xFF071426);text(51,ty+17,"MyOS",0xFFFFFFFF);uint32_t x=108;const char*labels[]={"Files","Editor","Settings","Terminal","Calc"};uint8_t shown[]={show_files,show_editor,show_settings,show_terminal,show_calculator};for(uint32_t i=0;i<5;i++){uint32_t tw=sw<600?54:84;if(x+tw<sw-105){rect(x,ty+7,tw,28,shown[i]?0xFF1E293B:0xFF111827);frame(x,ty+7,tw,28,shown[i]?0xFF3B536E:0xFF263449);text(x+7,ty+17,labels[i],shown[i]?0xFFDAE6F6:0xFF718096);x+=tw+5;}}text(sw>110?sw-100:4,ty+17,"SYSTEM OK",0xFF6EE7B7);}
static void desktop(void){margin=sw/64;if(margin<8)margin=8;gap=margin;top_y=66;top_h=(sh>500)?(sh-160)/2:((sh-100)/2);if(top_h<85)top_h=85;bottom_y=top_y+top_h+gap;bottom_h=(sh>bottom_y+60)?sh-bottom_y-52:80;files_w=(sw-4*margin-2*gap)/3;editor_w=files_w;settings_w=files_w;files_x=margin;editor_x=files_x+files_w+gap;settings_x=editor_x+editor_w+gap;terminal_x=margin;terminal_w=(sw*42)/100;if(terminal_w<120)terminal_w=120;calc_x=terminal_x+terminal_w+gap;calc_w=(sw>calc_x+margin)?sw-calc_x-margin:100;if(calc_w>sw/4)calc_w=sw/4;rect(0,0,sw,sh,0xFF0B1020);rect(0,0,sw,54,0xFF111827);line(0,53,sw,0xFF29364A);rect(16,12,29,29,0xFF34D399);grid(23,19,0xFF071426);text(55,14,"MyOS",0xFFFFFFFF);text(55,30,"Native Desktop",0xFF94A3B8);if(sw>500)text(sw-150,23,"FRAMEBUFFER  |  READY",0xFF6EE7B7);if(show_files)draw_files(files_x,top_y,files_w,top_h);if(show_editor)draw_editor(editor_x,top_y,editor_w,top_h);if(show_settings)draw_settings(settings_x,top_y,settings_w,top_h);if(show_terminal)draw_terminal(terminal_x,bottom_y,terminal_w,bottom_h);if(show_calculator)draw_calculator(calc_x,bottom_y,calc_w,bottom_h);draw_taskbar();if(launcher_open)draw_launcher();}
static void handle_click(int x,int y){uint32_t ty=sh-42;if(y>= (int)ty){if(x<48){launcher_open=!launcher_open;return;}uint32_t tx=108;uint32_t tw=sw<600?54:84;for(uint32_t i=0;i<5;i++){if(tx+tw<sw-105&&x>=(int)tx&&x<(int)(tx+tw)){if(i==0)show_files=!show_files;else if(i==1)show_editor=!show_editor;else if(i==2)show_settings=!show_settings;else if(i==3)show_terminal=!show_terminal;else show_calculator=!show_calculator;launcher_open=0;return;}tx+=tw+5;}return;}if(launcher_open){uint32_t w=sw>850?560:sw-80,h=sh>650?430:sh-150,lx=(sw-w)/2,ly=(sh-h)/2;if(y>=ly+112&&y<=ly+200){if(x<lx+185)show_files=1;else if(x<lx+350)show_terminal=1;else show_settings=1;}launcher_open=0;return;}if(y>=top_y&&y<top_y+top_h){if(x>=files_x&&x<files_x+files_w){if(x>files_x+files_w-34&&y<top_y+38)show_files=0;}else if(x>=editor_x&&x<editor_x+editor_w){if(x>editor_x+editor_w-34&&y<top_y+38)show_editor=0;}else if(x>=settings_x&&x<settings_x+settings_w){if(x>settings_x+settings_w-34&&y<top_y+38)show_settings=0;}}else if(y>=bottom_y&&y<bottom_y+bottom_h){if(x>=terminal_x&&x<terminal_x+terminal_w&&x>terminal_x+terminal_w-34&&y<bottom_y+38)show_terminal=0;else if(x>=calc_x&&x<calc_x+calc_w&&x>calc_x+calc_w-34&&y<bottom_y+38)show_calculator=0;}}
static void input(void){if(!(inb(0x64)&1))return;uint8_t s=inb(0x60);if(s&0x80)return;if(s==0x1F){launcher_open=1;desktop();cursor_draw(mouse_x,mouse_y);}if(s==0x01){launcher_open=0;desktop();cursor_draw(mouse_x,mouse_y);}}
void kernel_main(uint64_t magic,struct multiboot_info*mb){serial_init();serial_puts("MyOS: kernel start\r\n");if((uint32_t)magic!=MULTIBOOT_MAGIC||!mb){vga_msg("MyOS: invalid Multiboot data",0x4F);halt();}if(!(mb->flags&FB_FLAG)||!mb->framebuffer_addr||mb->framebuffer_bpp!=32||!mb->framebuffer_pitch){vga_msg("MyOS: framebuffer unavailable",0x4E);halt();}fb=(uint32_t*)(uintptr_t)mb->framebuffer_addr;pitch=mb->framebuffer_pitch;sw=mb->framebuffer_width;sh=mb->framebuffer_height;if(sw<320||sh<200){vga_msg("MyOS: invalid framebuffer size",0x4E);halt();}memory_init(mb->mem_upper);{struct myos_memory_stats ms;memory_stats(&ms);serial_puts("MyOS: memory manager online\r\n");if(ms.heap_end>ms.heap_start)serial_puts("MyOS: kernel heap available\r\n");else serial_puts("MyOS: kernel heap unavailable\r\n");}mouse_x=(int)sw/2;mouse_y=(int)sh/2;desktop();mouse_init();cursor_draw(mouse_x,mouse_y);serial_puts("MyOS: desktop + PS2 mouse ready\r\n");for(;;){input();mouse_poll();__asm__ volatile("pause");}}
