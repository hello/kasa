#ifndef __AMBA_VSPRINTF__
#define __AMBA_VSPRINTF__

# define do_div(n,base) ({				\
	int __base = (base);			\
	int __rem;					\
	(void)(((typeof((n)) *)0) == ((unsigned long long *)0));	\
	if (((n) >> 32) == 0) {			\
		__rem = (int)(n) % __base;		\
		(n) = (int)(n) / __base;		\
	} else						\
		__rem = __div64_32(&(n), __base);	\
	__rem;						\
 })


int sprintf(char *buf, const char *fmt, ...);
int printf(const char *fmt, ...);


#endif /* __AMBA_VSPRINTF__ */
