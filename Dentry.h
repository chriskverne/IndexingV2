// DEntry Structure
typedef struct {
    unsigned long long key_hash; 
    int type;
    int klen;
    int vlen;
    char key[256];  // Array of bytes to store the key
    int val;
} DEntry;
