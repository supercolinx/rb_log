#ifndef _COLIN_RB_LOG_H
#define _COLIN_RB_LOG_H
#include <sys/types.h>

void rbg_destory(void *rbg);
void *rbg_create(const char *pathname, int size);
ssize_t rbg_write(void *rbg, const char *buf, size_t size);
ssize_t rbg_read(void *rbg, char *buf, size_t size);
ssize_t rbg_backup(const void *rbg, const char *pathname);

#endif /* _COLIN_RB_LOG_H */
