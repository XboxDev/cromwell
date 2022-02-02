#ifndef __STDARG_H__
#define __STDARG_H__

typedef char* va_list;

#define _PDCLIB_va_round( type ) ( (sizeof(type) + sizeof(void *) - 1) & ~(sizeof(void *) - 1) )
#define va_arg( ap, type ) ( (ap) += (_PDCLIB_va_round(type)), ( *(type*) ( (ap) - (_PDCLIB_va_round(type)) ) ) )
#define va_copy( dest, src ) ( (dest) = (src), (void)0 )
#define va_end( ap ) ( (ap) = (void *)0, (void)0 )
#define va_start( ap, parmN ) ( (ap) = (char *) &parmN + ( _PDCLIB_va_round(parmN) ), (void)0 )

#endif
