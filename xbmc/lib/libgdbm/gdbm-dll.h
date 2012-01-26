#ifndef _GDBMDLLIMPEXP_H_
#define _GDBMDLLIMPEXP_H_ 1

#ifndef __GNUC__
# define __DLL_IMPORT__  __declspec(dllimport)
# define __DLL_EXPORT__  __declspec(dllexport)
#else
# define __DLL_IMPORT__  __attribute__((dllimport)) extern
# define __DLL_EXPORT__  __attribute__((dllexport)) extern
#endif 

#if (defined __WIN32__) || (defined _WIN32)
# ifdef BUILD_GDBM_DLL
#  define GDBM_DLL_IMPEXP     __DLL_EXPORT__
# elif defined(BUILD_GDBM_STATIC)
#  define GDBM_DLL_IMPEXP      
# elif defined (USE_GDBM_DLL)
#  define GDBM_DLL_IMPEXP     __DLL_IMPORT__
# elif defined (USE_GDBM_STATIC)
#  define GDBM_DLL_IMPEXP      
# else /* assume USE_GDBM_DLL */
#  define GDBM_DLL_IMPEXP     __DLL_IMPORT__
# endif
#else /* __WIN32__ */
# define GDBM_DLL_IMPEXP  
#endif

#endif /* _GDBMDLLIMPEXP_H_ */
