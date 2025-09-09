/* types.h - all shared definitions */
#ifndef TYPES_H
#define TYPES_H

#include "assembler.h"


/* additional constants */
#define MEMORY_SIZE 256
#define BASE_ADDRESS 100
#define WORD_SIZE 10

/* ARE bits */
#define ARE_ABSOLUTE 0
#define ARE_EXTERNAL 1
#define ARE_RELOCATABLE 2

/* addressing types */
#define IMMEDIATE_ADDR 0
#define DIRECT_ADDR 1
#define MATRIX_ADDR 2
#define REGISTER_ADDR 3

/* aliases for backward compatibility */
typedef enum
{
    IMMEDIATE_ADDRESSING = 0,
    DIRECT_ADDRESSING = 1,
    INDIRECT_ADDRESSING = 2, /* identical to MATRIX_ADDR */
    REGISTER_ADDRESSING = 3
} AddressingType;

/* symbol types */
typedef enum
{
    CODE,
    DATA,
    EXTERN_SYM
} SymbolType;

/* machine word - 10 bits */
typedef struct
{
    unsigned int bits : 10;
} MachineWord;

/* simple memory structure */
typedef struct
{
    MachineWord instruction_image[256];
    int data_memory[256];
    MachineWord data_image[256]; /* added for compatibility */
    int instruction_count;
    int data_count;
    int ICF; /* added this */
    int DCF; /* added this */
} MemoryImage;

/* symbol structure */
typedef struct Symbol
{
    char name[MAX_LABEL_LENGTH];
    int address;
    SymbolType type;
    int is_entry;
    struct Symbol *next;
} Symbol;

/* instruction definition */
typedef struct
{
    char name[10];
    int opcode;
    int operands_count;
} InstructionDef;

/* external reference */
typedef struct ExtRef
{
    char symbol_name[MAX_LABEL_LENGTH];
    int address;
    struct ExtRef *next;
} ExtRef;

/* entry point */
typedef struct EntryPoint
{
    char symbol_name[MAX_LABEL_LENGTH];
    int address;
    struct EntryPoint *next;
} EntryPoint;

#endif /* TYPES_H */
