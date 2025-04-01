#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <pthread.h>

#define DB_FILE "minidb.data"
#define PORT 8080
#define NAME_SIZE 50
#define BUFFER_SIZE 1024
#define ORDER 4  // B+ Tree Order

#pragma comment(lib, "ws2_32.lib")  // Link Winsock library

typedef struct {
    int id;
    char name[NAME_SIZE];
    int age;
} Record;

typedef struct BTreeNode {
    int keys[ORDER - 1];
    struct BTreeNode* children[ORDER];
    int isLeaf;
    int numKeys;
} BTreeNode;

BTreeNode* root = NULL;

BTreeNode* createNode(int isLeaf) {
    BTreeNode* newNode = (BTreeNode*)malloc(sizeof(BTreeNode));
    newNode->isLeaf = isLeaf;
    newNode->numKeys = 0;
    for (int i = 0; i < ORDER; i++) newNode->children[i] = NULL;
    return newNode;
}

void insert_record(int id, const char* name, int age) {
    FILE* file = fopen(DB_FILE, "ab");
    if (!file) {
        printf("Error opening database file!\n");
        return;
    }
    Record rec = {id, "", age};
    strncpy(rec.name, name, NAME_SIZE - 1);
    fwrite(&rec, sizeof(Record), 1, file);
    fclose(file);
}

void insert_into_btree(int id) {
    if (!root) {
        root = createNode(1);
        root->keys[0] = id;
        root->numKeys = 1;
    } else {
        BTreeNode* current = root;
        while (!current->isLeaf)
            current = current->children[current->numKeys];
        current->keys[current->numKeys++] = id;
    }
}

char* select_record(int id) {
    static char result[BUFFER_SIZE];
    FILE* file = fopen(DB_FILE, "rb");
    if (!file) {
        sprintf(result, "Database file not found!\n");
        return result;
    }
    
    Record rec;
    while (fread(&rec, sizeof(Record), 1, file)) {
        if (rec.id == id) {
            sprintf(result, "Record Found: ID=%d, Name=%s, Age=%d\n", rec.id, rec.name, rec.age);
            fclose(file);
            return result;
        }
    }
    sprintf(result, "Record not found!\n");
    fclose(file);
    return result;
}
void* client_handler(void* socket_desc) {
    int sock = *(SOCKET*)socket_desc;
    char buffer[BUFFER_SIZE];
    int id, age;
    char name[NAME_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        
        // **Loop until we receive a full command**
        int bytes_received = 0, total_received = 0;
        while ((bytes_received = recv(sock, buffer + total_received, BUFFER_SIZE - total_received - 1, 0)) > 0) {
            total_received += bytes_received;
            if (buffer[total_received - 1] == '\n')  // Stop reading when newline is received
                break;
        }

        if (bytes_received <= 0) {
            printf("Client disconnected or error occurred.\n");
            break;
        }

        buffer[total_received] = '\0';  // Ensure null-terminated string
        printf("Received command: [%s]\n", buffer);  // Debugging

        if (strncmp(buffer, "INSERT", 6) == 0) {
            if (sscanf(buffer, "INSERT %d %s %d", &id, name, &age) == 3) {
                insert_record(id, name, age);
                insert_into_btree(id);
                send(sock, "Record inserted successfully!\n", 30, 0);
            } else {
                send(sock, "Invalid INSERT format! Use: INSERT <id> <name> <age>\n", 55, 0);
            }
        } else if (strncmp(buffer, "SELECT", 6) == 0) {
            if (sscanf(buffer, "SELECT %d", &id) == 1) {
                send(sock, select_record(id), BUFFER_SIZE, 0);
            } else {
                send(sock, "Invalid SELECT format! Use: SELECT <id>\n", 40, 0);
            }
        } else if (strncmp(buffer, "EXIT", 4) == 0) {
            break;
        } else {
            send(sock, "Invalid command! Available: INSERT, SELECT, EXIT\n", 50, 0);
        }
    }

    closesocket(sock);
    free(socket_desc);
    return 0;
}


void start_server() {
    SOCKET server_fd, new_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        printf("Socket creation failed! Error: %d\n", WSAGetLastError());
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind failed! Error: %d\n", WSAGetLastError());
        closesocket(server_fd);
        return;
    }

    if (listen(server_fd, 5) == SOCKET_ERROR) {
        printf("Listen failed! Error: %d\n", WSAGetLastError());
        closesocket(server_fd);
        return;
    }

    printf("MiniDB Server started on port %d\n", PORT);

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr*)&client_addr, &addr_size);
        if (new_socket == INVALID_SOCKET) {
            printf("Accept failed! Error: %d\n", WSAGetLastError());
            continue;
        }

        pthread_t thread;
        SOCKET* new_sock = malloc(sizeof(SOCKET));
        *new_sock = new_socket;
        pthread_create(&thread, NULL, client_handler, (void*)new_sock);
    }

    closesocket(server_fd);
}

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed to initialize Winsock! Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    start_server();
    WSACleanup();
    return 0;
}