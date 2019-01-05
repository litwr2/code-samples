/******************************************************************************\
    malt_mutex.h

    Header file for mutex types and functions
\******************************************************************************/

#ifndef _MALT_MUTEX_H
#define _MALT_MUTEX_H

struct __lock {
    int mx, owner, counter;
};// __attribute__ ((aligned (16)));
typedef struct __lock malt_mutex;
typedef struct __lock *pmalt_mutex;
void malt_inc_mutex_lock(malt_mutex*);
void malt_inc_mutex_unlock(malt_mutex*);
void malt_fe_mutex_lock(malt_mutex*);
void malt_fe_mutex_unlock(malt_mutex*);
void malt_nohw_mutex_lock(malt_mutex*);
void malt_nohw_mutex_unlock(malt_mutex*);
void malt_inc_recursive_mutex_lock(malt_mutex*);
void malt_inc_recursive_mutex_unlock(malt_mutex*);
void malt_nohw_recursive_mutex_lock(malt_mutex*);
void malt_nohw_recursive_mutex_unlock(malt_mutex*);
#endif
