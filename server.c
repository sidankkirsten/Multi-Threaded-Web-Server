/* CSci4061 Fall 2018 Project 3
* Name: Ziwen Song, Kirsten Qi
* X500: song0164, qixxx259
* Group ID: 110 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include "util.h"
#include <stdbool.h>
#include <unistd.h>

#define MAX_THREADS 100
#define MAX_queue_len 100
#define MAX_CE 100
#define INVALID -1
#define BUFF_SIZE 1024
#define MAX_FILENAME 64

/*
  THE CODE STRUCTURE GIVEN BELOW IS JUST A SUGESSTION. FEEL FREE TO MODIFY AS NEEDED
*/

// structs:
typedef enum slot_status {
	SLOT_FULL = 0,
	SLOT_EMPTY = 1
} SLOT_STATUS;

typedef struct request_queue {
  int fd;
  char *request;
} request_t;

typedef struct cache_entry {
  int len;
  char *request;
  char *content;
  int hit_times;
  int status;
} cache_entry_t;

cache_entry_t **cache_array;
request_t *request_array = NULL;
int thread_counter[MAX_THREADS];

int fill = 0;
int use = 0;
int count = 0;  // count the number of the requests

int queue_length = 0;
int cache_size = 0;

pthread_mutex_t mutex_1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_dispatch = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_worker = PTHREAD_COND_INITIALIZER;


/* ************************ Dynamic Pool Code ***********************************/
// Extra Credit: This function implements the policy to change the worker thread pool dynamically
// depending on the number of requests
void * dynamic_pool_size_update(void *arg) {
  while(1) {
    // Run at regular intervals
    // Increase / decrease dynamically based on your policy
  }
}
/**********************************************************************************/

/* ************************************ Cache Code ********************************/

// Function to check whether the given request is present in cache
int getCacheIndex(char *request) {
  // return the index if the request is present in the cache
  int i;
  for (i = 0; i < cache_size; i++) {
    if (strcmp(cache_array[i]->request, request) == 0) {
      cache_array[i]->hit_times++;
      return i;
    }
  }

  return -1;
}

// Function to check if there is a empty slot in cache
int getEmptySlot() {
  int i;
  for(i = 0; i < cache_size; i++) {
    if(cache_array[i]->status == SLOT_EMPTY) return i;
  }

  return -1;
}

void addIntoCacheHelp(int idx, char *mybuf, char *memory , int memory_size) {
  // allocate memory for each cache entry
  cache_array[idx] = (cache_entry_t *) malloc (sizeof(cache_entry_t));
  cache_array[idx]->request = malloc(strlen(mybuf)+1);
  cache_array[idx]->content = malloc(memory_size+1);

  // add the request and its file content into cache
  cache_array[idx]->len = memory_size;
  memcpy(cache_array[idx]->request, mybuf, strlen(mybuf)+1);
  memcpy(cache_array[idx]->content, memory,memory_size+1);
  cache_array[idx]->hit_times = 0;
  cache_array[idx]->status = SLOT_FULL;
}

// Function to add the request and its file content into the cache
void addIntoCache(char *mybuf, char *memory , int memory_size) {
  // It should add the request at an index according to the cache replacement policy
  // Make sure to allocate/free memeory when adding or replacing cache entries
  int index = 0;
  int cache_index;

  if ((index = getEmptySlot()) != INVALID) { 	// cache holds less requests than capacity
    free(cache_array[index]->content);
    free(cache_array[index]->request);
    free(cache_array[index]);

    addIntoCacheHelp(index, mybuf, memory, memory_size);
  }
  else {		// cache is full
    int least_frequent_index = 0;
    int least_frequent = cache_array[least_frequent_index]->hit_times;

		int j;
    for (j = 0; j < cache_size;j++) {
			int hit_times = cache_array[j]->hit_times;
      if (least_frequent > hit_times) {
				least_frequent = hit_times;
        least_frequent_index = j;
      }
    }

    if (cache_array[least_frequent_index] != NULL) {
			free(cache_array[least_frequent_index]->content);
      free(cache_array[least_frequent_index]->request);
      free(cache_array[least_frequent_index]);
		}
    addIntoCacheHelp(least_frequent_index, mybuf, memory, memory_size);
  }
}

// clear the memory allocated to the cache
void deleteCache() {
  // De-allocate/free the cache memory
  int i;
  for(i = 0; i < cache_size; i++) {
    free(cache_array[i]->content);
    free(cache_array[i]->request);
    free(cache_array[i]);
  }

  free(cache_array);
}

// Function to initialize the cache
void initCache() {
  // Allocating memory and initializing the cache array
  cache_array = (cache_entry_t **) malloc(sizeof(cache_entry_t *) * cache_size);

  int i;
  for (i = 0; i < cache_size; i++) {
    cache_array[i] = (cache_entry_t *) malloc(sizeof(cache_entry_t));
    cache_array[i]->len = 0;
    cache_array[i]->request = (char *) malloc(BUFF_SIZE);
    cache_array[i]->content = (char *) malloc(BUFF_SIZE);
    cache_array[i]->hit_times = 0;
    cache_array[i]->status = SLOT_EMPTY;
  }
}

// Function to open and read the file from the disk into the memory
// return the size of the file
int readFromDisk(char *path, char **buffer) {
  // Open and read the contents of file given the request
  FILE *fp;
  char *buf;
  int buffer_size;

	char filename[MAX_FILENAME];
	sprintf(filename, ".%s", path);

  // open the file
  if ((fp = fopen(filename, "r")) == NULL) {
    return -1;
  }

  // get the number of bytes of the file
  fseek(fp, 0, SEEK_END);
  buffer_size = ftell(fp);

  fseek(fp, 0, SEEK_SET);

  // allocate memory for the buffer to hold the file
  buf = (char *) malloc(sizeof(char) * buffer_size);

  // read the file into buffer
  if (fread(buf, 1, buffer_size, fp) != buffer_size) {
		return -1;
	}
  fclose(fp);

  *buffer = buf;

	// add the file into cache
  addIntoCache(path, buf, buffer_size);

  free(buf);

  return buffer_size;
}

/**********************************************************************************/

/* ************************************ Utilities ********************************/
// Function to get the content type from the request
char* getContentType(char *mybuf) {
  // Should return the content type based on the file type in the request
  // (See Section 5 in Project description for more details)
  char *content_type;
  char **strings;
  int args = makeargv(mybuf, ".", &strings);

  if (args < 2) exit(1);

  if (strcmp("html", strings[1]) == 0)
    content_type = "text/html";
  else if (strcmp("jpg", strings[1]) == 0)
    content_type = "image/jpeg";
  else if (strcmp("gif", strings[1]) == 0)
    content_type = "image/gif";
  else
    content_type = "text/plain";

  freemakeargv(strings);

  return content_type;
}

// This function returns the current time in microseconds
long getCurrentTimeInMicro() {
  struct timeval curr_time;
  gettimeofday(&curr_time, NULL);
  return curr_time.tv_sec * 1000000 + curr_time.tv_usec;
}

/**********************************************************************************/
// Function to add the request in a queue
void addRequestIntoQueue(int fd, char *request, int queue_length) {
  request_array[fill].fd = fd;
  request_array[fill].request = request;
  fill = (fill + 1) % queue_length;
  count++;
}

// Function to get the request from a queue
request_t *getRequestFromQueue(int queue_length) {
  request_t *request_entry;
  request_entry->fd = request_array[use].fd;
  request_entry->request = request_array[use].request;
  use = (use + 1) % queue_length;
  count--;

  return request_entry;
}

void writeRequestLog(int threadId, int reqNum, int fd, char *request, int len, int interval, int flag) {
  FILE *fp;
  char buf[BUFF_SIZE];

	if (len != INVALID) {		// the file from the request was opened and read correctly
		sprintf(buf, "[%d][%d][%d][%s][%d][%d us]", threadId, reqNum, fd, request, len, interval);
	}
	else {
		sprintf(buf, "[%d][%d][%d][%s][Requested file not found.][%d us]", threadId, reqNum, fd, request, interval);
	}

	// check if the request was found in the cache
  if (flag) {
    strcat(buf, "[HIT]\n");
  }
  else {
    strcat(buf, "[MISS]\n");
  }

	// open "web_server_log" to write
  if ((fp = fopen("web_server_log", "a")) == NULL) {
		exit(1);
	}

	// write log to file
  if (fwrite(buf, 1, strlen(buf), fp) != strlen(buf)) {
		perror("write failed");
		exit(1);
	}
  fclose(fp);

  printf("%s", buf);
}

// Function to receive the request from the client and add to the queue
void * dispatch(void *arg) {
  int num_dispatcher = *(int *) arg;

  while (1) {
    int i;
    for (i = 0; i < num_dispatcher; i++) {
      // Accept client connection
      int fd = accept_connection();

      // Get request from the client
      char *filename;
      get_request(fd, filename);

      pthread_mutex_lock(&mutex_1);

      // wait if buffers are currently filled
      while (count == queue_length) pthread_cond_wait(&cond_dispatch, &mutex_1);

      // Add the request into the queue
      addRequestIntoQueue(fd, filename, queue_length);

      pthread_cond_signal(&cond_worker);
      pthread_mutex_unlock(&mutex_1);
    }
   }
   return NULL;
}

/**********************************************************************************/

// Function to retrieve the request from the queue, process it and then return a result to the client
void * worker(void *arg) {
  int num_workers = *(int *) arg;

  while (1) {
    int i;
    for (i = 0; i < num_workers; i++) {
      pthread_mutex_lock(&mutex_1);
      // wait if buffers are currently empty
      while (count == 0) pthread_cond_wait(&cond_worker, &mutex_1);

      // Get the request from the queue
      request_t *request_entry = getRequestFromQueue(queue_length);
      int fd = request_entry->fd;
      char *request = request_entry->request;

      pthread_cond_signal(&cond_dispatch);
      pthread_mutex_unlock(&mutex_1);

			int idx;
			int len;
			char *content;
			int start, end, interval;
			int isHit = 1;

      pthread_mutex_lock(&mutex_2);

      // Start recording time
      start = getCurrentTimeInMicro();

      // Get the data from the disk or the cache
      if ((idx = getCacheIndex(request)) == INVALID) {	// the data is not found in the cache
        isHit = 0;
				len = readFromDisk(request, &content);	// read the data from the disk

        if (len == INVALID) {		// there is a problem with accessing the file
					content = "Requested file not found.";
				}
				else {
					idx = getCacheIndex(request);
					len = cache_array[idx]->len;
					content = cache_array[idx]->content;
					cache_array[idx]->hit_times--;
				}
      }
			else {	// the data is found in the cache
				len = cache_array[idx]->len;
				content = cache_array[idx]->content;
			}

      // number of requests a specific worker thread has handled
      thread_counter[i]++;

      // Stop recording the time
      end = getCurrentTimeInMicro();

      // Log the request into the file and terminal
      interval = end - start;
      writeRequestLog(i, thread_counter[i], fd, request, len, interval, isHit);

      pthread_mutex_unlock(&mutex_2);

      // return the result
      if (len == INVALID) {
				return_error(fd, content);
			}
			else {
				char *content_type = getContentType(request);
				return_result(fd, content_type, content, len);
			}

    }

  }
  return NULL;
}

/**********************************************************************************/

int main(int argc, char **argv) {
  // Error check on number of arguments
  if(argc != 8){
    printf("usage: %s port path num_dispatcher num_workers dynamic_flag queue_length cache_size\n", argv[0]);
    return -1;
  }

  // Get the input args
  int port = atoi(argv[1]);
  char *root_path = argv[2];
  int num_dispatcher = atoi(argv[3]);
  int num_workers = atoi(argv[4]);
  int dynamic_flag = atoi(argv[5]);
  queue_length = atoi(argv[6]);
  cache_size = atoi(argv[7]);

  // Perform error checks on the input arguments
  if (port < 1025 || port > 65535) return -1;

  if (num_dispatcher > MAX_THREADS) {
    printf("Invalid number of dispatch threads.\n");
    exit(1);
  }

  if (num_workers > MAX_THREADS) {
    printf("Invalid number of worker threads.\n");
    exit(1);
  }

  if (queue_length > MAX_queue_len) {
    printf("Invalid queue_len size.\n");
    exit(1);
  }

  if (cache_size > MAX_CE) {
    printf("Invalid cache_size.\n");
    exit(1);
  }

	printf("Starting server on port: %d: %d disp, %d work\n", port, num_dispatcher, num_workers);

  // Change the current working directory to server root directory
  chdir(root_path);

  FILE *fp = fopen("web_server_log", "w");
  if (fp == NULL) {
    exit(1);
  }

  // Start the server and initialize cache
  init(port);
  initCache();

  request_array = (request_t *) malloc(sizeof(request_t) * queue_length);

  // Create dispatcher and worker threads
  pthread_t dispatch_tid, worker_tid;
  pthread_create(&dispatch_tid, NULL, dispatch, (void *) &num_dispatcher);
  pthread_create(&worker_tid, NULL, worker, (void *) &num_workers);

  pthread_join(dispatch_tid, NULL);
  pthread_join(worker_tid, NULL);

  // Clean up
  pthread_cond_destroy(&cond_dispatch);
  pthread_cond_destroy(&cond_worker);
  pthread_mutex_destroy(&mutex_1);
  pthread_mutex_destroy(&mutex_2);

  deleteCache();

  return 0;
}
