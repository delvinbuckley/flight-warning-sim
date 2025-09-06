//Delvin Buckley
//Program 8

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int condFlag = 0; //Condition flag

struct node {
    //Create node that displays warning
    pthread_t threadNum;
    char name[10];
    struct node *next;
};

struct node *head = NULL;

void addNode(struct node** head, pthread_t t_Num, char name[]){
    //Add warning node to list
    struct node *newNode = malloc(sizeof(struct node));
    if (!newNode) {
        perror("Failed to allocate memory for new node");
        exit(EXIT_FAILURE);
    }
    newNode->threadNum = t_Num;
    strncpy(newNode->name, name, sizeof(newNode->name));
    newNode->next = *head;
    *head = newNode;
    // Wake consumer whenever a new node is added
    pthread_mutex_lock(&mutex1);
    condFlag = 1;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex1);
}

void printNode(struct node* list) {
    //Display warning nodes
    while (list != NULL) {
        printf("Warning in thread %lu. ", (unsigned long)list->threadNum);
        printf("%s is out of bound!\n", list->name);
        list = list->next;
    }
}

void* producer1(void* arg) {
    //Producer Thread 1
    (void)arg;
    int fd1;
    ssize_t numRead;
    double pitch;
    double roll;
    double yaw;

    if ((fd1 = open("accl1.dat", O_RDONLY)) == -1) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    } 
    while (1) {
        pthread_mutex_lock(&mutex2);
        numRead = read(fd1, &pitch, sizeof(double));
        numRead = read(fd1, &roll, sizeof(double));
        numRead = read(fd1, &yaw, sizeof(double));
        pthread_mutex_unlock(&mutex2);

        if (numRead == -1) {
            break;
        }
        if (pitch > 20.0 || pitch < -20.0) {
            addNode(&head, pthread_self(), "pitch");
        }
        if (roll > 20.0 || roll < -20.0) {
            addNode(&head, pthread_self(), "roll");
        }
        sleep(1);
    }
    close(fd1);
}

void* producer2(void* arg) {
    //Producer Thread 2
    (void)arg;
    int fd2;
    ssize_t numRead;
    double pitch;
    double roll;
    double yaw;

    if ((fd2 = open("accl2.dat", O_RDONLY)) == -1) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    } 
    while (1) {
        pthread_mutex_lock(&mutex2);
        numRead = read(fd2, &pitch, sizeof(double));
        numRead = read(fd2, &roll, sizeof(double));
        numRead = read(fd2, &yaw, sizeof(double));
        pthread_mutex_unlock(&mutex2);

        if (numRead == -1) {
            break;
        }
        if (pitch > 20.0 || pitch < -20.0) {
            addNode(&head, pthread_self(), "pitch");
        }
        if (roll > 20.0 || roll < -20.0) {
            addNode(&head, pthread_self(), "roll");
        }
        sleep(1);
    }
    close(fd2);
}

void* consumer(void* arg) {
    //Consumer Thread
    (void)arg;
    for (;;) {
        pthread_mutex_lock(&mutex1);
        while (condFlag == 0) {
            pthread_cond_wait(&cond, &mutex1);
        }
        printNode(head); // Print the list of warnings
        freeList(&head); // Free the list after printing
        condFlag = 0; // Reset condition flag
        pthread_mutex_unlock(&mutex1);
    }
}

void freeList(struct node** head) {
    struct node* current = *head;
    struct node* next;

    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }

    *head = NULL; // reset head pointer
}

int main (int agrc, char* argv[]) {

    pthread_t pt1, pt2, ct1;

    if (pthread_create(&pt1, NULL, producer1, NULL) != 0) {
        perror("pthread pt1");
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&pt2, NULL, producer2, NULL) != 0) {
        perror("pthread pt2");
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&ct1, NULL, consumer, NULL) != 0) {
        perror("pthread ct1");
        exit(EXIT_FAILURE);
    } 
    if (pthread_join(pt1, NULL) != 0) {
        perror("pthread join pt1");
        exit(EXIT_FAILURE);
    }
    if (pthread_join(pt2, NULL) != 0) {
        perror("pthread join pt2");
        exit(EXIT_FAILURE);
    }
    if (pthread_cancel(ct1) != 0) {
        perror("pthread cancel");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}