#include <stdint.h>
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
static void vga_msg(const char*s,uint8_t color){
 uint32_t i=0;
 while(*s && i<80*25){vga[i++]=((uint16_t)color<<8)|(uint8_t)*s++;}
}
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
static void window(uint32_t x,uint32_t y,uint32_t w,uint32_t h,const char*t,uint32_t a){
 rect(x+7,y+7,w,h,0x55000000);rect(x,y,w,h,0xFF182130);frame(x,y,w,h,0xFF354155);
 rect(x,y,w,38,0xFF202A3A);rect(x,y,4,38,a);text(x+18,y+15,t,0xFFFFFFFF);
 rect(x+w-72,y+14,8,8,0xFF8794A8);rect(x+w-50,y+14,8,8,0xFF8794A8);rect(x+w-28,y+14,8,8,0xFFF87171);
}
static void draw_launcher(void){
 uint32_t w=sw>850?560:sw-80,h=sh>650?430:sh-150,x=(sw-w)/2,y=(sh-h)/2;
 rect(x+8,y+8,w,h,0x55000000);rect(x,y,w,h,0xFF141B28);frame(x,y,w,h,0xFF40506A);
 rect(x,y,w,62,0xFF1E2736);text(x+24,y+22,"Search MyOS",0xFF8B98AB);line(x+24,y+48,w-48,0xFF344155);
 text(x+24,y+86,"Applications",0xFF8B98AB);
 rect(x+24,y+112,150,88,0xFF202A3A);folder(x+48,y+132);text(x+48,y+166,"Files",0xFFFFFFFF);
 rect(x+192,y+112,150,88,0xFF202A3A);terminal(x+214,y+132);text(x+214,y+166,"Terminal",0xFFFFFFFF);
 rect(x+360,y+112,150,88,0xFF202A3A);settings(x+382,y+132);text(x+382,y+166,"Settings",0xFFFFFFFF);
 text(x+24,y+238,"System",0xFF8B98AB);text(x+24,y+268,"Display",0xFFD5DCE7);text(x+24,y+290,"Network",0xFFD5DCE7);text(x+24,y+312,"Appearance",0xFFD5DCE7);
 text(x+24,y+356,"Press ESC to return to desktop",0xFF6EE7B7);
}
static void desktop(void){
 rect(0,0,sw,sh,0xFF0B1020);rect(0,60,sw,sh-112,0xFF101827);rect(sw/2,60,sw/2,sh-112,0xFF111D31);
 rect(0,0,sw,60,0xFF171D29);line(0,59,sw,0xFF2D394C);rect(18,16,28,28,0xFF3B82F6);grid(23,20,0xFFFFFFFF);
 text(58,18,"MyOS",0xFFFFFFFF);text(58,34,"Native Desktop",0xFF8B98AB);
 if(sw>520){text(sw-305,23,"CPU",0xFF8B98AB);text(sw-265,23,"RAM",0xFF8B98AB);text(sw-215,23,"NET",0xFF8B98AB);text(sw-155,23,"LIVE",0xFF6EE7B7);}
 folder(34,92);text(30,120,"Files",0xFFE8EDF5);terminal(34,156);text(25,184,"Terminal",0xFFE8EDF5);settings(34,220);text(22,248,"Settings",0xFFE8EDF5);
 uint32_t w=sw>900?650:(sw>700?570:sw-120),h=330,x=(sw-w)/2,y=100;
 window(x,y,w,h,"Welcome to MyOS",0xFF3B82F6);text(x+34,y+70,"A native x86-64 desktop.",0xFFFFFFFF);text(x+34,y+94,"Mouse and keyboard input are now active.",0xFFB7C3D0);
 line(x+34,y+130,w-68,0xFF344155);text(x+34,y+154,"Quick access",0xFF8B98AB);
 rect(x+34,y+180,150,72,0xFF202A3A);folder(x+52,y+198);text(x+52,y+224,"Files",0xFFFFFFFF);
 rect(x+204,y+180,150,72,0xFF202A3A);terminal(x+222,y+198);text(x+222,y+224,"Terminal",0xFFFFFFFF);
 rect(x+374,y+180,150,72,0xFF202A3A);settings(x+392,y+198);text(x+392,y+224,"Settings",0xFFFFFFFF);
 text(x+34,y+286,"S = launcher   Esc = close   Left click = select",0xFF6EE7B7);
 uint32_t ty=sh-52;rect(0,ty,sw,52,0xFF171D29);line(0,ty,sw,0xFF303B4E);
 rect(14,ty+10,34,32,0xFF2563EB);grid(22,ty+16,0xFFFFFFFF);rect(62,ty+10,44,32,0xFF202A3A);folder(72,ty+15);
 rect(112,ty+10,44,32,0xFF202A3A);terminal(121,ty+15);rect(162,ty+10,44,32,0xFF202A3A);settings(171,ty+15);
 if(launcher_open)draw_launcher();
}
static void input(void){if(!(inb(0x64)&1))return;uint8_t s=inb(0x60);if(s&0x80)return;if(s==0x1F){launcher_open=1;desktop();}if(s==0x01){launcher_open=0;desktop();}}

void kernel_main(uint64_t magic,struct multiboot_info*mb){
 serial_init();
 serial_puts("MyOS: kernel start\r\n");
 if((uint32_t)magic!=MULTIBOOT_MAGIC||!mb){vga_msg("MyOS: invalid Multiboot data",0x4F);halt();}
 if(!(mb->flags&FB_FLAG)||!mb->framebuffer_addr||mb->framebuffer_bpp!=32||!mb->framebuffer_pitch){
  vga_msg("MyOS: framebuffer unavailable",0x4E);
  halt();
 }
 fb=(uint32_t*)(uintptr_t)mb->framebuffer_addr;
 pitch=mb->framebuffer_pitch;sw=mb->framebuffer_width;sh=mb->framebuffer_height;
 if(sw<320||sh<200){vga_msg("MyOS: invalid framebuffer size",0x4E);halt();}
 mouse_x=(int)sw/2;mouse_y=(int)sh/2;
 desktop();
 mouse_init();
 cursor_draw(mouse_x,mouse_y);
 serial_puts("MyOS: desktop + PS2 mouse ready\r\n");
 for(;;){input();__asm__ volatile("pause");}
}
