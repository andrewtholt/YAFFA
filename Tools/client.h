
typedef enum { NONE, BOOLEAN, ERROR, STATUS, INTEGER, STRING, REAL, POINTER } Type;

typedef struct {
  Type type;
  union {
    int integer;
    char *string;
    float real;
    void *pointer;
  } x;
} Value;


// char *redisCommand(int, char *);
// Value redisReply(int);
int redisPing(int );
int enQueue(int, char *, char *, int);
char *deQueue(int ,char *);
void clearValue(Value);


