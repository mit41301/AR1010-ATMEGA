
#ifndef __MYDEFS_H
#define __MYDEFS_H
#define	F_CPU		8000000
#define T1_PRESCALER	8
#define T1_TICKS   		F_CPU / ( T1_PRESCALER * 65536 )	
#define T1_ONESECOND    T1_TICKS


#endif //__MYDEFS_H
