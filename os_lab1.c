#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
bool ready = false;
int n = 0;
const int flag = 3;

void* provider(void* arg) {
    while (n < flag) {
        sleep(1);
        pthread_mutex_lock(&lock);
        
        if (!ready) {
            ready = true;
            n++;
            printf("sent\n");
            pthread_cond_signal(&cond);
        }
        
        pthread_mutex_unlock(&lock);
    }
    return 0;
}

void* consumer(void* arg) {
    int p = 0;
    while (p < flag) {
        pthread_mutex_lock(&lock);
        
        while (!ready) {
            pthread_cond_wait(&cond, &lock);
        }
        
        printf("received\n");
        ready = false;
        p++;   
        pthread_mutex_unlock(&lock);
    }
    return 0;
}

int main() {
    pthread_t provider_thread, consumer_thread;
    
    pthread_create(&provider_thread, NULL, provider, NULL);
    pthread_create(&consumer_thread, NULL, consumer, NULL);
    
    pthread_join(provider_thread, NULL);
    pthread_join(consumer_thread, NULL);
    
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);
    
    printf("program finished");
    
    return 0;
}