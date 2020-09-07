//  Paranoid Pirate queue

#include "czmq.h"
#include "worker.h"
#define HEARTBEAT_LIVENESS  3       //  3-5 is reasonable
#define HEARTBEAT_INTERVAL  1000    //  msecs

//  Paranoid Pirate Protocol constants
#define PPP_READY       "\001"      //  Signals worker is ready
#define PPP_HEARTBEAT   "\002"      //  Signals worker heartbeat

//  .split worker class structure
//  Here we define the worker class; a structure and a set of functions that
//  act as constructor, destructor, and methods on worker objects:

typedef struct {
    zframe_t *identity;         //  Identity of worker
    char *id_string;            //  Printable identity
    int64_t expiry;             //  Expires at this time
} worker_t;

//  Construct new worker
static worker_t *
s_worker_new (zframe_t *identity)
{
    worker_t *self = (worker_t *) zmalloc (sizeof (worker_t));
    self->identity = identity;
    self->id_string = zframe_strhex (identity);
    self->expiry = zclock_time ()
                 + HEARTBEAT_INTERVAL * HEARTBEAT_LIVENESS;
    return self;
}

//  Destroy specified worker object, including identity frame.
static void
s_worker_destroy (worker_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        worker_t *self = *self_p;
        zframe_destroy (&self->identity);
        free (self->id_string);
        free (self);
        *self_p = NULL;
    }
}

//  .split worker ready method
//  The ready method puts a worker to the end of the ready list:

static void
s_worker_ready (worker_t *self, zlist_t *workers)
{
    worker_t *worker = (worker_t *) zlist_first (workers);
    while (worker) {
        if (streq (self->id_string, worker->id_string)) {
            zlist_remove (workers, worker);
            s_worker_destroy (&worker);
            break;
        }
        worker = (worker_t *) zlist_next (workers);
    }
    zlist_append (workers, self);
}

//  .split get next available worker
//  The next method returns the next available worker identity:

static zframe_t *
s_workers_next (zlist_t *workers)
{
    worker_t *worker = zlist_pop (workers);
    assert (worker);
    zframe_t *frame = worker->identity;
    worker->identity = NULL;
    s_worker_destroy (&worker);
    return frame;
}

//  .split purge expired workers
//  The purge method looks for and kills expired workers. We hold workers
//  from oldest to most recent, so we stop at the first alive worker:

static void
s_workers_purge (zlist_t *workers)
{
    worker_t *worker = (worker_t *) zlist_first (workers);
    while (worker) {
        if (zclock_time () < worker->expiry)
            break;              //  Worker is alive, we're done here

        zlist_remove (workers, worker);
        s_worker_destroy (&worker);
        worker = (worker_t *) zlist_first (workers);
    }
}

//  .split main task
//  The main task is a load-balancer with heartbeating on workers so we
//  can detect crashed or blocked worker tasks:

//int main (void)
void *server_task (void *args)
{
    //    void *context = zmq_ctx_new ();
    //    void *requester = zmq_socket (context, ZMQ_REQ);
    void *ctx = zmq_ctx_new ();
    void *frontend = zmq_socket (ctx, ZMQ_ROUTER);
    void *backend = zmq_socket (ctx, ZMQ_ROUTER);
    zmq_bind (frontend, "tcp://*:5555");    //  For clients
    zmq_bind (backend,  "tcp://127.0.0.1:5556");    //  For workers
    printf ("i am sever\n");
    //  List of available workers
    zlist_t *workers = zlist_new ();

    //  Send out heartbeats at regular intervals
    uint64_t heartbeat_at = zclock_time () + HEARTBEAT_INTERVAL;

    while (true) {
        zmq_pollitem_t items [] = {
            { backend,  0, ZMQ_POLLIN, 0 },
            { frontend, 0, ZMQ_POLLIN, 0 }
        };
        //  Poll frontend only if we have available workers
        int rc = zmq_poll (items, zlist_size (workers)? 2: 1,
            HEARTBEAT_INTERVAL * ZMQ_POLL_MSEC);
        if (rc == -1)
            break;              //  Interrupted

       // printf ("-----rc -------%d %d\n",rc);
        //  Handle worker activity on backend
        if (items [0].revents & ZMQ_POLLIN) {
            //  Use worker identity for load-balancing
            zmsg_t *msg = zmsg_recv (backend);
            if (!msg)
                break;          //  Interrupted
            //printf ("i am sever1 msg\n");
            zframe_t *frame = zmsg_first(msg);
           // printf("-client--frame---%s---\n",zframe_data(frame));
             printf("-client--frame-1--%s-size:%d--\n",zframe_data(frame),zframe_size(frame));
           // zmsg_dump (msg);
            //  Any sign of life from worker means it's ready
            zframe_t *identity = zmsg_unwrap (msg);
            worker_t *worker = s_worker_new (identity);
            s_worker_ready (worker, workers);

            //  Validate control message, or return reply to client
            if (zmsg_size (msg) == 1) {
               // printf ("Validate control message\n");
                zframe_t *frame = zmsg_first (msg);
                if (memcmp (zframe_data (frame), PPP_READY, 1)
                &&  memcmp (zframe_data (frame), PPP_HEARTBEAT, 1)) {
                    printf ("E: invalid message from worker");
                    zmsg_dump (msg);
                }
                zmsg_destroy (&msg);
            }else{
               // printf ("return reply to client\n");
                  printf("-client-return reply to clien-frame-1--%s-size:%d--\n",zframe_data(frame),zframe_size(frame));
                 zmsg_dump (msg);
                  zmsg_send (&msg, frontend);

            }
            //zmq_msg_send (zmq_msg_t *msg_, void *s_, int flags_);
           // zmq_msg_recv
        }
        if (items [1].revents & ZMQ_POLLIN) {
             printf ("i am sever1 ZMQ_POLLIN client\n");
            //  Now get next client request, route to next worker
//            zmsg_t *msg = zmsg_recv (frontend);
//            if (!msg)
//                break;          //  Interrupted

             zframe_t *frame = zframe_recv (frontend);
             if (!frame)
                 break;          //  Interrupted

           // zframe_t *frame = zmsg_first(msg);
           // printf("-client--frame---%s---\n",zframe_data(frame));
             printf("-client--frame---%s-size:%d--\n",zframe_data(frame),zframe_size(frame));


//           printf ("Client:recv");
//           zmsg_dump (msg);
//            printf ("Client:recv end\n");
//            zframe_t *identity = s_workers_next (workers);
//            zmsg_prepend (msg, &identity);
//            zmsg_send (&msg, backend);
              zframe_send (&frame, backend, 0);

            //printf ("Client:send");
           // zmsg_dump (msg);
           // printf ("Client:send end\n");
        }
        //  .split handle heartbeating
        //  We handle heartbeating after any socket activity. First, we send
        //  heartbeats to any idle workers if it's time. Then, we purge any
        //  dead workers:
        if (zclock_time () >= heartbeat_at) {
            //printf ("i am sever1 msg zclock_time () >= heartbeat_at\n");
            worker_t *worker = (worker_t *) zlist_first (workers);
            while (worker) {
                zframe_send (&worker->identity, backend,
                             ZFRAME_REUSE + ZFRAME_MORE);
                zframe_t *frame = zframe_new (PPP_HEARTBEAT, 1);
                zframe_send (&frame, backend, 0);
                worker = (worker_t *) zlist_next (workers);
            }
            heartbeat_at = zclock_time () + HEARTBEAT_INTERVAL;
        }
        s_workers_purge (workers);
    } //while
    //  When we're done, clean up properly
    while (zlist_size (workers)) {
        worker_t *worker = (worker_t *) zlist_pop (workers);
        s_worker_destroy (&worker);
    }
    zlist_destroy (&workers);
    zmq_ctx_destroy (&ctx);
   // return 0;
}



int zmj_zmq_init (void)
{

    zmq_threadstart (server_task, NULL);
    // sleep(1);
    zmq_threadstart (worker_task, NULL);
   // zmq_threadstart (worker_task, NULL);
    //zmq_threadstart (worker_task, NULL);

    while(1){
         //printf ("msg\n");
        sleep(1);
    }
}
