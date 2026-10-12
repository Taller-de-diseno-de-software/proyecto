// Instrucciones para el codigo intermedio
typedef enum {
    ADD,
    SUB,
    MULT,
    DIV,
    MOD, 
    NEG
} InstrucAritmeticas;

typedef enum {
    AND,
    OR,
    NOT
} InstrucLogicas;


typedef enum {
    LT,
    GT,
    EQ
} InstrucRelacionales;

typedef enum {
    ASSIGN
} InstrucCopia;

typedef enum {
    LABEL,
    JUMP,
    JMPF
} InstrucCopia;

typedef enum {
    ENTER,
    PARAM,
    CALL,
    RET,
    LEAVE
} InstrucFunciones;