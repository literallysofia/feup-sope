
char * fifoNames[4] = {"/tmp/fifoN","/tmp/fifoS","/tmp/fifoE","/tmp/fifoO"};


typedef struct {
    int id;
   	double parked_time;
    char * entryFifo;
    char vFifo[19];
} Vehicle;

typedef struct {
    int id;
    char * fifoName;
} Entry;

typedef struct {
	clock_t t;
	int id;
	char dest;
	double parkedT;
	clock_t lifetime;
	char * obs;
} gLog;

/*
void mySleep(double ticks) {
	float ticksPS  = sysconf(_SC_CLK_TCK);
    double seconds = (double)ticks / ticksPS;
    struct timespec req;
    req.tv_sec = (time_t) (seconds);
    req.tv_nsec = (long) (seconds * pow(10,9) - req.tv_sec * pow(10,9));
    nanosleep(&req, NULL);
}
*/