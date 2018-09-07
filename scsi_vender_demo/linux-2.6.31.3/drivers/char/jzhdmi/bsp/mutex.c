/*
 * mutex.c
 *
 *  Created on: Jun 25, 2010
 *  Synopsys Inc.
 *  SG DWC PT02
 */

#include "mutex.h"
#include "../util/log.h"
#include "../util/error.h"

int mutex_Initialize(void* pHandle)
{
	struct semaphore *mutex = (struct semaphore *)pHandle;
	init_MUTEX(mutex);
	return TRUE;
}

int mutex_Destruct(void* pHandle)
{
	/* do nothing */
	return TRUE;
}

int mutex_Lock(void* pHandle)
{
	struct semaphore *mutex = (struct semaphore *)pHandle;

	down(mutex);
	return TRUE;
}

int mutex_Unlock(void* pHandle)
{
	struct semaphore *mutex = (struct semaphore *)pHandle;

	up(mutex);
	return TRUE;
}
