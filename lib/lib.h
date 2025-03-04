#ifndef _LIB_H_
#define _LIB_H_

#include "stdint.h"

int printf(const char *format, ...);
void clear_screenu();
void sleepu(uint64_t ticks);
void exitu(void);
void waitu(int pid);
unsigned char keyboard_readu(void);
void memset(void* buffer, char value, int size);
void memmove(void* dst, void* src, int size);
void memcpy(void* dst, void* src, int size);
int memcmp(void* src1, void* src2, int size);
int get_total_memoryu(void);
int get_free_memoryu(void);
int get_used_memoryu(void);
int open_file(char *name);
int read_file(int fd, void *buffer, uint32_t size);
void close_file(int fd);
int get_file_size(int fd);
int fork();
void exec(char* name);

#endif