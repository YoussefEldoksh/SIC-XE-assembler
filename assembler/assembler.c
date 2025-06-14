#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define TABLE_SIZE 300

typedef struct HashNode
{
    char *key;    // Mnemonic
    char *value1; // HEX code string
    int value2;   // Instruction length in bytes
    int value3;   // Format type (1, 2, 3, or 4)
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

void insert(HashNode *table[], const char *key  , char *value, int format)
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
            fprintf(outFile, "%-10s %-8s %-8d %-8d\n",
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
            fprintf(outFile, "%-10s %-8s %-8d %-8d\n",
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
        printf("Processing: %s", buffer2); // Print each line as it's processed

        char *token = strtok(buffer, " \n");
        if (token != NULL)
        {
            // Store the symbol and its current location
            char LCCTRstr[20]; // Allocate proper buffer
            sprintf(LCCTRstr, "%d", LCCTR);
            insert(symtab, token, LCCTRstr, 0);
            printf("Symbol: %s\n", token);
            printf("LCCTR: %d\n", LCCTR);
            printf("Format: %d\n", get(symtab, token)->value3);
            printf("--------------------------------\n");

            char *mnemonic = token;
            char *operand = strtok(NULL, " \n");
            if (operand != NULL)
            {
                if (strcmp(mnemonic, "START") == 0)
                {
                    printf("Found START directive\n");
                    token = strtok(NULL, " \n");
                    if (token != NULL)
                    {
                        LCCTR = atoi(token); // Set starting address
                        printf("Starting address set to: %d\n", LCCTR);
                    }
                }
                else if (strcmp(mnemonic, "END") == 0)
                {
                    printf("Found END directive\n");
                }
                else
                {
                    // Check if it's an operation
                    HashNode *operation = get(optab, mnemonic);
                    if (operation != NULL)
                    {
                        // Update LCCTR based on instruction format
                        if (strstr(mnemonic, "+") != NULL)
                        {
                            // Format 4 instruction
                            LCCTR += 4;
                            printf("Format 4 instruction, LCCTR += 4\n");
                        }
                        else
                        {
                            // Format 3 instruction
                            LCCTR += 3;
                            printf("Format 3 instruction, LCCTR += 3\n");
                        }
                    }
                    else
                    {
                        // Check for directives
                        if (strcmp(operand, "BYTE") == 0)
                        {
                            token = strtok(NULL, " \n");
                            if (token != NULL)
                            {
                                L = strlen(token) - 2; // Subtract 2 for quotes
                                LCCTR += L;
                                printf("BYTE directive, LCCTR += %d\n", L);
                            }
                        }
                        else if (strcmp(operand, "WORD") == 0)
                        {
                            LCCTR += 3;
                            printf("WORD directive, LCCTR += 3\n");
                        }
                        else if (strcmp(operand, "RESB") == 0)
                        {
                            token = strtok(NULL, " \n");
                            if (token != NULL)
                            {
                                L = atoi(token);
                                LCCTR += L;
                                printf("RESB directive, LCCTR += %d\n", L);
                            }
                        }
                        else if (strcmp(operand, "RESW") == 0)
                        {
                            token = strtok(NULL, " \n");
                            if (token != NULL)
                            {
                                L = 3 * atoi(token);
                                LCCTR += L;
                                printf("RESW directive, LCCTR += %d\n", L);
                            }
                        }
                    }
                }
            }
            fprintf(copyFile, "%s", buffer2);
            continue;
        }
        fflush(stdout);
        fprintf(copyFile, "%s", buffer2);
    }
    fclose(srcCode);
    fclose(copyFile);
}

void pass_two(HashNode *optab[], HashNode *regs[], HashNode *symtab[])
{
    char buffer[256];        // Allocate proper buffer
    char buffer2[256];       // Allocate proper buffer
    char *nixpbe = "000000"; // 6 zeros initial value
    copyFile = fopen("copy_file.txt", "r");
    objFile = fopen("object_code.txt", "w");
    if (copyFile == NULL)
    {
        printf("==== NO COPY FILE FOUND ====\n");
        exit(1);
    }

    // Get program name and starting address from first line
    if (fgets(buffer, sizeof(buffer), copyFile) == NULL)
    {
        printf("Error reading first line\n");
        fclose(copyFile);
        fclose(objFile);
        return;
    }
    strcpy(buffer2, buffer);
    char *progName = strtok(buffer, " \n");
    char *startDirective = strtok(NULL, " \n");
    int startAddr = 0;
    if (startDirective != NULL && strcmp(startDirective, "START") == 0)
    {
        char *addrStr = strtok(NULL, " \n");
        if (addrStr != NULL)
        {
            startAddr = atoi(addrStr);
            LCCTR = startAddr;
        }
    }

    // Get program length from symbol table
    HashNode *progSymbol = get(symtab, progName);
    int progLength = 0;
    if (progSymbol != NULL)
    {
        // Find the last symbol's address + length to get program length
        for (int i = 0; i < TABLE_SIZE; i++)
        {
            HashNode *node = symtab[i];
            while (node != NULL)
            {
                if (node->value2 > 0) // If it's a symbol with length
                {
                    char *num = node->value1;
                    int endAddr = atoi(num) + node->value2;
                    if (endAddr > progLength)
                    {
                        progLength = endAddr;
                    }
                }
                node = node->next;
            }
        }
        progLength -= startAddr; // Subtract starting address to get length
    }

    // Write header record
    fprintf(objFile, "H^%-6s^%06X^%06X\n", progName, startAddr, progLength);
    printf("Header Record: H%-6s^%06X^%06X\n", progName, startAddr, progLength);

    // Reset file pointer to beginning for processing instructions
    rewind(copyFile);

    while (fgets(buffer, sizeof(buffer), copyFile))
    {
        strcpy(buffer2, buffer);
        char *token = strtok(buffer, " \n");
        if (token != NULL)
        {
            HashNode *operation = get(optab, token);
            if (operation != NULL)
            {
                char *opcode = operation->value1;
                int format = operation->value3; // Format is in value3
                int length = operation->value2; // Length is in value2
                char *nixpbe = "000000";
                char *txt_record = malloc(32 + 1);
                strcpy(txt_record, "");

                if (format == 1)
                {
                    strcat(txt_record, opcode);
                    fprintf(objFile, "T%06X%02X%s\n", LCCTR, length, txt_record);
                }
                else if (format == 2)
                {
                    token = strtok(NULL, " \n");
                    if (token != NULL)
                    {
                        HashNode *reg_node = get(regs, token);
                        if (reg_node != NULL)
                        {
                            strcat(txt_record, opcode);
                            strcat(txt_record, reg_node->value1);
                            fprintf(objFile, "T%06X%02X%s\n", LCCTR, length, txt_record);
                        }
                        else
                        {
                            printf("Invalid register: %s\n", token);
                        }
                    }
                }
                else if (format == 3)
                {
                    token = strtok(NULL, " \n");
                    if (token != NULL)
                    {
                        if (token[0] == '#')
                        {
                            token++;
                            nixpbe = "010000";
                        }
                        else if (token[0] == '@')
                        {
                            token++;
                            nixpbe = "100000";
                        }
                        else if (token[strlen(token) - 1] == 'X')
                        {
                            nixpbe = "111010";
                        }
                        strcat(txt_record, opcode);
                        strcat(txt_record, nixpbe);
                        fprintf(objFile, "T%06X%02X%s\n", LCCTR, length, txt_record);
                    }
                }
                else if (format == 4)
                {
                    token = strtok(NULL, " \n");
                    if (token != NULL)
                    {
                        if (token[0] == '#')
                        {
                            token++;
                            nixpbe = "010001";
                        }
                        else if (token[0] == '@')
                        {
                            token++;
                            nixpbe = "100001";
                        }
                        else if (token[strlen(token) - 1] == 'X')
                        {
                            nixpbe = "111011";
                        }
                        strcat(txt_record, opcode);
                        strcat(txt_record, nixpbe);
                        fprintf(objFile, "T%06X%02X%s\n", LCCTR, length, txt_record);
                    }
                }
                free(txt_record);
            }
        }
    }
    fclose(copyFile);
    fclose(objFile);
}

int main()
{
    HashNode *OPTAB[TABLE_SIZE] = {NULL};
    HashNode *SYMTAB[TABLE_SIZE] = {NULL};
    HashNode *REGS[TABLE_SIZE] = {NULL};
    char buffer[50];
    char mnemonic[10];
    int format;
    char opcode[10]; // Changed back to 10 to accommodate hex opcodes

    instructionsFile = fopen("instructions.txt", "r");
    if (instructionsFile == NULL)
    {
        printf("FILE WASN'T FOUND");
        exit(1);
    }
    while (fgets(buffer, sizeof(buffer), instructionsFile))
    {
        if (sscanf(buffer, "%s %d %s", mnemonic, &format, opcode) == 3)
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
        printf("Opcode for ADD: %s   %s   %d   %d\n", val->key, val->value1, val->value2, val->value3);
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
            char register_num[10]; // Allocate proper buffer
            sprintf(register_num, "%d", reg_num);
            insert(REGS, mnemonic, register_num, 0);
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
        printf("Register A: %s   %s   %d   %d\n", reg->key, reg->value1, reg->value2, reg->value3);
    }

    // Write tables to file before freeing them
    write_tables_to_file(OPTAB, REGS, "assembler_tables.txt");
    pass_one(OPTAB, REGS, SYMTAB);

    // Call pass two to generate object code
    printf("\nGenerating object code...\n");
    pass_two(OPTAB, REGS, SYMTAB);

    printf("Object code generation complete. Check object_code.txt\n");

    // Free tables after writing
    free_table(OPTAB);
    free_table(SYMTAB);
    free_table(REGS);

    return 0;
}
