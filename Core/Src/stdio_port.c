/* Override CubeMX's weak newlib I/O stubs without editing generated code. */
#include <errno.h>

extern int __io_putchar(int ch) __attribute__((weak));
extern int __io_getchar(void) __attribute__((weak));

int _write(int file, char *ptr, int len)
{
  if (file != 1 && file != 2) {
    errno = EBADF;
    return -1;
  }
  if (len < 0) {
    errno = EINVAL;
    return -1;
  }
  if (len == 0) return 0;
  if (!__io_putchar) {
    errno = ENOSYS;
    return -1;
  }
  for (int i = 0; i < len; ++i) {
    if (__io_putchar((unsigned char)ptr[i]) < 0) {
      errno = EIO;
      return i ? i : -1;
    }
  }
  return len;
}

int _read(int file, char *ptr, int len)
{
  if (file != 0) {
    errno = EBADF;
    return -1;
  }
  if (len < 0) {
    errno = EINVAL;
    return -1;
  }
  if (len == 0) return 0;
  if (!__io_getchar) {
    errno = ENOSYS;
    return -1;
  }
  for (int i = 0; i < len; ++i) {
    int ch = __io_getchar();
    if (ch < 0) {
      errno = EIO;
      return i ? i : -1;
    }
    ptr[i] = (char)ch;
  }
  return len;
}
