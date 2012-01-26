#ifdef WIN32_MEMORY_LEAK_DETECT
// Undefine new so we don't blow the PLACEMENT_NEW() calls.
#ifdef new
#undef new
#endif

#endif	// WIN32_MEMORY_LEAK_DETECT


#define PLACEMENT_NEW(addr) new(addr)
