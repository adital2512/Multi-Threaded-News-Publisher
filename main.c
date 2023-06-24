// Adi Tal 314835703
// configuration format:
//      1
//      2
//      3
//
//      2
//      2
//      4
//
//      5

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>

typedef struct {
    char** buffer;
    int inIndex;
    int outIndex;
    int size;
    int id;
    int done;
    pthread_mutex_t mutex;
    sem_t semEmpty;
    sem_t semFull;
} BoundedBuffer;

typedef struct Node {
    char* data;
    struct Node* next;
} Node;

typedef struct {
    Node* list;
    pthread_mutex_t mutex;
    sem_t items;
} UnboundedBuffer;

typedef struct {
    int numProducts;
    BoundedBuffer buffer;
} Producer;

typedef struct {
    int size;
    Producer* prodList;
    UnboundedBuffer* ubSport;
    UnboundedBuffer* ubNews;
    UnboundedBuffer* ubWeather;
} dispatcherArgs;

typedef struct {
    char* type;
    UnboundedBuffer* ub;
    BoundedBuffer* bb;
} consumeArgs;

void pull_string_unBounded(UnboundedBuffer* buffer, char* result) {
    sem_wait(&buffer->items);
    pthread_mutex_lock(&buffer->mutex);
    if (!buffer->list->next) {
        strcpy(result, buffer->list->data);
        free(buffer->list->data);
        free(buffer->list);
        buffer->list = NULL;
    } else {
        Node* iter1 = buffer->list;
        Node* iter2 = buffer->list->next;
        while (iter2->next) {
            iter1 = iter2;
            iter2 = iter1->next;
        }
        strcpy(result, iter2->data);
        free(iter2->data);
        free(iter2);
        iter1->next = NULL;
    }
    pthread_mutex_unlock(&buffer->mutex);
}

void push_string_unBounded(UnboundedBuffer* buffer, char* string) {
    pthread_mutex_lock(&buffer->mutex);
    Node* list = buffer->list;
    buffer->list = malloc(sizeof(Node));
    buffer->list->data = malloc(sizeof(char) * 128);
    strcpy(buffer->list->data, string);
    free(string);
    buffer->list->next = list;
    sem_post(&buffer->items);
    pthread_mutex_unlock(&buffer->mutex);
}

void create_unBounded_buffer(UnboundedBuffer* buffer) {
    buffer->list = NULL;
    pthread_mutex_init(&buffer->mutex, NULL);
    sem_init(&buffer->items, 0, 0);
}

void destroy_unBounded_buffer(UnboundedBuffer* buffer) {
    pthread_mutex_destroy(&buffer->mutex);
    sem_destroy(&buffer->items);
}

void destroy_bounded_buffer(BoundedBuffer* boundedBuffer) {
    for (int i = 0; i < boundedBuffer->size; i++) {
        free(boundedBuffer->buffer[i]);
    }
    free(boundedBuffer->buffer);
    pthread_mutex_destroy(&boundedBuffer->mutex);
    sem_destroy(&boundedBuffer->semEmpty);
    sem_destroy(&boundedBuffer->semFull);
}

void create_bounded_buffer(BoundedBuffer* boundedBuffer, int size, int id) {
    boundedBuffer->size = size;
    boundedBuffer->id = id;
    boundedBuffer->done = 0;
    boundedBuffer->buffer = malloc(size * sizeof(char*));
    for (int i = 0; i < size; i++) {
        boundedBuffer->buffer[i] = malloc(128 * sizeof(char));
    }
    boundedBuffer->inIndex = 0;
    boundedBuffer->outIndex = 0;
    pthread_mutex_init(&boundedBuffer->mutex, NULL);
    sem_init(&boundedBuffer->semEmpty, 0, size);
    sem_init(&boundedBuffer->semFull, 0, 0);
}

void pull_string_bounded(BoundedBuffer* boundedBuffer, char* result) {
    sem_wait(&boundedBuffer->semFull);
    pthread_mutex_lock(&boundedBuffer->mutex);
    strcpy(result, boundedBuffer->buffer[boundedBuffer->outIndex]);
    boundedBuffer->outIndex = (boundedBuffer->outIndex + 1) % boundedBuffer->size;
    pthread_mutex_unlock(&boundedBuffer->mutex);
    sem_post(&boundedBuffer->semEmpty);
}

void push_string_bounded(BoundedBuffer* boundedBuffer, char* str) {
    sem_wait(&boundedBuffer->semEmpty);
    pthread_mutex_lock(&boundedBuffer->mutex);
    strcpy(boundedBuffer->buffer[boundedBuffer->inIndex], str);
    free(str);
    boundedBuffer->inIndex = (boundedBuffer->inIndex + 1) % boundedBuffer->size;
    pthread_mutex_unlock(&boundedBuffer->mutex);
    sem_post(&boundedBuffer->semFull);
}

void addProd(Producer** list, int numProducts, int queueSize, int* size) {
    (*size)++;
    Producer* newList = realloc(*list, (*size) * sizeof(Producer));
    if (newList == NULL) {
        printf("Failed to allocate memory for the new producer.\n");
        return;
    }
    *list = newList;
    create_bounded_buffer(&((*list)[*size - 1].buffer), queueSize, (*size) - 1);
    (*list)[*size - 1].numProducts = numProducts;
}

void readConfigurationFile(char* filename, Producer** prodList, int* sizeProdList, int* coSize)
{
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Failed to open file.\n");
        exit(1);
    }

    char line[100];
    int prodNumber, articleCount, queueSize;
    int coEditorSize = 0;

    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "%d", &prodNumber);
        fgets(line, sizeof(line), file);
        sscanf(line, "%d", &articleCount);
        fgets(line, sizeof(line), file);
        sscanf(line, "%d", &queueSize);

        if (fgets(line, sizeof(line), file) == NULL) {
            break;
        } else {
            if (queueSize < 1) {
                printf("queue size must be > 0, ignoring this producers\n");
            } else {
                addProd(prodList, articleCount, queueSize, sizeProdList);
            }
        }
    }

    if (sscanf(line, "%d", &coEditorSize) == 1) {
        if (coEditorSize < 1) {
            printf("queue size must be > 0, exiting\n");
            exit(1);
        } else {
            *coSize = coEditorSize;
        }
    }

    fclose(file);
}

void* producer(void* args) {
    Producer* producer = (Producer*) args;
    int indexProducer = producer->buffer.id;

    for (int i = 0; i < producer->numProducts; i++) {
        int indexProduct = i;
        char* string = malloc(sizeof(char) * 128);

        int random = rand() % 3;
        if (random == 0) {
            sprintf(string, "Producer %d SPORTS %d", indexProducer, indexProduct );
        } else if (random == 1) {
            sprintf(string, "Producer %d WEATHER %d", indexProducer, indexProduct);
        } else {
            sprintf(string, "Producer %d NEWS %d", indexProducer, indexProduct);
        }
        push_string_bounded(&producer->buffer, string);
    }

    char* done = malloc(sizeof(char) * 10);
    strcpy(done, "DONE");
    push_string_bounded(&producer->buffer, done);
    return NULL;
}

void* dispatcher(void* args) {
    dispatcherArgs* da = (dispatcherArgs*) args;
    int sizeProdList = da->size;
    Producer* prodList = da->prodList;
    int countDown = sizeProdList;

    while (countDown) {
        for (int i = 0; i < sizeProdList; i++) {
            if (!prodList[i].buffer.done) {
                char* string = malloc(sizeof(char) * 128);
                pull_string_bounded(&prodList[i].buffer, string);
                if (strcmp(string, "DONE") == 0) {
                    prodList[i].buffer.done = 1;
                    --countDown;
                    free(string);
                } else {
                    int producerId;
                    char producerType[10];
                    int producerData;
                    int matched = sscanf(string, "Producer %d %s %d", &producerId, producerType, &producerData);
                    if (matched == 3) {
                        if (strcmp(producerType, "SPORTS") == 0 ) {
                            push_string_unBounded(da->ubSport, string);
                        } else if(strcmp(producerType, "NEWS") == 0) {
                            push_string_unBounded(da->ubNews, string);
                        } else {
                            push_string_unBounded(da->ubWeather, string);
                        }
                    }
                }
            }
        }
    }

    char* done1 = malloc(sizeof(char) * 10);
    strcpy(done1, "DONE");
    push_string_unBounded(da->ubWeather, done1);

    char* done2 = malloc(sizeof(char) * 10);
    strcpy(done2, "DONE");
    push_string_unBounded(da->ubNews, done2);

    char* done3 = malloc(sizeof(char) * 10);
    strcpy(done3, "DONE");
    push_string_unBounded(da->ubSport, done3);
    return NULL;
}

void* consume(void* args) {
    consumeArgs* ca = (consumeArgs*) args;
    UnboundedBuffer* ub = ca->ub;
    BoundedBuffer* bb = ca->bb;
//    char* type = ca->type;
    int done = 0;
    while(!done) {
        char* string = malloc(sizeof(char) * 128);
        pull_string_unBounded(ub, string);
        if(!strcmp(string, "DONE")) {
            push_string_bounded(bb, string);
            done = 1;
        } else {
            push_string_bounded(bb, string);
        }
    }
    return NULL;
}

void* screenManager(void* args) {
    int x = 0;
    BoundedBuffer* bb = (BoundedBuffer*) args;
    int count = 3;
    char* res = malloc(sizeof(char) * 128);
    while (count) {
        pull_string_bounded(bb, res);
        if(strcmp(res, "DONE") == 0) {
            count--;
        } else {
            printf("%s\n", res);
            x++;
        }
    }
    free(res);
    return NULL;
}

int main(int argc, char* argv[]) {

    if (argc != 2) {
        printf("wrong count of arguments. exiting\n");
        exit(1);
    }

    Producer* prodList = malloc(sizeof(Producer));
    prodList[0].numProducts = 0;
    create_bounded_buffer(&prodList[0].buffer, 1, 0);
    int sizeProdList = 1;
    int coEditorSize;
    readConfigurationFile(argv[1], &prodList, &sizeProdList, &coEditorSize);

    if (sizeProdList == 1) {
        printf("no valid producers found, exiting\n");
        exit(1);
    }

    pthread_t* producerThreads = malloc(sizeProdList * sizeof(pthread_t));

    for (int i = 0; i < sizeProdList; i++) {
        if (pthread_create(&producerThreads[i], NULL, producer, (void*)&prodList[i]) != 0) {
            printf("Failed to create producer thread %d.\n", i);
        }
    }

    BoundedBuffer* coEditorBuffer = malloc(sizeof(BoundedBuffer));
    create_bounded_buffer(coEditorBuffer, 3, -1);

    pthread_t ScreenThread;
    if (pthread_create(&ScreenThread, NULL, screenManager, (void*)coEditorBuffer) != 0) {
        printf("Failed to create screen thread.\n");
    }

    UnboundedBuffer* ubSports = malloc(sizeof(UnboundedBuffer));
    create_unBounded_buffer(ubSports);
    consumeArgs* caSports = malloc(sizeof(consumeArgs));
    caSports->type = malloc(sizeof(char) * 10);
    strcpy(caSports->type, "sports");
    caSports->ub = ubSports;
    caSports->bb = coEditorBuffer;
    pthread_t sportsThread;
    if (pthread_create(&sportsThread, NULL, consume, (void*)caSports) != 0) {
        printf("Failed to create sports thread.\n");
    }

    UnboundedBuffer* ubNews = malloc(sizeof(UnboundedBuffer));
    create_unBounded_buffer(ubNews);
    consumeArgs* caNews = malloc(sizeof(consumeArgs));
    caNews->type = malloc(sizeof(char) * 10);
    strcpy(caNews->type, "news");
    caNews->ub = ubNews;
    caNews->bb = coEditorBuffer;
    pthread_t newsThread;
    if (pthread_create(&newsThread, NULL, consume, (void*)caNews) != 0) {
        printf("Failed to create news thread.\n");
    }

    UnboundedBuffer* ubWeather = malloc(sizeof(UnboundedBuffer));
    create_unBounded_buffer(ubWeather);
    consumeArgs* caWeather = malloc(sizeof(consumeArgs));
    caWeather->type = malloc(sizeof(char) * 10);
    strcpy(caWeather->type, "weather");
    caWeather->ub = ubWeather;
    caWeather->bb = coEditorBuffer;
    pthread_t weatherThread;
    if (pthread_create(&weatherThread, NULL, consume, (void*)caWeather) != 0) {
        printf("Failed to create weather thread.\n");
    }

    pthread_t dispatcherThread;
    dispatcherArgs* da = malloc(sizeof(dispatcherArgs));
    da->prodList = prodList;
    da->size = sizeProdList;
    da->ubWeather = ubWeather;
    da->ubSport = ubSports;
    da->ubNews = ubNews;
    if (pthread_create(&dispatcherThread, NULL, dispatcher, (void*)da) != 0) {
        printf("Failed to create dispatcher thread.\n");
    }

    if (pthread_join(ScreenThread, NULL) != 0) {
        printf("Failed to join screen thread.\n");
    }

    for (int i = 0; i < sizeProdList; i++) {
        if (pthread_join(producerThreads[i], NULL) != 0) {
            printf("Failed to join producer thread %d.\n", i);
        }
    }

    if (pthread_join(dispatcherThread, NULL) != 0) {
        printf("Failed to join dispatcher thread.\n");
    }

    if (pthread_join(sportsThread, NULL) != 0) {
        printf("Failed to join sportsThread thread.\n");
    }

    if (pthread_join(newsThread, NULL) != 0) {
        printf("Failed to join newsThread thread.\n");
    }

    if (pthread_join(weatherThread, NULL) != 0) {
        printf("Failed to join sportsThread thread.\n");
    }

    for (int i = 0; i < sizeProdList; i++) {
        destroy_bounded_buffer(&prodList[i].buffer);
    }
    destroy_bounded_buffer(coEditorBuffer);
    free(coEditorBuffer);
    free(prodList);

    destroy_unBounded_buffer(ubWeather);
    destroy_unBounded_buffer(ubNews);
    destroy_unBounded_buffer(ubSports);

    free(caSports->type);
    free(caSports);
    free(caNews->type);
    free(caNews);
    free(caWeather->type);
    free(caWeather);
    free(producerThreads);
    free(ubWeather);
    free(ubNews);
    free(ubSports);
    free(da);

    return 0;
}

