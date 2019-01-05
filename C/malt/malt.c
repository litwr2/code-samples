#include <stdarg.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <errno.h>
#include <libmalt.h>
#include "romfs.h"

int isatty(int fd);

#define DEF0(nm) void nm(){ tprintf("%s called!\n",#nm); /*malt_trace_fatal("");*/ }

int gettimeofday(struct timeval * __restrict tv, void* __restrict tz) {
  u64_t t = malt_get_time_us();
  tv->tv_sec  = 1521000000 + t / 1000000;   //20th of March 2018
  tv->tv_usec = t % 1000000;
  return 0;
}

int clock_gettime(clockid_t clk_id, struct timespec *tp) {
  u64_t t = malt_get_time_us();
  tp->tv_sec  = 1521000000 + t / 1000000;   //20th of March 2018
  tp->tv_nsec = t % 1000000000;
  return 0;
}

void tprintf(const char *fmt, ...) {
  va_list ap;
  va_start(ap,fmt);
  char buf[0x80];
  vsnprintf(buf,sizeof(buf)-1,fmt,ap);
  va_end (ap);
#if 1 //!?
  malt_write_reg(TRACER_TRACE_ADDR, (u32_t)buf);
#else
  char *p = buf;
  while (*p)
    malt_putchar(*p++);
#endif
}

unsigned write(int fd, const char *buf, size_t count) {
  if (isatty(fd)) //FIXME?
    for (int i=0; i<count; i++)
      malt_putchar(buf[i]);
  return count;
}

int fstat(int fd, struct stat *st)
{
    //write(1,"fstat!\n",6);
    if (isatty(fd))
      st->st_mode = S_IFCHR;
    return 0;
}

int isatty(int fd) {
  return fd >= 0 && fd <= 2;
}

int close(int fd)
{
    //malt_write_reg(TRACER_CLOSE_ADDR, fd);
    //return malt_trace_status();
    return romfs_close(fd - 3);
}

int open(const char *path, int flags, ...)
{
    //return malt_trace_open2(0, path);
    if (flags != O_RDONLY)
        return errno = EROFS, -1;
    return romfs_open(path, flags);
}

int read(int fd, void *buf, size_t nbytes)
{
    //return malt_trace_read2(fd, buf, nbytes);

    if (fd == 0) { // stdin!
      char *b=(char*)buf, *p=b;
      for (; p-b < nbytes-1; p++) { 
        *p = malt_getchar();
        //fprintf(stderr,"%c",*p & 0xff);
        if (*p == '\n') {
            p++;
            break;
        }
        if (*p == -1) //EOF
            break;
      }
      *p = '\0';
      return p-b;
    }

    return romfs_read(fd - 3, buf, nbytes);
}

off_t lseek(int fd, off_t offset, int whence) {
    return romfs_lseek(fd - 3, offset, whence);
}

int unlink(const char *pathname) { //a dummy function for Google Test
    return 0;
}

int mkdir(const char *pathname, mode_t mode) { //a dummy function for Google Test
    return errno = EACCES, -1;
}

char *getcwd(char *buf, size_t size) { //a dummy function for Google Test
    if (size >= 2) {
      buf[0] = '/';
      buf[1] = 0;
      return buf;
    }
    return errno = ERANGE, NULL;
}

int stat(const char *pathname, struct stat *buf) { //a dummy function for Google Test
    return errno = EACCES, -1;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
    useconds_t usec = req->tv_sec*1000000 + (req->tv_nsec + 500)/1000;
    malt_sleep_pcnt_us(usec);
    return 0;
}

unsigned usleep(useconds_t usec) {
    malt_sleep_pcnt_us(usec);
    return 0;
}

unsigned sleep(unsigned sec) {
    malt_sleep_pcnt_us(sec*1000000);
    return 0;
}

long sysconf(int name) { //a dummy function
    tprintf("[sysconf] name = %d\n", name);
    return errno = EINVAL, -1;
}

void malt_libc_init() {
}

void malt_libc_exit() {
//  __call_exitprocs (code, NULL); 
// What is it for?! The stack unwinder? It is taken from Newlib's exit()
  if (_GLOBAL_REENT->__cleanup)
    (*_GLOBAL_REENT->__cleanup) (_GLOBAL_REENT);
}

void _exit(int rc) {
  if (rc)
    malt_trace_fatal("");
  else
    malt_trace_finish();
}

int kill(pid_t pid, int sig) {
  tprintf("%s: pid=%d, sig=%d\n", __func__, pid, sig);
  return -1;
}

pid_t getpid() {
  return 1;
}

void abort() {
  printf("%s:\n",__func__);
  exit(1);
}

extern unsigned __heap_start;
static char*    __brk_val;

void* sbrk(int c)
{
    if (!__brk_val)
        __brk_val = (char*)&__heap_start;
        
    if ((c > 0) && ((u32_t)__brk_val >= (0xFFFF0000 - c)))
    {
        malt_trace_print("malloc fail %X %X\n", c, __brk_val);
        malt_trace_fatal("malloc overflow!");
        return NULL;
    }
    
    void* old_brk_val = __brk_val; 
    __brk_val += c;
    return old_brk_val;
}

malt_mutex __lock___sinit_recursive_mutex;
malt_mutex __lock___sfp_recursive_mutex;
malt_mutex __lock___atexit_recursive_mutex;
malt_mutex __lock___at_quick_exit_mutex;
malt_mutex __lock___malloc_recursive_mutex;
malt_mutex __lock___env_recursive_mutex;
malt_mutex __lock___tz_mutex;
malt_mutex __lock___dd_hash_mutex;
malt_mutex __lock___arc4random_mutex;
#define lock_exit(m) {malt_write_reg(TRACER_TRACE_ADDR, (u32_t)m);exit(7);}
void __retarget_lock_init (_LOCK_T *plock) {
    *plock = calloc(1, sizeof(malt_mutex));
}
void __retarget_lock_init_recursive (_LOCK_T *plock) {
    *plock = calloc(1, sizeof(malt_mutex));
}
void __retarget_lock_close (_LOCK_T lock) {
    free(lock);
}
void __retarget_lock_close_recursive (_LOCK_T lock) {
    free(lock);
}
void __retarget_lock_acquire (_LOCK_T lock) {
    malt_inc_mutex_lock(lock);
}
void __retarget_lock_release (_LOCK_T lock) {
    malt_inc_mutex_unlock(lock);
}
int __retarget_lock_try_acquire (_LOCK_T lock) {
    lock_exit("__retarget_lock_try_acquire() not implemented");
    return 0;
}
int __retarget_lock_try_acquire_recursive (_LOCK_T lock) {
    lock_exit("__retarget_lock_try_acquire_recursive() not implemented");
    return 0;
}
void __retarget_lock_acquire_recursive (_LOCK_T lock) {
    malt_inc_recursive_mutex_lock(lock);
}
void __retarget_lock_release_recursive (_LOCK_T lock) {
    malt_inc_recursive_mutex_unlock(lock);
}

