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

static uint32_t *fb; static uint32_t pitch,sw,sh; static uint8_t launcher_open,settings_open,update_checked,boot_screen=1;

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
static void draw_launcher(void){
 /* Windows 11-inspired Start panel, anchored beside the MyOS taskbar button. */
 uint32_t w=sw>900?520:(sw>620?sw-48:sw-24),h=sh>650?420:sh-140;
 uint32_t x=12,y=sh-h-64;if(x+w>sw-12)w=sw-24;
 rect(x+6,y+6,w,h,0x44000000);rect(x,y,w,h,0xF01B2433);frame(x,y,w,h,0xFF3B4A60);
 rect(x+16,y+14,w-32,36,0xFF263247);frame(x+16,y+14,w-32,36,0xFF35445C);
 text(x+30,y+27,"Search apps, settings and files",0xFF9EADC2);
 text(x+20,y+70,"Pinned",0xFFFFFFFF);text(x+w-102,y+70,"All apps  >",0xFFB7C3D0);
 uint32_t gap=12,cardw=(w-56)/3,cardh=82;if(cardw>150)cardw=150;
 uint32_t sx=x+20;
 const char* labels[6]={"My Files","Terminal","Settings","Task Manager","System Info","Updates"};
 uint32_t colors[6]={0xFF3B82F6,0xFF0F766E,0xFF7C3AED,0xFF2563EB,0xFFB45309,0xFF475569};
 for(uint32_t i=0;i<6;i++){uint32_t col=i%3,row=i/3,cx=sx+col*(cardw+gap),cy=y+94+row*(cardh+14);
  rect(cx,cy,cardw,cardh,0xFF263247);frame(cx,cy,cardw,cardh,0xFF35445C);rect(cx+12,cy+12,28,28,colors[i]);
  if(i==0)folder(cx+13,cy+14);else if(i==1)terminal(cx+13,cy+15);else if(i==2)settings(cx+13,cy+14);else grid(cx+20,cy+20,0xFFFFFFFF);
  text(cx+12,cy+53,labels[i],0xFFF1F5F9);
 }
 uint32_t fy=y+h-54;rect(x+1,fy,w-2,53,0xFF151D2A);rect(x+18,fy+10,30,30,0xFF3478F6);
 grid(x+24,fy+16,0xFFFFFFFF);text(x+58,fy+14,"MyOS User",0xFFFFFFFF);text(x+58,fy+30,"Local session",0xFF9EADC2);text(x+w-96,fy+19,"Power  o",0xFFB7C3D0);
}
static int mouse_x,mouse_y,mouse_cycle,mouse_buttons,cursor_visible;
static uint8_t mouse_packet[3];
#define CURSOR_W 18
#define CURSOR_H 24
static uint32_t cursor_saved[CURSOR_W*CURSOR_H];
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
static void cursor_save(int x,int y){for(int yy=0;yy<CURSOR_H;yy++)for(int xx=0;xx<CURSOR_W;xx++){int px=x+xx,py=y+yy;cursor_saved[yy*CURSOR_W+xx]=(px>=0&&py>=0&&(uint32_t)px<sw&&(uint32_t)py<sh)?fb[py*(pitch/4)+px]:0;}}
static void cursor_restore(int x,int y){for(int yy=0;yy<CURSOR_H;yy++)for(int xx=0;xx<CURSOR_W;xx++){int px=x+xx,py=y+yy;if(px>=0&&py>=0&&(uint32_t)px<sw&&(uint32_t)py<sh)fb[py*(pitch/4)+px]=cursor_saved[yy*CURSOR_W+xx];}}
/* Compact Windows-style arrow. Each set bit is one cursor pixel, MSB first. */
static void cursor_draw(int x,int y){
 static const uint16_t shape[CURSOR_H]={0x8000,0xC000,0xE000,0xF000,0xF800,0xFC00,0xFE00,0xFF00,0xFF80,0xFFC0,0xFFE0,0xFFF0,0xFFF8,0xFFFC,0xFFF0,0xFBE0,0xF1C0,0xE080,0xC000,0xC000,0xE000,0x7000,0x3000,0x0000};
 cursor_save(x,y);
 for(int yy=0;yy<CURSOR_H;yy++){
  uint16_t row=shape[yy],above=yy?shape[yy-1]:0,below=yy+1<CURSOR_H?shape[yy+1]:0;
  uint16_t edge=(uint16_t)(row|(row>>1)|(row<<1)|above|(above>>1)|(above<<1)|below|(below>>1)|(below<<1));
  for(int xx=0;xx<16;xx++){uint16_t bit=(uint16_t)(0x8000u>>xx);int px=x+xx,py=y+yy;if(px<0||py<0||(uint32_t)px>=sw||(uint32_t)py>=sh)continue;if(edge&bit)fb[py*(pitch/4)+px]=0xFF101010;if(row&bit)fb[py*(pitch/4)+px]=0xFFFFFFFF;}
 }
 cursor_visible=1;
}
static void cursor_hide(void){if(cursor_visible){cursor_restore(mouse_x,mouse_y);cursor_visible=0;}}
static void cursor_move(int nx,int ny){if(nx<0)nx=0;if(ny<0)ny=0;if(nx>(int)sw-CURSOR_W)nx=(int)sw-CURSOR_W;if(ny>(int)sh-CURSOR_H)ny=(int)sh-CURSOR_H;if(nx==mouse_x&&ny==mouse_y)return;cursor_hide();mouse_x=nx;mouse_y=ny;cursor_draw(mouse_x,mouse_y);}
static void handle_mouse_click(int x,int y);
static void mouse_poll(void){for(;;){uint8_t st=inb(0x64);if(!(st&1))break;/* Never consume keyboard bytes as mouse data. */if(!(st&0x20))break;uint8_t d=inb(0x60);if(mouse_cycle==0){if(!(d&8))continue;mouse_packet[0]=d;mouse_cycle=1;}else if(mouse_cycle==1){mouse_packet[1]=d;mouse_cycle=2;}else{mouse_packet[2]=d;mouse_cycle=0;int dx=(int8_t)mouse_packet[1],dy=(int8_t)mouse_packet[2];cursor_move(mouse_x+dx*2,mouse_y-dy*2);int old_buttons=mouse_buttons;mouse_buttons=mouse_packet[0]&7;if((mouse_buttons&1)&&!(old_buttons&1))handle_mouse_click(mouse_x,mouse_y);}}}
static void draw_settings(void){uint32_t w=sw>700?560:sw-40,h=300,x=(sw-w)/2,y=100;window(x,y,w,h,"My Settings - Updates",0xFF805AD5);text(x+24,y+58,"System Updates",0xFFFFFFFF);text(x+24,y+82,"Installed version: development build",0xFFB7C3D0);text(x+24,y+104,"Release channel: stable (not connected)",0xFFB7C3D0);rect(x+24,y+140,190,38,0xFF2563EB);frame(x+24,y+140,190,38,0xFF60A5FA);text(x+38,y+155,"Check for Updates",0xFFFFFFFF);if(update_checked){text(x+24,y+198,"Online check is not available yet.",0xFFFBBF24);text(x+24,y+220,"Network, TLS and signed-manifest support",0xFFB7C3D0);text(x+24,y+238,"must be implemented before downloads.",0xFFB7C3D0);}else{text(x+24,y+198,"No update server is connected yet.",0xFFB7C3D0);text(x+24,y+220,"Updates will be verified before install.",0xFFB7C3D0);}text(x+24,y+270,"Esc = close  |  U = open update settings",0xFF6EE7B7);}

/* Native first-run screens. Disk installation and secure authentication are not implemented. */
static void draw_welcome(void){
 uint32_t w=sw>760?680:sw-32,h=sh>560?430:sh-32,x=(sw-w)/2,y=(sh-h)/2;
 rect(0,0,sw,sh,0xFF0B1220);rect(x+8,y+8,w,h,0x55000000);rect(x,y,w,h,0xFF151F30);frame(x,y,w,h,0xFF34445E);
 rect(x,y,w,5,0xFF4F8CFF);rect(x+34,y+34,44,44,0xFF3478F6);grid(x+45,y+45,0xFFFFFFFF);
 text(x+94,y+40,"Welcome to MyOS",0xFFFFFFFF);text(x+94,y+62,"A fresh start for your computer",0xFF9EADC2);line(x+34,y+98,w-68,0xFF344155);
 text(x+36,y+124,"Choose how you want to continue",0xFFFFFFFF);
 rect(x+34,y+160,w-68,78,0xFF202D42);frame(x+34,y+160,w-68,78,0xFF3B82F6);text(x+54,y+180,"Try MyOS",0xFFFFFFFF);text(x+54,y+202,"Open the live desktop without changing your disks.",0xFFB7C3D0);
 rect(x+34,y+254,w-68,78,0xFF1B2638);frame(x+34,y+254,w-68,78,0xFF3A4A63);text(x+54,y+274,"Install MyOS",0xFFFFFFFF);text(x+54,y+296,"Review installation readiness and safety status.",0xFFB7C3D0);
 text(x+36,y+h-36,"ENTER Try MyOS     I Install     No disk changes are made here",0xFF6EE7B7);
}
static void draw_signin(void){
 uint32_t w=sw>650?520:sw-32,h=370,x=(sw-w)/2,y=(sh-h)/2;
 rect(0,0,sw,sh,0xFF0B1220);rect(x+8,y+8,w,h,0x55000000);rect(x,y,w,h,0xFF151F30);frame(x,y,w,h,0xFF34445E);rect(x,y,w,5,0xFF4F8CFF);
 rect(x+w/2-24,y+30,48,48,0xFF263B5A);text(x+w/2-8,y+45,"M",0xFFFFFFFF);text(x+w/2-52,y+96,"Welcome back",0xFFFFFFFF);text(x+w/2-80,y+120,"Sign in to your MyOS session",0xFF9EADC2);
 rect(x+42,y+158,w-84,48,0xFF202D42);frame(x+42,y+158,w-84,48,0xFF3B82F6);text(x+58,y+177,"MyOS User",0xFFFFFFFF);
 rect(x+42,y+220,w-84,42,0xFF1B2638);frame(x+42,y+220,w-84,42,0xFF3A4A63);text(x+58,y+236,"Local demo session",0xFFB7C3D0);
 text(x+42,y+284,"Secure accounts/passwords are not implemented yet.",0xFFFBBF24);
 rect(x+w-174,y+316,132,34,0xFF2563EB);frame(x+w-174,y+316,132,34,0xFF60A5FA);text(x+w-151,y+329,"Continue",0xFFFFFFFF);text(x+42,y+h-18,"ENTER Continue     ESC Back",0xFF6EE7B7);
}
static void draw_installer(void){
 uint32_t w=sw>760?680:sw-32,h=430,x=(sw-w)/2,y=(sh-h)/2;
 rect(0,0,sw,sh,0xFF0B1220);rect(x+8,y+8,w,h,0x55000000);rect(x,y,w,h,0xFF151F30);frame(x,y,w,h,0xFF34445E);rect(x,y,w,5,0xFFF59E0B);
 text(x+32,y+28,"Install MyOS",0xFFFFFFFF);text(x+32,y+54,"Installation readiness",0xFF9EADC2);line(x+32,y+82,w-64,0xFF344155);
 rect(x+32,y+104,28,28,0xFF14532D);text(x+41,y+114,"1",0xFFFFFFFF);text(x+74,y+108,"Boot environment",0xFFFFFFFF);text(x+74,y+128,"This screen is running from the live kernel.",0xFFB7C3D0);
 rect(x+32,y+156,28,28,0xFF7F1D1D);text(x+41,y+166,"!",0xFFFFFFFF);text(x+74,y+160,"Disk installer unavailable",0xFFFFFFFF);text(x+74,y+182,"Disk detection, partitioning and file copying are not ready.",0xFFFBBF24);
 rect(x+32,y+218,w-64,64,0xFF2A2025);frame(x+32,y+218,w-64,64,0xFF854D4D);text(x+48,y+232,"SAFETY: No disks have been changed.",0xFFFFFFFF);text(x+48,y+254,"Install is disabled until a tested installer exists.",0xFFFFC4C4);
 rect(x+32,y+h-72,128,38,0xFF263449);frame(x+32,y+h-72,128,38,0xFF536780);text(x+62,y+h-58,"Back",0xFFFFFFFF);
 rect(x+w-194,y+h-72,162,38,0xFF202A3A);frame(x+w-194,y+h-72,162,38,0xFF536780);text(x+w-174,y+h-58,"Try MyOS instead",0xFFFFFFFF);text(x+32,y+h-20,"ESC Back     T Try MyOS",0xFF6EE7B7);
}
static void desktop(void){
 /* Cool blue layered wallpaper, rendered natively in the framebuffer. */
 rect(0,0,sw,sh,0xFF101827);rect(0,0,sw/2,sh,0xFF111D31);rect(sw/2,0,sw-sw/2,sh,0xFF17243A);
 rect(sw/8,sh/8,sw*3/4,sh*3/4,0xFF14243B);rect(sw/3,sh/5,sw/3,sh/2,0xFF1B3658);
 rect(sw/3+18,sh/5+18,sw/3-36,sh/2-36,0xFF172D4A);rect(sw/3+36,sh/5+36,sw/3-72,sh/2-72,0xFF1D4165);
 folder(28,32);text(20,62,"My Files",0xFFF1F5F9);terminal(28,94);text(16,124,"Terminal",0xFFF1F5F9);
 settings(28,156);text(13,186,"Settings",0xFFF1F5F9);grid(28,218,0xFFB7C3D0);text(13,248,"My System",0xFFF1F5F9);
 uint32_t w=sw>900?560:(sw>700?520:sw-110),h=220,x=(sw-w)/2,y=(sh-h-58)/2;
 rect(x+7,y+7,w,h,0x44000000);rect(x,y,w,h,0xE61A2433);frame(x,y,w,h,0xFF35445C);rect(x,y,w,4,0xFF60A5FA);
 rect(x+22,y+22,36,36,0xFF3478F6);grid(x+31,y+31,0xFFFFFFFF);text(x+72,y+24,"Welcome to MyOS",0xFFFFFFFF);
 text(x+72,y+46,"A modern desktop, built from the kernel up.",0xFFB7C3D0);line(x+22,y+76,w-44,0xFF35445C);text(x+22,y+96,"Quick access",0xFF9EADC2);
 rect(x+22,y+118,138,58,0xFF263247);folder(x+34,y+132);text(x+70,y+140,"My Files",0xFFFFFFFF);
 rect(x+172,y+118,138,58,0xFF263247);terminal(x+184,y+135);text(x+220,y+140,"Terminal",0xFFFFFFFF);
 rect(x+322,y+118,138,58,0xFF263247);settings(x+334,y+135);text(x+370,y+140,"Settings",0xFFFFFFFF);
 text(x+22,y+h-22,"Press S to open Start   |   Esc to close",0xFF6EE7B7);
 uint32_t ty=sh-48;rect(0,ty,sw,48,0xE6171F2C);line(0,ty,sw,0xFF3A465A);
 rect(10,ty+7,38,34,launcher_open?0xFF315A91:0xFF26364D);frame(10,ty+7,38,34,0xFF49617F);grid(20,ty+17,0xFFFFFFFF);
 text(56,ty+18,"MyOS",0xFFFFFFFF);rect(112,ty+7,38,34,0xFF202A3A);folder(118,ty+13);
 rect(156,ty+7,38,34,0xFF202A3A);terminal(162,ty+13);rect(200,ty+7,38,34,0xFF202A3A);settings(206,ty+13);
 rect(sw-142,ty+7,132,34,0xFF202A3A);text(sw-130,ty+18,"NET  |  VOL",0xFFB7C3D0);
 if(launcher_open)draw_launcher();if(settings_open)draw_settings();
}
static void handle_mouse_click(int x,int y){
 if(boot_screen){
  uint32_t w=sw>760?680:sw-32,h=sh>560?430:sh-32,bx=(sw-w)/2,by=(sh-h)/2;
  if(boot_screen==1){if(y>=(int)by+150&&y<(int)by+242)boot_screen=3;else if(y>=(int)by+244&&y<(int)by+340)boot_screen=2;}
  else if(boot_screen==2){if(y>=(int)(by+h-82)&&x<(int)bx+180)boot_screen=1;else if(y>=(int)(by+h-82)&&x>(int)bx+(int)w-205)boot_screen=3;}
  else if(boot_screen==3){uint32_t swid=sw>650?520:sw-32,sx=(sw-swid)/2,sy=(sh-370)/2;if(y>=(int)sy+310&&x>(int)sx+(int)swid-184)boot_screen=0;else return;}
  cursor_hide();if(boot_screen==1)draw_welcome();else if(boot_screen==2)draw_installer();else if(boot_screen==3)draw_signin();else desktop();cursor_draw(mouse_x,mouse_y);return;
 }
 uint32_t ty=sh-48;
 if(y>=(int)ty&&x>=10&&x<=48){launcher_open=!launcher_open;settings_open=0;cursor_hide();desktop();cursor_draw(mouse_x,mouse_y);return;}
 if(launcher_open){
  uint32_t w=sw>900?520:(sw>620?sw-48:sw-24),h=sh>650?420:sh-140,lx=12,ly=sh-h-64;
  if(lx+w>sw-12)w=sw-24;
  uint32_t cardw=(w-56)/3;if(cardw>150)cardw=150;uint32_t gap=12,sx=lx+20;
  if(x>=(int)sx&&y>=(int)ly+94&&y<(int)ly+94+2*(82+14)){
   uint32_t col=(uint32_t)(x-(int)sx)/(cardw+gap),row=(uint32_t)(y-((int)ly+94))/(82+14);
   if(col<3&&row<2){uint32_t idx=row*3+col;launcher_open=0;if(idx==2||idx==5){settings_open=1;update_checked=0;}cursor_hide();desktop();cursor_draw(mouse_x,mouse_y);return;}
  }
  if(x<(int)lx||x>(int)(lx+w)||y<(int)ly||y>(int)(ly+h))launcher_open=0;
 }
 uint32_t pw=sw>700?560:sw-40,px=(sw-pw)/2,py=100;
 if(settings_open){if(y>=(int)py&&y<(int)py+38&&x>=(int)(px+pw-42))settings_open=0;else if(x>=(int)px+24&&x<(int)px+214&&y>=(int)py+140&&y<(int)py+178)update_checked=1;else return;}
 else{uint32_t ww=sw>900?560:(sw>700?520:sw-110),wx=(sw-ww)/2,wy=(sh-220-58)/2;
  if((x>=15&&x<=75&&y>=210&&y<=258)||(x>=200&&x<=238&&y>=(int)sh-41&&y<(int)sh-7))settings_open=1;
  if(x>=(int)wx+322&&x<(int)wx+460&&y>=(int)wy+118&&y<(int)wy+176)settings_open=1;
  if(!settings_open)return;launcher_open=0;
 }
 cursor_hide();desktop();cursor_draw(mouse_x,mouse_y);
}
static void input(void){
 if(!(inb(0x64)&1)) return;
 if(inb(0x64)&0x20) return;
 uint8_t s=inb(0x60);
 if(s&0x80) return;
 if(boot_screen){
  if(boot_screen==1){if(s==0x1C||s==0x14)boot_screen=3;else if(s==0x17)boot_screen=2;}
  else if(boot_screen==2){if(s==0x01)boot_screen=1;else if(s==0x14)boot_screen=3;}
  else if(boot_screen==3){if(s==0x1C)boot_screen=0;else if(s==0x01)boot_screen=1;}
  cursor_hide();if(boot_screen==1)draw_welcome();else if(boot_screen==2)draw_installer();else if(boot_screen==3)draw_signin();else desktop();cursor_draw(mouse_x,mouse_y);return;
 }
 if(s==0x1F){cursor_hide();launcher_open=1;settings_open=0;desktop();cursor_draw(mouse_x,mouse_y);}
 if(s==0x01){cursor_hide();launcher_open=0;settings_open=0;desktop();cursor_draw(mouse_x,mouse_y);}
 if(s==0x16){cursor_hide();settings_open=1;launcher_open=0;desktop();cursor_draw(mouse_x,mouse_y);}
}
void kernel_main(uint64_t magic,struct multiboot_info*mb){serial_init();serial_puts("MyOS: kernel start\r\n");if((uint32_t)magic!=MULTIBOOT_MAGIC||!mb){vga_msg("MyOS: invalid Multiboot data",0x4F);halt();}if(!(mb->flags&FB_FLAG)||!mb->framebuffer_addr||mb->framebuffer_bpp!=32||!mb->framebuffer_pitch){vga_msg("MyOS: framebuffer unavailable",0x4E);halt();}fb=(uint32_t*)(uintptr_t)mb->framebuffer_addr;pitch=mb->framebuffer_pitch;sw=mb->framebuffer_width;sh=mb->framebuffer_height;if(sw<320||sh<200){vga_msg("MyOS: invalid framebuffer size",0x4E);halt();}memory_init(mb->mem_upper);{struct myos_memory_stats ms;memory_stats(&ms);serial_puts("MyOS: memory manager online\r\n");if(ms.heap_end>ms.heap_start)serial_puts("MyOS: kernel heap available\r\n");else serial_puts("MyOS: kernel heap unavailable\r\n");}mouse_x=(int)sw/2;mouse_y=(int)sh/2;draw_welcome();mouse_init();cursor_draw(mouse_x,mouse_y);serial_puts("MyOS: first-run welcome screen ready\r\n");for(;;){input();mouse_poll();__asm__ volatile("pause");}}
