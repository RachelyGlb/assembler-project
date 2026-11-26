/* types.h - all shared definitions */
#ifndef TYPES_H
#define TYPES_H

/*
 * Legacy header: it serves only the old passes, the old memory builder and
 * the old output writer, and disappears with them. The rebuilt modules use
 * common.h. The standard headers below are included here because the legacy
 * sources rely on getting them through this header.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "common.h"

/*
 * Legacy buffer sizes (not the limits of the language, which are
 * MAX_SOURCE_LINE_LENGTH and MAX_NAME_LENGTH in common.h).
 */
#define MAX_LINE_LENGTH 82
#define MAX_LABEL_LENGTH 31

/* additional constants (MEMORY_SIZE now comes from common.h) */
#define BASE_ADDRESS 100

/* ARE bits */
#define ARE_EXTERNAL 1
#define ARE_RELOCATABLE 2

/* addressing types */
#define IMMEDIATE_ADDR 0
#define DIRECT_ADDR 1
#define MATRIX_ADDR 2
#define REGISTER_ADDR 3

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
