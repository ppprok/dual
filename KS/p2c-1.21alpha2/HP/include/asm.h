/* Header for module asm, generated by p2c */
#ifndef ASM_H
#define ASM_H



#ifndef SYSGLOBALS_H
#include <p2c/sysglobals.h>
#endif



#ifdef ASM_G
# define vextern
#else
# define vextern extern
#endif



extern Void asm_moveleft PP((long *s, long *d, long z));
extern Void asm_moveright PP((long *s, long *d, long z));
extern Void asm_fastmove PP((Anyptr *s, Anyptr *d, long z));
extern Void asm_newbytes PP((Anyptr **p, long z));
extern Void asm_powerup PV( );
extern Void asm_errmsg PV( );
extern Void asm_findroms PV( );
extern Void asm_f_pwr_on PV( );
extern Void asm_flpyread PP((long sector, long *buffer));
extern Void asm_flpy_wrt PP((long sector, long *buffer));
extern Void asm_flpymread PP((long sector_count, long sector, long *buffer));
extern Void asm_flpymwrite PP((long sector_count, long sector, long *buffer));
extern Void asm_flpyinit PP((Anyptr *ptr, int i));
extern Void asm_setintlevel PP((long level));
extern long asm_intlevel PV( );
extern Void asm_newwords PP((Anyptr **p, long wordsize));
extern Void asm_userprogram PP((long execloc, long initsp));
extern Void asm_sappend PP((Char *dest, int MAX_dest, Char *src));
extern long asm_iand PP((long a, long b));
extern long asm_ior PP((long a, long b));
extern Void asm_ci_switch PV( );
extern Void asm_initvects PV( );
extern Void asm_cpymsg PP((Char *msg));
#define asm_memavail()        10000000
/*  extern long asm_memavail PV( );  */
extern long asm_ticker PV( );
extern Void asm_cache_on PV( );
extern Void asm_cache_off PV( );
extern Void asm_icache_off PV( );
extern Void asm_icache_on PV( );
extern Void asm_flush_icache PV( );



#undef vextern

#endif /*ASM_H*/

/* End. */

