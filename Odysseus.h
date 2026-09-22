#ifndef CODE_ODYSSEUS_H
#define CODE_ODYSSEUS_H

#define MAX_NAME 50
#define MAX_PATH 256
#define MAX_FOOD 20
#define IP_LENGTH 16

typedef struct {
    char product[MAX_NAME];
    float quantity;
} Food;

typedef struct {
    char name[MAX_NAME];
    char filePath[MAX_PATH];
    char ithacaIP[IP_LENGTH];
    int ithacaPort;
    char islandName[MAX_NAME];
    char islandIP[IP_LENGTH];
    int islandPort;
    int initialMoney; //maybe should be float
    int initialFoodSupply;
    Food foodSupplies[MAX_FOOD];
} Odysseus;

#endif //CODE_ODYSSEUS_H
