#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>

// ------------------------------------------------------------------------ //
// ------------------------------------------------------------------------ //
// ---------------------HASHTABLE FUNCTIONS/DECLARATIONS------------------- //
// ------------------------------------------------------------------------ //
// ------------------------------------------------------------------------ //

#define TABLE_SIZE 100000

// Function declarations
void dir_search(char *basePath, int min_len);
void scan_file(char *abs_path, int min_len);
void is_dir(char *base_path, int min_len);

void delete_path(char *filename, char *path);

typedef struct entry_t
{
    char *key;
    char *value; // Need to change this to include 1. # of paths containing the word, and 2. Continuous string containing the values separated by
    struct entry_t *next;
} entry_t;

typedef struct
{
    entry_t **entries;
} ht_t;

ht_t *ht;

// Hash function
unsigned int hash(const char *key)
{
    unsigned long int value = 0;
    unsigned int i = 0;
    unsigned int key_len = strlen(key);

    // do several rounds of multiplication
    for (; i < key_len; ++i)
    {
        value = value * 37 + key[i];
    }

    // make sure value is 0 <= value < TABLE_SIZE
    value = value % TABLE_SIZE;

    return value;
}

entry_t *ht_pair(const char *key, const char *value)
{
    // allocate the entry
    entry_t *entry = malloc(sizeof(entry_t) * 1);
    entry->key = malloc(strlen(key) + 1);
    entry->value = malloc(strlen(value) + 1); // THIS NEEDS TO BE CHANGED

    // copy the key and value in place
    strcpy(entry->key, key);
    strcpy(entry->value, value);

    // next starts out null but may be set later on
    entry->next = NULL;

    return entry;
}

// Creates the hashtable and sets all entries to null
ht_t *ht_create(void)
{
    // allocate table
    ht_t *hashtable = malloc(sizeof(ht_t) * 1);

    // allocate table entries
    hashtable->entries = malloc(sizeof(entry_t *) * TABLE_SIZE);

    // set each to null (needed for proper operation)
    int i = 0;
    for (; i < TABLE_SIZE; ++i)
    {
        hashtable->entries[i] = NULL;
    }

    return hashtable;
}

// Sets the key and value in the hashtable
// Firstly it checks if the slot in the hashtable == NULL, if it does then simply insert the ht_pair in
// the entry slot
// Secondly if not, it walks along the linked list until it finds the matching key that is passed in as
// a parameter
// Once this is found, (at the moment) we reallocate the space for the value variable so we can
// include the new absolute path (separated by a space) which can be used to build the trove-file later
// NOTE: I want to change this from a space " " to maybe a null byte and see if I can iterate over the
// elements inside just like we do with the command line *argv[]...
void ht_set(ht_t *hashtable, const char *key, const char *value)
{
    unsigned int slot = hash(key);

    // try to look up an entry set
    entry_t *entry = hashtable->entries[slot];

    // no entry means slot empty, insert immediately
    if (entry == NULL)
    {
        hashtable->entries[slot] = ht_pair(key, value);
        return;
    }
    else if (strstr(entry->value, value))
    {
        return;
    }

    entry_t *prev;

    // walk through each entry until either the end is
    // reached or a matching key is found
    while (entry != NULL)
    {
        // check key
        if (strcmp(entry->key, key) == 0)
        {
            char *temp;
            size_t newlen;

            newlen = strlen(entry->value) + strlen(value) + 2; // Adding + 2 to take into account for the space and the new null byte
            temp = (char *)malloc(newlen);

            strcpy(temp, entry->value);
            strcat(temp, "\\");
            strcat(temp, value);

            entry->value = realloc(entry->value, newlen);
            strcpy(entry->value, temp);
            return;
        }

        // walk to next
        prev = entry;
        entry = prev->next;
    }

    // end of chain reached without a match, add new
    prev->next = ht_pair(key, value);
}

// Retrieves a single value when a key is passed in
char *ht_get(ht_t *hashtable, const char *key)
{
    unsigned int slot = hash(key);

    // try to find a valid slot
    entry_t *entry = hashtable->entries[slot];

    // no slot means no entry
    if (entry == NULL)
    {
        return NULL;
    }

    // walk through each entry in the slot, which could just be a single thing
    while (entry != NULL)
    {
        // return value if found
        if (strcmp(entry->key, key) == 0)
        {
            return entry->value;
        }

        // proceed to next key if available
        entry = entry->next;
    }

    // reaching here means there were >= 1 entries but no key match
    return NULL;
}

// Deletes a key/value pair
void ht_del(ht_t *hashtable, const char *key)
{
    unsigned int bucket = hash(key);

    // try to find a valid bucket
    entry_t *entry = hashtable->entries[bucket];

    // no bucket means no entry
    if (entry == NULL)
    {
        return;
    }

    entry_t *prev;
    int idx = 0;

    // walk through each entry until either the end is reached or a matching key is found
    while (entry != NULL)
    {
        // check key
        if (strcmp(entry->key, key) == 0)
        {
            // first item and no next entry
            if (entry->next == NULL && idx == 0)
            {
                hashtable->entries[bucket] = NULL;
            }

            // first item with a next entry
            if (entry->next != NULL && idx == 0)
            {
                hashtable->entries[bucket] = entry->next;
            }

            // last item
            if (entry->next == NULL && idx != 0)
            {
                prev->next = NULL;
            }

            // middle item
            if (entry->next != NULL && idx != 0)
            {
                prev->next = entry->next;
            }

            // free the deleted entry
            free(entry->key);
            free(entry->value);
            free(entry);

            return;
        }

        // walk to next
        prev = entry;
        entry = prev->next;

        ++idx;
    }
}

// This is simply for debugging purposes - will delete this later
// but allows us to check the slots and what key and values are in them
void ht_dump(ht_t *hashtable)
{
    for (int i = 0; i < TABLE_SIZE; ++i)
    {
        entry_t *entry = hashtable->entries[i];

        if (entry == NULL)
        {
            continue;
        }

        printf("slot[%4d]: ", i);

        for (;;)
        {
            printf("%s=%s ", entry->key, entry->value);

            if (entry->next == NULL)
            {
                break;
            }

            entry = entry->next;
        }

        printf("\n");
    }
}

// A function that takes in a file and iterates through finding each word that is strictly
// ALPHANUMERIC - and then passes the newly created word and its absolute path into ht_set
// so it can then be added to the hashtable
void add_words(FILE *f, int min_len, char *path)
{
    int character;   // integer representation of character we will store in the file
    char word[1000]; // where we store the character each iteration - currently hardcoded at the moment

    while (!feof(f))
    {
        int index = 0;
        while ((character = fgetc(f)) != EOF && isalnum(character))
        {
            word[index] = character;
            index++;
        }
        word[index] = '\0';

        // If the word is greater than or = to the min length then we add it to the hashmap
        if (strlen(word) >= min_len)
        {
            ht_set(ht, word, path); // Adds values to the hashtable
        }
    }
}

// Recursively searches through a directory and scans the files
// Each call to dir_search appends "/dp->d_name"
// From here we check if it is a directory again or a file and each recursive loop
// will keep going through this process and find all the files in the directory that
// is passed into the commandline
void dir_search(char *basePath, int min_len)
{
    char path[PATH_MAX];
    struct dirent *dp;
    DIR *dir = opendir(basePath);

    if (!dir)
    {
        return;
    }

    while ((dp = readdir(dir)) != NULL)
    {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
        {
            strcpy(path, basePath);   // copy the basepath to path
            strcat(path, "/");        // concatenate "/"
            strcat(path, dp->d_name); // concatenate directory/file name

            struct stat st;
            if (stat(path, &st) == 0)
            {
                if (S_ISDIR(st.st_mode))
                {
                    dir_search(path, min_len);
                }
                else
                {
                    scan_file(path, min_len);
                }
            }
        }
    }
    closedir(dir);
}

// This function scans through the file and adds the words and absolute path to a hashtable
void scan_file(char *abs_path, int min_len)
{
    FILE *f = fopen(abs_path, "rb");
    add_words(f, min_len, abs_path); // Searches through the file for all words >= min_len
    fclose(f);
}

// This function is used when we first check to see if a command line argument is a file or a directory
// If it is a FILE, then we simply scan the file and add the words to the hashtable
// If it is a DIRECTORY, we call dir_search and pass in the absolute path for the commandline argument
void is_dir(char *base_path, int min_len)
{
    char buffer[PATH_MAX];
    char *res = realpath(base_path, buffer); // Retrieve the ABSOLUTE PATH and copy to 'buffer'

    if (res)
    {
        // stat struct allows us to check if the 'buffer' absolute path is a directory or a
        // file by using the st_mode variable
        struct stat st;
        stat(buffer, &st);
        DIR *dir = opendir(buffer);

        if (S_ISDIR(st.st_mode))
        {
            // If command line argument is a directory - we will pass the buffer into dir_search
            // and recursively traverse through the folder to find all the files and add
            // their words to the hashtable
            closedir(dir); // probably not efficient but allows the dir_search function to work properly
            dir_search(buffer, min_len);
        }
        else
        {
            // If command line argument is a file - we will pass the buffer into scan_file
            // and this will add the words to the hashtable
            scan_file(buffer, min_len);
        }
    }
}

// ------------------------------------------------------------------------ //
// ------------------------------------------------------------------------ //
// --------------------------TROVE UTILITIES------------------------------- //
// ------------------------------------------------------------------------ //
// ------------------------------------------------------------------------ //

// Build Trove file
void build_trove(ht_t *hashtable, char *basePath)
{

    char path[PATH_MAX];
    char *res = realpath(basePath, path);

    printf("%s\n", path);

    // open directory
    struct stat st;
    stat(path, &st);

    FILE *f = fopen(path, "wb");
    if (f == NULL)
    {
        printf("Error opening file!\n");
        exit(1);
    }

    for (int i = 0; i < TABLE_SIZE; ++i)
    {
        entry_t *entry = hashtable->entries[i];

        if (entry == NULL)
        {
            continue;
        }

        // Change to while(true)
        for (;;)
        {
            // printf("%s=%s ", entry->key, entry->value);

            fprintf(f, "%s\\%s\n", entry->key, entry->value);
            // fprintf(f, "%s\n", );

            if (entry->next == NULL)
            {
                break;
            }

            entry = entry->next;
        }
    }
    fclose(f);
}

// Remove absolute path from the line
// Append to new file
// Delete old file
// Copy name to new file
// NOTE - THIS ASSUMPTION IS THAT THE FILE WILL BE IN THE SAME DIRECTORY AS ./trove EXECUTABLE
// ^^ this makes sense because we are passing into the command line one single file not an absolute path to the file
// via the -f flag
void delete_path(char *filename, char *path)
{
    FILE *filein = fopen(filename, "r"); // file we are searching line by line
    FILE *fileout = fopen("temp", "w");  // file we are writing to
    char abs_path[PATH_MAX];
    char *res = realpath(path, abs_path); // realpath fills the abs_path buffer

    // Concatenated path with "\"
    char *concat_path = (char *)malloc(strlen(abs_path) + 2);
    strcat(concat_path, "\\");
    strcat(concat_path, abs_path);

    if (filein == NULL || fileout == NULL)
    {
        printf("Error opening file!\n");
        exit(1);
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    // Debug seeing if it drops any words from file
    int countin = 0;
    int countout = 0;

    while ((read = getline(&line, &len, filein)) != -1)
    {

        char *token;
        token = strtok(line, "\\");
        char new_line[10000]; // Dynamic won't work for some reason...
        strcpy(new_line, token);

        // Get the absolute path value and then enter while loop
        token = strtok(NULL, "\\");
        countin++;
        while (token != NULL)
        {
            if (!(strstr(token, abs_path)))
            {
                strcat(new_line, "\\");
                strcat(new_line, token);
            }
            token = strtok(NULL, "\\");
        }

        if (strstr(new_line, "\\"))
        {
            countout++;
            // Add string to file out
            fprintf(fileout, "%s", new_line);
        }
    }

    printf("Number of lines file in: %-30d Number of lines file out: %d\n", countin, countout);

    // Freeing memory and closing files
    free(concat_path);
    // free(new_line);
    if (line)
        free(line);
    fclose(filein);
    fclose(fileout);

    // Delete old file with absolute path
    remove(filename); // do we need a check here?

    // Change temp file to existing filename
    rename("temp", filename);
}

// Search trove file for word
// Prints out the absolute paths corresponding to the word
void search_word(char *filename, char *word)
{
    FILE *f = fopen(filename, "rb");
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    if (f == NULL)
    {
        printf("Error opening file!\n");
        exit(1);
    }

    while ((read = getline(&line, &len, f)) != -1)
    {
        // printf("Retrieved line of length %zu:\n", read);
        // printf("%s", line);
        line[strcspn(line, "\n")] = 0;
        const char s[2] = "\\";
        char *token;

        // AT THE MOMENT THIS RETURNS THE WORD AS WELL AS THE ABSOLUTE PATHS

        token = strtok(line, s);
        if (strcmp(token, word) == 0)
        {
            while (token != NULL)
            {
                printf("%s\n", token);
                token = strtok(NULL, s);
            }
        }
    }

    fclose(f);
    if (line)
        free(line);
    // exit(EXIT_SUCCESS);
}

// ------------------------------------------------------------------------ //
// ------------------------------------------------------------------------ //
// --------------------------------MAIN------------------------------------ //
// ------------------------------------------------------------------------ //
// ------------------------------------------------------------------------ //

int main(int argc, char *argv[])
{
    // Used for getopt() function
    int opt;

    // The default minimum length when -l is not passed into command line
    int min_len = 4;

    // Dynamically allocating space for the filename incase this changes via a commandline input
    char *filename = (char *)malloc(strlen("trove") + 1);

    // Create the hashtable
    ht = ht_create();

    // Check to see if no arguments have been passed in and returns a statement
    // prompting the user how to use ./trove
    if (argc < 2)
    {
        printf("\nUsage: ./trove [-f filename] [-b | -r | -u] [-l length] filelist\nor     ./trove [-f filename] word\n\nwhere options are:\n-b		build a new trove-file\n-f filename	provide the name of the trove-file to be built or searched\n-l length	specify the minimum-length of words added to the trove-file\n-r		remove information from the trove-file\n-u		update information in the trove-file\n\n");
        exit(EXIT_FAILURE);
    }

    while ((opt = getopt(argc, argv, "f:l:r:u:b:")) != -1)
    {
        switch (opt)
        {
        case 'f':
            // Reallocate the memory to = the length of the filename passed in + 1 for the null byte
            filename = (char *)realloc(filename, strlen(optarg) + 1);
            strcpy(filename, optarg);
            break;
        case 'l':
            // Get the length from the arguments
            // MAY NEED TO ADD A CHECK IN TO MAKE SURE MIN LENGTH IS >= 1
            min_len = atoi(optarg);
            if (min_len < 1)
            {
                printf("Must provide a positive integer for -l argument!");
                return EXIT_FAILURE;
            }
            break;
        case 'r':
            // Checking if first argument is a directory or not
            // inside of is_dir, if it is, then we traverse through the dictionary and delete
            // printf("%s\n", optarg);
            delete_path(filename, optarg);
            for (; optind < argc; optind++)
            {
                delete_path(filename, argv[optind]);
            }
            free(filename);
            exit(EXIT_SUCCESS);
            break;
        case 'u':
            // Retrieve the first argument for filename from list of files
            printf("The first argument is: %s\n", optarg);
            // Retrieve the extra arguments of filenames if any
            for (; optind < argc; optind++)
            {
                printf("The extra arguments are: %s\n", argv[optind]);
            }
            free(filename);
            exit(EXIT_SUCCESS);
            break;
        case 'b':
            // Retrieve the first argument for filename from list of files
            is_dir(optarg, min_len);
            for (; optind < argc; optind++)
            {
                is_dir(argv[optind], min_len);
            }
            // Trove file handler
            if (strlen(filename) == 0)
            {
                printf("The trove-file name to be built or searched is: trove\n");
                // Go into the tmp directory
                // save file as trove
                build_trove(ht, "/tmp/trove");
            }
            else
            {
                char *path;
                if ((path = malloc(strlen("./") + strlen(filename) + 1)) != NULL)
                {
                    path[0] = '\0';
                    strcat(path, "./");
                    strcat(path, filename);
                    build_trove(ht, path);
                }
                else
                {
                    // Error message...
                }
                free(path);
            }
            free(filename);
            exit(EXIT_SUCCESS);
            break;
        case '?':
            printf("Invalid argument!\n");
            break;
        default:
            printf("ERROR!!\n");
            break;
        }
    }

    // Read through trove file test
    search_word("trove-file", "main");

    return 0;
}

// ------------------------------ DEBUG CODE ------------------------------- //

// search_word("trove-file.txt", "hello");

// char buffer[PATH_MAX]; // Get the new absolute path to pass into function for recursive file search
// char *abs_path = realpath("/tmp", buffer);

// printf("%s\n", buffer);

// Cases
// 0            1       2            3          4           5           6...inf
// ----------------------------------- BUILD ------------------------------------- //
// ./trove      -f      <filename>   -b         -l          <length>    <filelist>
// ./trove      -f      <filename>   -b         <filelist>
// ./trove      -b      -l           <length>   <filelist>
// ./trove      -b      <filelist>
// ----------------------------------- UPDATE ------------------------------------- //
// ./trove      -f      <filename>   -u         -l          <length>    <filelist>
// ./trove      -f      <filename>   -u         <filelist>
// ./trove      -u      -l           <length>   <filelist>
// ./trove      -u      <filelist>
// ----------------------------------- REMOVE ------------------------------------- //
// ./trove      -f      <filename>   -r         -l          <length>    <filelist>
// ./trove      -f      <filename>   -r         <filelist>
// ./trove      -r      -l           <length>   <filelist>
// ./trove      -r      <filelist>
// -------------------------------- WORD RETRIEVE --------------------------------- //
// ./trove      -f      <filename>   <word>
// ./trove      <word>