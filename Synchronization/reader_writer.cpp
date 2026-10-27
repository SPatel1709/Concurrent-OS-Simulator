#include <iostream>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include "cond.c"

using namespace std;

Cond_t cv;
int *shared_data;

// shared_data[0] = number of active readers
// shared_data[1] = writer active (0 or 1)
// shared_data[2] = value being read or written

void reader(int id)
{
    for (int i = 0; i < 3; i++)
    {
        cond_lock(&cv);

        while (shared_data[1] == 1)
            cond_wait(&cv);

        shared_data[0]++;
        cout << "Reader " << id << " reads " << shared_data[2] << endl;

        cond_unlock(&cv);

        usleep(200000);

        cond_lock(&cv);
        shared_data[0]--;

        if (shared_data[0] == 0)
            cond_broadcast(&cv);

        cond_unlock(&cv);
        usleep(100000);
    }
}

void writer(int id)
{
    for (int i = 0; i < 3; i++)
    {
        cond_lock(&cv);

        while (shared_data[0] > 0 || shared_data[1] == 1)
            cond_wait(&cv);

        shared_data[1] = 1;
        shared_data[2]++;
        cout << "Writer " << id << " writes " << shared_data[2] << endl;

        cond_unlock(&cv);

        usleep(200000);

        cond_lock(&cv);
        shared_data[1] = 0;
        cond_broadcast(&cv);
        cond_unlock(&cv);

        usleep(100000);
    }
}

int main()
{
    cv = cond_create(IPC_PRIVATE, IPC_PRIVATE);
    cond_init(&cv);

    int shmid = shmget(IPC_PRIVATE, 3 * sizeof(int), IPC_CREAT | 0600);
    if (shmid == -1)
        error("shmget failed");

    shared_data = (int *)shmat(shmid, NULL, 0);
    if (shared_data == (int *)-1)
        error("shmat failed");

    shared_data[0] = 0;
    shared_data[1] = 0;
    shared_data[2] = 0;

    for (int i = 0; i < 5; i++)
    {
        pid_t pid = fork();
        if (pid == -1)
            error("fork failed");

        if (pid == 0)
        {
            if (i < 3)
                reader(i);
            else
                writer(i - 3);

            shmdt(shared_data);
            return 0;
        }
    }

    for (int i = 0; i < 5; i++)
        wait(NULL);

    cout << "Final value = " << shared_data[2] << endl;

    shmdt(shared_data);
    shmctl(shmid, IPC_RMID, NULL);
    cond_destroy(&cv);
    return 0;
}
