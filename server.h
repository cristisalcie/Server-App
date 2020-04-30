#ifndef _SERVER_H
#define _SERVER_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>

/*
 * Error macro checker.
 * Example:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */
#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

// Max dimension of an entire data payload.
#define BUFLEN	      	2000
#define MAX_CLIENTS     5	      // Max waiting clients.


// Bytes offset to get to type field from a payload.
#define TYPE_OFF        50

// Bytes offset to get to message from a payload.
#define MSG_OFF         TYPE_OFF + 1

// Sign offset from payload.
#define SIGN_OFF        MSG_OFF

// Signed value offset from payload.
#define VAL_OFF         SIGN_OFF + 1

#define FLOAT_POW_OFF   VAL_OFF + 4

#define INT_ID          0
#define SHORT_REAL_ID   1
#define FLOAT_ID        2
#define STRING_ID       3

#define INITIAL_CAPACITY 5  // Initial topic capacity.

typedef struct Topic{
    char *name;     //   Name of topic.
    __uint8_t sf;   //   If 1 and subscriber ID disconnected he will receive
                    // when he connects back everything he lost.
                    //   If 0 and subscriber ID disconnected he will not
                    // receive what he lost while he was gone.
} Topic;

typedef struct SubscriberData {
    int sockId;                 // -1 if client disconnected.
    int ID;                     // Client ID, -1 if uninitialized.
    __uint8_t mustBeForwarded;  // Flag to know whether he must be sent msg
                                // when reconnect occurs.
    int capacity_topics;        // Capacity of topics.
    int num_topics;             // Number of topics.
    Topic *topics;              // Array of topics.
} SubscriberData;

typedef struct ClientList {
  SubscriberData *data;
  struct ClientList* next;
} ClientList;

int min(int a, int b);
int ten_to_pow(int power);
int convert_to_ID(char *string);
void storeMsg(int id, char *msg);
void hexDump (const char * desc, const void * addr, const int len);
void init(ClientList **list);
int isEmpty(ClientList *list);
void insert(ClientList **list, SubscriberData *newElement);
long length(ClientList *list);
void deleteOnce(ClientList **list, int id);
void freeList(ClientList **list);
SubscriberData* createSubscriber(int sockId, int id);
int containsID(ClientList *list, int id);
SubscriberData* findID(ClientList *list, int id);
SubscriberData* findSocket(ClientList *list, int sockId);
int connected(ClientList *list, int id);
int isSubscribed(SubscriberData *sub, char *topic, __uint8_t sf);
void subscribe(ClientList **list, int sockId, char *topic, __uint8_t sf);
void unsubscribe(ClientList **list, int sockId, char *topic);
void printSubscribeList(SubscriberData *sub);

int min(int a, int b) {
    return (a < b) ? a : b;
}

int ten_to_pow(int power) {
    if (power == 0) return 1;
    int p = power - 1;
    int rez = 10;
    while (p > 0) {
        rez *= 10;
        p--;
    }
    return rez;
}

int convert_to_ID(char *string) {
    int id = 0;
    int i = 0;
    int len = strlen(string);
    int digit;
    while (len > 0) {
        // In case it is not an ascii char between 0-9.
        digit = abs(string[i] - 48);
        while (digit > 9) {
            digit /= 10;
        }
        id += (string[i] - 48) * ten_to_pow(len - 1);
        i++;        
        len--;
    }
    return id;
}

void storeMsg(int id, char *msg) {
    char file_name[50];
    sprintf(file_name, "sub%d.sf", id);

    FILE *fp = fopen(file_name, "a");
    DIE(fp == NULL, "fopen");
    
    fprintf(fp, "%s\n", msg);
    
    fclose(fp);
}

void init(ClientList **list) {
  *list = NULL;
}

/**
 * Returns 1 if true and 0 false.
 */
int isEmpty(ClientList *list) {
  return list != NULL ? 0 : 1;
}

void insert(ClientList **list, SubscriberData *newElement) {
  ClientList *s1 = (ClientList*) malloc(sizeof(ClientList));
  s1->data = newElement;
  s1->next = NULL;

  if (isEmpty(*list)) {
    *list = s1;
    return;
  }

  if (newElement->sockId < (*list)->data->sockId) {
    s1->next = *list;
    *list = s1;
  } else {

    ClientList *head = *list;
    ClientList *prev = NULL;
    while (!isEmpty(*list)) {
      if (newElement->sockId < (*list)->data->sockId) {
        break;
      }
      prev = *list;
      *list = (*list)->next;
    }

    *list = prev;
    s1->next = (*list)->next;
    (*list)->next = s1;

    *list = head;
  }
}

long length(ClientList *list) {
  long len = 0;
  while (!isEmpty(list)) {
    len++;
    list = list->next;
  }
  return len;
}

void deleteOnce(ClientList **list, int id) {
  char containsElem = containsID(*list, id);
  if (containsElem == 1) {
    ClientList *tmp, *head = *list;

    while(!isEmpty(*list)) {
      if ((*list)->data->ID == id) {
        tmp = *list;
        *list = (*list)->next;
        head = *list;
        for (int i = 0; i < tmp->data->num_topics; i++) {
            free(tmp->data->topics[i].name);
        }
        free(tmp->data->topics);
        free(tmp->data);
        free(tmp);
        break;
      }
      
      if ((*list)->next != NULL) {
        if ((*list)->next->data->ID == id) {
          tmp = (*list)->next;
          (*list)->next = (*list)->next->next;
          for (int i = 0; i < tmp->data->num_topics; i++) {
              free(tmp->data->topics[i].name);
            }
          free(tmp->data->topics);
          free(tmp->data);
          free(tmp);
          break;
        }
      }
      *list = (*list)->next;
    }
    
    *list = head;
  }
}

void freeList(ClientList **list) {
  while (!isEmpty(*list)) {
    deleteOnce(list, (*list)->data->ID);
  }
  free(list);
}

SubscriberData* createSubscriber(int sockId, int id) {
    SubscriberData *sub = (SubscriberData*) malloc(sizeof(SubscriberData));
    sub->sockId = sockId;
    sub->ID = id;
    sub->mustBeForwarded = 0;
    sub->capacity_topics = INITIAL_CAPACITY;
    sub->num_topics = 0;
    sub->topics = (Topic*) malloc(INITIAL_CAPACITY * sizeof(Topic));
    return sub;
}

int containsID(ClientList *list, int id) {
  while (!isEmpty(list)) {
    if (list->data->ID == id) {
      return 1;
    }
    list = list->next;
  }
  return 0;
}

/**
 * Returns NULL if not found.
 */
SubscriberData* findID(ClientList *list, int id) {
  while (!isEmpty(list)) {
    if (list->data->ID == id) {
      return list->data;
    }
    list = list->next;
  }
  return NULL;
}

/**
 * Returns NULL if not found.
 */
SubscriberData* findSocket(ClientList *list, int sockId) {
  while (!isEmpty(list)) {
    if (list->data->sockId == sockId) {
      return list->data;
    }
    list = list->next;
  }
  return NULL;
}

/**
 * Returns 0 if not connected, 1 otherwise.
 */
int connected(ClientList *list, int id) {
    if (list == NULL) return 0;
    SubscriberData *tmp = findID(list, id);
    if (tmp->sockId == -1) return 0;
    return 1;
}

/**
 * If subscribed to topic, just update field sf.
 * Returns -1 if not subscribed to topic, index of found topic otherwise.
 */
int isSubscribed(SubscriberData *sub, char *topic, __uint8_t sf) {
    if (sub == NULL || topic == NULL) return -1;
    for (int i = 0; i < sub->num_topics; i++) {
        if (strcmp(sub->topics[i].name, topic) == 0) {
            sub->topics[i].sf = sf;
            return i;
        }
    }
    return -1;
}

void subscribe(ClientList **list, int sockId, char *topic, __uint8_t sf) {
    if (list == NULL || *list == NULL || topic == NULL) return;

    SubscriberData *sub = findSocket(*list, sockId);
    
    if (isSubscribed(sub, topic, sf) == -1) {
        if (sub->num_topics >= sub->capacity_topics) {
            sub->capacity_topics += INITIAL_CAPACITY;
            sub->topics = (struct Topic*) realloc(sub->topics,
                                    sub->capacity_topics * sizeof(Topic));
        }
        sub->topics[sub->num_topics].name = strdup(topic);
        if (sf == 0 || sf == 1) {
            sub->topics[sub->num_topics].sf = sf;
        } else {
            // Default case.
            sub->topics[sub->num_topics].sf = 0;
        }
        sub->num_topics++;
    }
}

void unsubscribe(ClientList **list, int sockId, char *topic) {
    if (list == NULL || *list == NULL || topic == NULL) return;

    SubscriberData *sub = findSocket(*list, sockId);

    int idx;
    if ((idx = isSubscribed(sub, topic, 0)) >= 0) {
        free(sub->topics[idx].name);
        sub->topics[idx].name = NULL;
        for (int i = idx; i < sub->num_topics - 1; i++) {
            sub->topics[i].name = sub->topics[i + 1].name;
            sub->topics[i].sf = sub->topics[i + 1].sf;
        }
        sub->num_topics--;
    }
}

/**
 * Returns 0 if he must not be forwarded any message, 1 otherwise.
 */
int mustBeForwarded(ClientList *client) {
    return client->data->mustBeForwarded;
}

void printSubscribeList(SubscriberData *sub) {
    if (sub == NULL) return;

    for (int i = 0; i < sub->num_topics; i++) {
        printf("topic[%d].name = %s  &&  ", i, sub->topics[i].name);
        printf("topic[%d].sf = %d\n", i, sub->topics[i].sf);
    }
    printf("\n");
}

#endif
