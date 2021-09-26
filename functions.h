#define SIZE 10000
#define MAXFILES 200
#define MAXSIZE 1000

struct pool_t{
    char buffer[MAXFILES][MAXSIZE];
    int b_start;
    int b_end;
    int b_count;
};

struct delays_t{
    int csid;
    int delay;
    struct delays_t *next;
};

pthread_mutex_t mtx;
pthread_cond_t writer_cond;
pthread_cond_t reader_cond;
struct pool_t pool;
int writers;
int readers;

pthread_mutex_t bytesmtx;
pthread_mutex_t filesmtx;
int bytesTransferred;
int filesTransferred;

int numContentDevices;
int numContentsWroteB;
int numrequests;
int numserved;
pthread_mutex_t crbmtx;
pthread_mutex_t reqmtx;
pthread_mutex_t servedmtx;

int AllDone;

void init_pool(struct pool_t *pool);

void writer_lock();
void writer_unlock();

void reader_lock();
void reader_unlock();

void list_dir(const char * dir_name, char *lbuffer);
