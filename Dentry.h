// DEntry Structure
typedef struct {
    unsigned long long key_hash; 
    char key[256]; 
    int val;
    int klen;
    int vlen;
    int slabs_needed;
} DEntry;
