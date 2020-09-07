#ifndef WORKER_H
#define WORKER_H

#ifdef __cplusplus
extern "C" {
#endif

extern void *worker_task (void *args);
extern int zmj_zmq_init(void);


#ifdef __cplusplus
}
#endif

#endif // WORKER_H
