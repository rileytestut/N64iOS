#ifndef NO_ASM
#define NEW_DYNAREC 1
#endif

extern int pcaddr;
extern int pending_exception;

void invalidate_block(unsigned int block);
void new_dynarec_init();
void new_dynarec_cleanup();
