#ifndef CODE_ISLAND_H
#define CODE_ISLAND_H

#define MAX_NAME 50
#define MAX_PATH 256
#define IP_LENGTH 16
#define MAX_ROUTES 50

typedef struct {
    char name[MAX_NAME];
    char ipAddress[IP_LENGTH];
    int port;
} Route;

typedef struct {
    char name[MAX_NAME];
    char path[MAX_PATH];
    char ipAddress[IP_LENGTH];
    int port;
    int maxCapacity;
    Route routes[MAX_ROUTES];
} Island;

#endif //CODE_ISLAND_H
