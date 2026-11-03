#include <assert.h>
#include <errno.h>

int _read(int file, char *ptr, int len);
int _write(int file, char *ptr, int len);

#ifdef TEST_BACKEND
static int calls;
static int fail_after = -1;
int __io_putchar(int ch)
{
  if (calls == fail_after) return -1;
  ++calls;
  assert(ch == 255);
  return ch;
}
int __io_getchar(void)
{
  if (calls == fail_after) return -1;
  ++calls;
  return 255;
}
#endif

int main(void)
{
  char buf[3] = {(char)255, (char)255, (char)255};
  assert(_write(0, buf, 1) == -1 && errno == EBADF);
  assert(_read(1, buf, 1) == -1 && errno == EBADF);
  assert(_write(1, buf, -1) == -1 && errno == EINVAL);
  assert(_read(0, buf, -1) == -1 && errno == EINVAL);
  assert(_write(1, buf, 0) == 0);
  assert(_read(0, buf, 0) == 0);
#ifdef TEST_BACKEND
  assert(_write(1, buf, 3) == 3);
  assert(_write(2, buf, 3) == 3);
  assert(_read(0, buf, 3) == 3);
  assert((unsigned char)buf[2] == 255);
  calls = 0;
  fail_after = 1;
  assert(_write(1, buf, 3) == 1 && errno == EIO);
  calls = 0;
  assert(_read(0, buf, 3) == 1 && errno == EIO);
  calls = 0;
  fail_after = 0;
  assert(_write(1, buf, 3) == -1 && errno == EIO);
  assert(_read(0, buf, 3) == -1 && errno == EIO);
#else
  assert(_write(1, buf, 1) == -1 && errno == ENOSYS);
  assert(_write(2, buf, 1) == -1 && errno == ENOSYS);
  assert(_read(0, buf, 1) == -1 && errno == ENOSYS);
#endif
  return 0;
}
