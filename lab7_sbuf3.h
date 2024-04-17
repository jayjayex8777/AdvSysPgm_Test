#include <linux/semaphore.h>

typedef struct {
        int *buf;               /* buffer array */
        int n;                  /* maximum number of slots */
        int front;              /* buf[(front+1)%n] is first item */
        int rear;               /* buf[rear%n] is last item */
        struct semaphore mutex; /* protects accesses to buf */
        struct semaphore slots; /* counts available slots */
        struct semaphore items; /* counts available items */
} sbuf_t;
