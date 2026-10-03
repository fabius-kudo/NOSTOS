#ifndef CODE_ISLAND_PARSER_H
#define CODE_ISLAND_PARSER_H

#define MAX_NAME 100

//Todo: maybe need a product.h file
typedef struct {
    char name[MAX_NAME];
    int quantity;
    int price;
} Product;

typedef struct {
    char *name;
    char *ip_address;
    int port;
} Route;

typedef struct {
    char *name;
    char *path;
    char *ip_address;
    int port;
    int max_capacity;
    Route *routes;
    int num_routes;
} Island;

Island parseIsland(char *islands_path);
void freeIsland(Island island);
Product *parseStock(char *stock_path, int *num_products);
int filterRoutes(Island *island) ;

void printIsland(const Island *island);
void printProducts(const Product *products, int num_products);

#endif //CODE_ISLAND_PARSER_H