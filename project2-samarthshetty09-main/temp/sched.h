#define EXPDISTSCHED 1
#define LINUXSCHED 2
#define DEFAULTSCHD 0

void setschedclass (int sched_class);
int getschedclass();
void start_new_epoch();

extern int currClassSchd;
