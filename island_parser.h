#ifndef CODE_ISLAND_PARSER_H
#define CODE_ISLAND_PARSER_H

#define MAX_NAME 50
#define MAX_PATH 256
#define IP_LENGTH 16
#define MAX_ROUTES 50

//Todo: maybe need a product.h file
typedef struct {
    char name[MAX_NAME];
    int quantity;
    int price;
} Product;

typedef struct {
    char name[MAX_NAME];
    char ip_address[IP_LENGTH];
    int port;
} Route;

typedef struct {
    char name[MAX_NAME];
    char path[MAX_PATH];
    char ip_address[IP_LENGTH];
    int port;
    int max_capacity;
    Route routes[MAX_ROUTES];
    int num_routes;
} Island;

Island parseIsland(char *islands_path);
Product *parseStock(char *stock_path, int *num_products);

#endif //CODE_ISLAND_PARSER_H