#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define TABLE_SIZE 300

typedef struct HashNode
{
    char *key;   // Mnemonic
    char value1; // Binary code string
    int value2;  // Instruction length in bytes
    int value3;  // Format type (1, 2, 3, or 4)
    struct HashNode *next;
} HashNode;

// needed data structures
FILE *instructionsFile;
FILE *registersFile;
int L = 0;
int LCCTR = 0;
FILE *srcCode;
FILE *copyFile;
FILE *objFile;

unsigned int hash(char *key)
{
    unsigned int hash = 0;
    while (*key)
        hash = (hash * 31) + *key++;
    return hash % TABLE_SIZE;
}

void insert(HashNode *table[], const char *key, int value, int format)
{
    unsigned int index = hash(key);
    HashNode *node = table[index];

    while (node != NULL)
    {
        if (strcmp(node->key, key) == 0)
        {
            node->value1 = value;
            node->value3 = format;
            return;
        }
        node = node->next;
    }

    HashNode *newNode = malloc(sizeof(HashNode));
    newNode->key = strdup(key);
    newNode->value1 = value;
    newNode->value2 = (format == 1) ? 1 : (format == 2) ? 2
                                      : (format == 3)   ? 3
                                                        : 4; // Set instruction length based on format
    newNode->value3 = format;
    newNode->next = table[index];
    table[index] = newNode;
}

HashNode *get(HashNode *table[], const char *key)
{
    unsigned int index = hash(key);
    HashNode *node = table[index];
    while (node != NULL)
    {
        if (strcmp(node->key, key) == 0)
            return node;
        node = node->next;
    }
    return NULL;
}

void free_table(HashNode *table[])
{
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        HashNode *node = table[i];
        while (node != NULL)
        {
            HashNode *temp = node;
            node = node->next;
            free(temp->key);
            free(temp);
        }
        table[i] = NULL;
    }
}

void write_tables_to_file(HashNode *optab[], HashNode *regs[], const char *filename)
{
    FILE *outFile = fopen(filename, "w");
    if (outFile == NULL)
    {
        printf("Error creating output file\n");
        return;
    }

    // Write header
    fprintf(outFile, "=== SIC/XE ASSEMBLER TABLES ===\n\n");

    // Write OPTAB
    fprintf(outFile, "OPCODE TABLE (OPTAB):\n");
    fprintf(outFile, "%-10s %-8s %-8s %-8s\n", "Mnemonic", "Opcode", "Length", "Format");
    fprintf(outFile, "----------------------------------------\n");

    for (int i = 0; i < TABLE_SIZE; i++)
    {
        HashNode *node = optab[i];
        while (node != NULL)
        {
            fprintf(outFile, "%-10s 0x%-6X %-8d %-8d\n",
                    node->key,
                    node->value1,
                    node->value2,
                    node->value3);
            node = node->next;
        }
    }

    // Write REGS
    fprintf(outFile, "\nREGISTER TABLE (REGS):\n");
    fprintf(outFile, "%-10s %-8s %-8s %-8s\n", "Register", "Number", "Length", "Format");
    fprintf(outFile, "----------------------------------------\n");

    for (int i = 0; i < TABLE_SIZE; i++)
    {
        HashNode *node = regs[i];
        while (node != NULL)
        {
            fprintf(outFile, "%-10s %-8d %-8d %-8d\n",
                    node->key,
                    node->value1,
                    node->value2,
                    node->value3);
            node = node->next;
        }
    }

    fclose(outFile);
    printf("Tables have been written to %s\n", filename);
}

void pass_one(HashNode *optab[], HashNode *regs[], HashNode *symtab[])
{
    char buffer[50];
    char buffer2[50];
    srcCode = fopen("input_code.txt", "r");
    copyFile = fopen("copy_file.txt", "w");
    if (srcCode == NULL)
    {
        printf("==== NO SOURCE FILE FOUND ====\n");
        exit(1);
    }
    while (fgets(buffer, sizeof(buffer), srcCode))
    {
        strcpy(buffer2, buffer);
        char *token = strtok(buffer, " \n");
        if (token != NULL && strcmp(token, "PROG") == 0)
        {
            insert(symtab, "PROG", LCCTR, 0);
            token = strtok(NULL, " \n");
            if (token != NULL && strcmp(token, "START") == 0)
            {
                printf("Found START directive\n");
                token = strtok(NULL, " \n");
                L = atoi(token);
                LCCTR += L;
                fprintf(copyFile, "%s", buffer2);
                continue;
            }
        }
        if (token != NULL && strcmp(token, "END") == 0)
        {
            printf("Found END directive\n");
            fprintf(copyFile, "%s", buffer2);
            continue;
        }

        char *mnemonic = token;
        char *operand = strtok(NULL, " \n");
        if (operand != NULL)
        {
            HashNode *operation = get(optab, mnemonic);
            L = operation->value2;

            if (operation != NULL)
            {
                LCCTR += L;
            }
            else if (operation == NULL)
            {
                // First check if it's a directive
                token = strtok(NULL, " \n");
                if (token != NULL)
                {
                    if (strcmp(token, "BYTE") == 0)
                    {
                        token = strtok(NULL, " \n");
                        L = strlen(token);
                        insert(symtab, mnemonic, LCCTR, L);
                        LCCTR += L;
                    }
                    else if (strcmp(token, "WORD") == 0)
                    {
                        L = 3;
                        insert(symtab, mnemonic, LCCTR, L);
                        LCCTR += L;
                    }
                    else if (strcmp(token, "RESB") == 0)
                    {
                        token = strtok(NULL, " \n");
                        L = atoi(token);
                        insert(symtab, mnemonic, LCCTR, L);
                        LCCTR += L;
                    }
                    else if (strcmp(token, "RESW") == 0)
                    {
                        token = strtok(NULL, " \n");
                        L = 3 * atoi(token);
                        insert(symtab, mnemonic, LCCTR, L);
                        LCCTR += L;
                    }
                    else if (strcmp(token, "EQU") == 0)
                    {
                        token = strtok(NULL, " \n");
                        int value = atoi(token);
                        insert(symtab, mnemonic, value, 0);
                    }
                    else if (strcmp(token, "ORG") == 0)
                    {
                        token = strtok(NULL, " \n");
                        LCCTR = atoi(token);
                    }
                    else
                    {
                        // Check if it's an operation with format 4 (+)
                        if (strstr(mnemonic, "+") != NULL)
                        {
                            operation = get(optab, mnemonic + 1); // Skip the + character
                            if (operation != NULL)
                            {
                                L = 4;
                                insert(symtab, mnemonic, LCCTR, L);
                                LCCTR += L;
                            }
                        }
                        // Check if it's a regular operation
                        else
                        {
                            operation = get(optab, mnemonic);
                            if (operation != NULL)
                            {
                                L = 3;
                                insert(symtab, mnemonic, LCCTR, L);
                                LCCTR += L;
                            }
                        }
                    }
                }
            }

            fprintf(copyFile, "%s", buffer2);
            continue;
        }
        fprintf(copyFile, "%s", buffer2);
    }
    fclose(srcCode);
    fclose(copyFile);
}
int main()
{
    HashNode *OPTAB[TABLE_SIZE] = {NULL};
    HashNode *SYMTAB[TABLE_SIZE] = {NULL};
    HashNode *REGS[TABLE_SIZE] = {NULL};
    char buffer[50];
    char mnemonic[10];
    int format;
    int opcode;

    instructionsFile = fopen("instructions.txt", "r");
    if (instructionsFile == NULL)
    {
        printf("FILE WASN'T FOUDN");
        exit(1);
    }
    while (fgets(buffer, sizeof(buffer), instructionsFile))
    {
        if (sscanf(buffer, "%s %d %x", mnemonic, &format, &opcode) == 3)
        {
            insert(OPTAB, mnemonic, opcode, format);
        }
        else
        {
            printf("Invalid line: %s", buffer);
        }
    }
    fclose(instructionsFile);
    HashNode *val = get(OPTAB, "ADD");
    if (val)
    {
        printf("Opcode for ADD: %s   %X   %d   %d\n", val->key, val->value1, val->value2, val->value3);
    }

    registersFile = fopen("registers.txt", "r");
    if (registersFile == NULL)
    {
        printf("REGISTERS FILE WASN'T FOUND\n");
        exit(1);
    }

    while (fgets(buffer, sizeof(buffer), registersFile))
    {
        int reg_num;
        if (sscanf(buffer, "%s %d", mnemonic, &reg_num) == 2)
        {
            insert(REGS, mnemonic, reg_num, 2); // Using format 2 for registers
        }
        else
        {
            printf("Invalid line in registers file: %s", buffer);
        }
    }
    fclose(registersFile);

    // Test register lookup
    HashNode *reg = get(REGS, "A");
    if (reg)
    {
        printf("Register A: %s   %d   %d   %d\n", reg->key, reg->value1, reg->value2, reg->value3);
    }

    // Write tables to file before freeing them
    write_tables_to_file(OPTAB, REGS, "assembler_tables.txt");
    pass_one(OPTAB, REGS, SYMTAB);

    // Free tables after writing
    free_table(OPTAB);
    free_table(SYMTAB);
    free_table(REGS);

    return 0;
}
