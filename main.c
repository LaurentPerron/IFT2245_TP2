/**
 * William Bach
 * Laurent Perron 1052137
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <pthread.h>
#include <unistd.h>
#include <wait.h>

typedef unsigned char bool;
typedef struct command_struct command;
typedef struct command_chain_head_struct command_head;

typedef enum {
    BIDON, NONE, OR, AND, ALSO
} operator;

struct command_struct {
    int *ressources;
    char **call;
    int call_size;
    int count;
    operator op;
    command *next;
};

struct command_chain_head_struct {
    int *max_resources;
    int max_resources_count;
    command *command;
    pthread_mutex_t *mutex;
    bool background;
};

// Forward declaration
typedef struct banker_customer_struct banker_customer;

struct banker_customer_struct {
    command_head *head;
    banker_customer *next;
    banker_customer *prev;
    int *current_resources;
    int depth;
};

void print_command_chain(command_head head){

}

typedef int error_code;
#define HAS_ERROR(err) ((err) < 0)
#define HAS_NO_ERROR(err) ((err) >= 0)
#define NO_ERROR 0
#define CAST(type, src)((type)(src))
#define ERROR (-1)
#define NULL_TERMINATOR '\0'

typedef struct {
    char **commands;
    int *command_caps;
    unsigned int command_count;
    unsigned int ressources_count;
    int file_system_cap;
    int network_cap;
    int system_cap;
    int any_cap;
} configuration;

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------

// Configuration globale
configuration *conf = NULL;

/**
 * Lis une ligne du shell.
 * @param out variable dans laquelle la ligne est enregistrée.
 * @return erreur ou 0.
 */
error_code readLine(char **out) {
    size_t size = 10;                       // size of the char array
    char *line = malloc(sizeof(char) * size);       // initialize a ten-char line
    if (line == NULL) return ERROR;   // if we can't, terminate because of a memory issue

    for (int at = 0; 1; at++) {
        if (at >= size) {
            size = 2 * size;
            char *tempPtr = realloc(line, size * sizeof(char));
            if (tempPtr == NULL) {
                free(line);
                return ERROR;
            }

            line = tempPtr;
        }
        int tempCh = getchar();

        if (tempCh == EOF) { // if the next char is EOF, terminate
            free(line);
            return ERROR;
        }
        char ch = (char) tempCh;

        if (ch == '\n') {        // if we get a newline
            line[at] = NULL_TERMINATOR;    // finish the line with return 0
            break;
        }
        line[at] = ch; // sets ch at the current index and increments the index
    }
    out[0] = line;
    return 0;
}

void freeStringArray(char **arr) {
    if (arr != NULL) {
        for (int i = 0; arr[i] != NULL; i++) {
            free(arr[i]);
        }
    }
    free(arr);
}

void freeConfiguration(configuration *config) {
        if (config == NULL) return;
        if (config->commands != NULL) {
            for(int i = 0; i < config->command_count; i++) {
                free(config->commands[i]);
            }
            free(config->commands);
        }
        if (config->command_caps != NULL) free(conf->command_caps);
        free(config);
}

error_code parse_first_line(const char *line) {
    char *copy;
    char *first_block;
    char *second_block;
    char *tok;
    int count;
    conf = malloc(sizeof(configuration));
    copy = strdup(line);
    if (copy == NULL) return ERROR;

    first_block = strdup(strtok(copy, "&")); // Bloc suppose des commands
    if(strcmp(line, first_block) == 0) {
        free(copy);
        return ERROR;
    }
    second_block = strdup(strtok(NULL, "&")); // bloc suppose des caps
    free(copy);
    // On regarde si fist_token[0] == lettre ou nombre
    if(first_block == NULL || second_block == NULL){
        return ERROR;
    }
    if (!(first_block[0] >= '0' && first_block[0] <= '9')) { // On a des commandes supp
       count = 1;
       int i = 0;
       while(first_block[i] != NULL_TERMINATOR) {
           if(first_block[i] == ',') count++;
           i++;
       }
       conf->commands = (char **)malloc(count * sizeof(char *));
       conf->command_count = count;
       conf->ressources_count = count + 4;
       char *coms = strdup(first_block);
       char *com = strtok(coms, ",");
       int j = 0;
       while(j < count) { // créer le tableau des commandes
           conf->commands[j] = strdup(com);

           com = strtok(NULL, ",");
           j++;
       }
       // Créer le tableau des ressources
       conf->command_caps = (int *)malloc(count * sizeof(int));
       char *caps = strdup(second_block);
       char *cap = strtok(caps, ",");
       int k = 0;
        while(k < count) { // créer le tableau des commandes
            if(cap == NULL) return ERROR;
            conf->command_caps[k] = (int) strtol(cap, NULL , 10);
            cap = strtok(NULL, ",");
            k++;
        }
        free(coms);
        free(com);
        free(caps);
        free(cap);
    } else {
        count = 0;
        conf->command_caps = NULL;
        conf->commands = NULL;
        conf->command_count = 0;
        conf->ressources_count = 4;
    }
    copy = strdup(line);
    if (copy == NULL) return ERROR;
    tok = strtok(copy, "&");
    if(count > 0) {
        tok = strtok(NULL, "&");
        tok = strtok(NULL, "&");
    }
    if(tok == NULL) return ERROR;
    conf->file_system_cap = (int) strtol(tok, NULL, 10);
    tok = strtok(NULL, "&");
    if(tok == NULL) return ERROR;
    conf->network_cap = (int) strtol(tok, NULL, 10);
    tok = strtok(NULL, "&");
    if(tok == NULL) return ERROR;
    conf->system_cap = (int) strtol(tok, NULL, 10);
    tok = strtok(NULL, "&");
    if(tok == NULL) return ERROR;
    conf->any_cap = (int) strtol(tok, NULL, 10);

    free(copy);
    free(first_block);
    free(second_block);
    //free(tok);
    return NO_ERROR;
}

/**
 * Cette fonction analyse la première ligne et remplie la configuration
 * @param line la première ligne du shell
 * @return un code d'erreur (ou rien si correct)
 */
error_code parse_first_line_OLD(char *line) {
    // TODO Bug a corriger
    char *commands = NULL;
    char **commands_conf = NULL;
    char *command_caps = NULL;
    int *command_caps_conf = NULL;
    char *all_caps = NULL;
    conf = malloc(sizeof(configuration));


    //on trouve les commandes à limiter
    int nextAnd = 0;
    while (1) {
        if (line[nextAnd] == '&') break;
        nextAnd++;
        if (line[nextAnd] == NULL_TERMINATOR) {
            free(conf);
            return ERROR;
        }
    }
    commands = malloc(sizeof(char) * (nextAnd + 2)); //nom des commandes à limiter
    if(commands == NULL) goto error;

    int i = 0;
    for(; i < nextAnd; i++) commands[i] = line[i];
    commands[i] = NULL_TERMINATOR;

    //on place les commandes dans la configuration
    commands_conf = malloc(sizeof(char) * strlen(commands));
    if (commands_conf == NULL) goto error;

    i = 0;
    int command_count = 0;
    char *token = strtok(commands, ",");
    while(token != NULL) {
        char *arg = malloc(sizeof(char) * (strlen(token) + 1));
        strcpy(arg, token);

        commands_conf[i] = arg;
        command_count++;

        i++;
        token = strtok(NULL, ",");
    }
    commands_conf[command_count] = NULL_TERMINATOR;
    conf->commands = commands_conf;
    conf->command_count = command_count;
    free(commands);
    freeStringArray(commands_conf);


    //on trouve la limitation des commandes
    int new_nextAnd = nextAnd + 1;
    while(1) {
        if(line[new_nextAnd] == '&') break;
        new_nextAnd++;
    }
    command_caps = malloc(sizeof(int) * (new_nextAnd - nextAnd + 1)); // capacité des commandes
    if(command_caps == NULL) goto error;

    int j = 0;
    for(; j < new_nextAnd - nextAnd - 1; j++) command_caps[j] = line[j + nextAnd + 1];
    command_caps[j] = NULL_TERMINATOR;

    //on place les capacités dans la configuration
    command_caps_conf = malloc(sizeof(int) * strlen(command_caps));
    if (command_caps_conf == NULL) goto error;

    j = 0;
    int nb = 0;
    int ressources_count = 0;
    char *token2 = strtok(command_caps, ",");
    while(token2 != NULL) {
        char *arg = malloc(sizeof(int) * strlen(token2));
        strcpy(arg, token2);

        nb = atoi(arg);
        command_caps_conf[j] = nb;

        j++;
        ressources_count++;
        token2 = strtok(NULL, ",");
    }
    conf->command_caps = command_caps_conf;
    conf->ressources_count = ressources_count + 3;
    free(command_caps);
    free(command_caps_conf);


    //on trouve les autres capacités
    int line_length = strlen(line);
    all_caps = malloc(sizeof(int) * (line_length - new_nextAnd));
    if(all_caps == NULL) goto error;

    int k = new_nextAnd + 1;
    for(; k < line_length; k++) all_caps[k-new_nextAnd-1] = line[k];
    all_caps[k] = NULL_TERMINATOR;

    //on place les autres capacités dans la configuration
    char *token3 = strtok(all_caps, "&");
    int loop = 0;
    while(token3 != NULL) {
        char *arg = malloc(sizeof(int) * strlen(token3));
        strcpy(arg, token3);

        nb = atoi(arg);
        loop++;
        switch(loop) {
            case 1: conf->file_system_cap = nb;
                    break;

            case 2: conf->network_cap = nb;
                    break;

            case 3: conf->system_cap = nb;
                    break;

            case 4: conf-> any_cap = nb;
                    break;

            default: goto error;
        }
        token3 = strtok(NULL, "&");
    }
    return NO_ERROR;

    error:
    free(commands);
    freeStringArray(commands_conf);
    free(command_caps);
    free(command_caps_conf);
    free(all_caps);
    freeConfiguration(conf);
    return ERROR;
}

command *freeAndNext(command *current) {
    if (current == NULL) return current;

    command *next = current->next;
    freeStringArray(current->call);
    free(current);
    return next;
}

void freeCommands(command *head) {
    command *current = head;
    while (current != NULL) current = freeAndNext(current);
}

#define FS_CMDS_COUNT 10
#define FS_CMD_TYPE 0
#define NETWORK_CMDS_COUNT 6
#define NET_CMD_TYPE 1
#define SYS_CMD_COUNTS 3
#define SYS_CMD_TYPE 2

const char *FILE_SYSTEM_CMDS[FS_CMDS_COUNT] = {
        "ls",
        "cat",
        "find",
        "grep",
        "tail",
        "head",
        "mkdir",
        "touch",
        "rm",
        "whereis"
};

const char *NETWORK_CMDS[NETWORK_CMDS_COUNT] = {
        "ping",
        "netstat",
        "wget",
        "curl",
        "dnsmasq",
        "route"
};

const char *SYSTEM_CMDS[SYS_CMD_COUNTS] = {
        "uname",
        "whoami",
        "exec"
};

/**
 * Cette fonction prend en paramètre un nom de ressource et retourne
 * le numéro de la catégorie de ressource
 * @param res_name le nom
 * @return le numéro de la catégorie (ou une erreur)
 */
error_code resource_no(char *res_name) {
    //On considère que les catégories définies lors de l'initialisation ont le numéro
    //3, 4, 5,.. dans l'ordre d'initialisation.

    //on checke si la ressource a été définie à l'initialisation.
    int cat_count = (int)conf->command_count;
    if(cat_count == 0) goto no_commands;

    for(int i=0; i < cat_count; i++) {

        if (strcmp(res_name, conf->commands[i]) == 0) {
            int cat = i + 3;
            return cat;
        }
    }
    no_commands:
    //sinon elle fait partie des ressources déjà définies ou de "other".
    //file system?
    for (int i=0; i < FS_CMDS_COUNT; i++) {

        if(strcmp(res_name, FILE_SYSTEM_CMDS[i]) == 0) {
            int cat = 0;
            return cat;
        }
    }
    //network system?
    for (int i=0; i < NETWORK_CMDS_COUNT; i++) {

        if(strcmp(res_name, NETWORK_CMDS[i]) == 0) {
            int cat = 1;
            return cat;
        }
    }
    //system?
    for (int i=0; i < SYS_CMD_COUNTS; i++) {

        if(strcmp(res_name, SYSTEM_CMDS[i]) == 0) {
            int cat = 2;
            return cat;
        }
    }
    //si aucune catégorie, c'est other.
    int cat = cat_count + 3;
    return cat;
}

/**
 * Cette fonction prend en paramètre un numéro de catégorie de ressource et retourne
 * la quantitée disponible de cette ressource
 * @param resource_no le numéro de ressource
 * @return la quantité de la ressource disponible
 */
int resource_count(int resource_no) {

    //la catégorie est déjà prédéfinie
    switch(resource_no) {
        case 0:  return conf->file_system_cap;

        case 1: return conf->network_cap;

        case 2: return conf->system_cap;

        default:           break;
    }

    //on calcule le nombre de catégories définies à l'initialisation.
    unsigned int cat_count = conf->command_count;
    //catégorie définie à l'initialisation?
    if (resource_no > 2 && resource_no < cat_count + 3) return conf->command_caps[resource_no-3];
    //other?
    else if (resource_no == cat_count + 3) return conf->any_cap;
    else return ERROR;
}

// Forward declaration
error_code evaluate_whole_chain(command_head **head);

/**
 * Créer une chaîne de commande qui correspond à une ligne de commandes
 * @param line la ligne de texte à parser
 * @param result le résultat de la chaîne de commande
 * @return un code d'erreur
 */
error_code create_command_chain(char *line, command_head **result) {
    char **call = NULL;
    char *wordPtr = NULL;
    command *c = NULL;

    int currentCallSize = 0;
    command_head *h = NULL;
    command *current = NULL;
    command *head = NULL;
    h = (command_head *)malloc(sizeof(command_head));
    // TODO Regarder si == -1

    int escaped = 0;

    for (int index = 0; line[index] != NULL_TERMINATOR; index++) {
        int nextSpace = index;
        while (1) {
            if (!escaped) {
                if (line[nextSpace] == ' ' || line[nextSpace] == NULL_TERMINATOR) {
                    break;
                }
            }
            if (line[nextSpace] == '(') escaped = 1;
            if (line[nextSpace] == ')') escaped = 0;
            nextSpace++;
        }

        wordPtr = malloc(sizeof(char) * (nextSpace - index + 1));
        if (wordPtr == NULL) goto error;

        int i = 0;
        for (; i < nextSpace - index; i++) wordPtr[i] = line[i + index];
        wordPtr[i] = NULL_TERMINATOR;

        operator operator = BIDON;
        if (strcmp(wordPtr, "||") == 0) operator = OR;
        if (strcmp(wordPtr, "&&") == 0) operator = AND;
        if (strcmp(wordPtr, "&") == 0) operator = ALSO;
        if (operator != ALSO && line[nextSpace] == NULL_TERMINATOR) operator = NONE;

        if(operator == OR || operator == AND || operator == ALSO) free(wordPtr);

        if (operator == BIDON || operator == NONE) {
            if (call == NULL) {
                currentCallSize = 1;
                call = malloc(sizeof(char *) * 2);
                if (call == NULL) goto error;
                call[1] = NULL;
            } else {
                currentCallSize++;
                char **tempPtr = realloc(call, (currentCallSize + 1) * sizeof(char *));
                if (tempPtr == NULL) goto error;

                call = tempPtr;
                call[currentCallSize] = NULL;
            }

            call[currentCallSize - 1] = wordPtr;
        }

        if (operator != BIDON) {
            c = malloc(sizeof(command));
            if (c == NULL) goto error;

            if (head == NULL) head = c;
            else current->next = c;

            c->count = 1;
            if (call[0][strlen(call[0]) - 1] == ')') {
                char *command = call[0];

                unsigned long command_len = strlen(command); //rn et fn ici

                int paren_pos = 0;
                for (; command[paren_pos] != '('; paren_pos++);

                wordPtr = malloc(paren_pos * sizeof(char));
                if(wordPtr == NULL) goto error;

                //isoler le nombre N de rN() et fN() dans wordPtr
                for (int j = 0; j < paren_pos; j++) wordPtr[j] = command[j + 1];
                wordPtr[paren_pos - 1] = NULL_TERMINATOR;

                int nb = atoi(wordPtr); // NOLINT(cert-err34-c)
                c->count = ('r' == command[0]) ? nb : -nb;

                free(wordPtr);
                if (NULL == (wordPtr = malloc(sizeof(char) * (command_len - paren_pos - 1)))) {
                    goto error;
                }

                memcpy(wordPtr, &command[paren_pos + 1], command_len - paren_pos - 2);
                wordPtr[command_len - paren_pos - 2] = NULL_TERMINATOR;
                free(command);

                unsigned long resized_len = strlen(wordPtr);

                int space_nb = 0;
                for (int j = 0; j < resized_len; j++) {
                    space_nb += wordPtr[j] == ' ';
                }

                char **temp;
                if (NULL == (temp = realloc(call, sizeof(char *) * (space_nb + 2)))) {
                    goto error;
                } else {
                    call = temp;
                    call[space_nb + 1] = NULL;
                }

                int copy_index = 0;
                char *token = strtok(wordPtr, " ");
                while (token != NULL) {
                    char *arg = malloc(sizeof(char) * (strlen(token)+1));
                    /* if (NULL == (arg )) {
                         goto error;
                     }*/

                    strcpy(arg, token);
                    call[copy_index] = arg;

                    token = strtok(NULL, " ");

                    copy_index++;
                }
                c->call_size = space_nb + 2;
                free(wordPtr);
            }

            c->call = call;
            c->next = NULL;
            c->op = operator;
            h->background = 0;
            if (operator == ALSO) {
                c->op = NONE;
                h->background = 1;
            }
            current = c;
            call = NULL;
        }
        if (line[nextSpace] == NULL_TERMINATOR) break;
        index = nextSpace;
    }
    h->command = head;
    result[0] = h;
    return 0;

    error:
    free(wordPtr);
    free(line);
    freeStringArray(call);
    freeAndNext(c);
    freeCommands(head);
    return ERROR;
    return NO_ERROR;
}

/**
 * Cette fonction compte les ressources utilisées par un block
 * La valeur interne du block est mise à jour
 * @param command_block le block de commande
 * @return un code d'erreur
 */
error_code count_ressources(command_head **head, command **command_block) {
    char *c = (*command_block)->call[0];
    int count = (*command_block)->count;
    unsigned int len = 4;
    len = conf->ressources_count;
    int ressources[len];
    for(int i = 0; i < len; i++) {
        ressources[i] = 0;
    }
    int index = resource_no(c);
    ressources[index] = count;

    (*command_block)->ressources = ressources;
    (*head)->max_resources[index] += count;
    return NO_ERROR;
}

/**
 * Cette fonction parcours la chaîne et met à jour la liste des commandes
 * @param head la tête de la chaîne
 * @return un code d'erreur
 */
error_code evaluate_whole_chain(command_head **head) {
    int len = (int)conf->ressources_count;
    (*head)->max_resources_count = len;
    int max_ressources[len];
    for(int i = 0; i < len; i++) {
        max_ressources[i] = 0;
    }
    (*head)->max_resources = max_ressources;
    command *c = (*head)->command;

    while(c != NULL) {
        count_ressources(head, &c);
        c = c->next;
    }
    return NO_ERROR;
}

// ---------------------------------------------------------------------------------------------------------------------
//                                              BANKER'S FUNCTIONS
// ---------------------------------------------------------------------------------------------------------------------

static banker_customer *first;
static pthread_mutex_t *register_mutex = NULL;
static pthread_mutex_t *available_mutex = NULL;
// Do not access directly!
// TODO use mutexes when changing or reading _available!
int *_available = NULL;


/**
 * Cette fonction enregistre une chaîne de commande pour être exécutée
 * Elle retourne NULL si la chaîne de commande est déjà enregistrée ou
 * si une erreur se produit pendant l'exécution.
 * @param head la tête de la chaîne de commande
 * @return le pointeur vers le compte client retourné
 */
banker_customer *register_command(command_head *head) {
    return NULL;
}

/**
 * Cette fonction enlève un client de la liste
 * de client de l'algorithme du banquier.
 * Elle libère les ressources associées au client.
 * @param customer le client à enlever
 * @return un code d'erreur
 */
error_code unregister_command(banker_customer *customer) {
    return NO_ERROR;
}

/**
 * Exécute l'algo du banquier sur work et finish.
 *
 * @param work
 * @param finish
 * @return
 */
int bankers(int *work, int *finish) {
    return 0;
}

/**
 * Prépare l'algo. du banquier.
 *
 * Doit utiliser des mutex pour se synchroniser. Doit allour des structures en mémoire. Doit finalement faire "comme si"
 * la requête avait passé, pour défaire l'allocation au besoin...
 *
 * @param customer
 */
void call_bankers(banker_customer *customer) {
}

/**
 * Parcours la liste de clients en boucle. Lorsqu'on en trouve un ayant fait une requête, on l'évalue et recommence
 * l'exécution du client, au besoin
 *
 * @return
 */
void *banker_thread_run() {
    return NULL;
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------

/**
 * Cette fonction effectue une requête des ressources au banquier. Utilisez les mutex de la bonne façon, pour ne pas
 * avoir de busy waiting...
 *
 * @param customer le ticket client
 * @param cmd_depth la profondeur de la commande a exécuter
 * @return un code d'erreur
 */
error_code request_resource(banker_customer *customer, int cmd_depth) {
    return NO_ERROR;
}

/**
 * Utilisez cette fonction pour initialiser votre shell
 * Cette fonction est appelée uniquement au début de l'exécution
 * des tests (et de votre programme).
 */
error_code init_shell() {
    char *line;

    //initialisation et configuration
    while(1) {
        if (HAS_ERROR(readLine(&line))) goto error;
        if (strlen(line) == 0) continue;
        if(HAS_ERROR(parse_first_line(line))) goto error;
        free(line);
        break;
    }

    error_code err = NO_ERROR;
    return err;

    error:
    free(line);
    printf("An error has occured\n");
    exit(-1);
}

/**
 * Utilisez cette fonction pour nettoyer les ressources de votre
 * shell. Cette fonction est appelée uniquement à la fin des tests
 * et de votre programme.
 */
void close_shell() {

    freeConfiguration(conf);
}

int callCommand(command *current) {
    if (current->count <= 0) return 1;
    if (strcmp(current->call[0], "exit") == 0) return 7;

    int exitCode = -1;     // the exit code of the child
    pid_t pid = 0;
    for (int i = 0; i < current->count; i++) {
        pid = fork();
        if (pid < 0) {        // forking failed
            return pid;
        } else if (pid == 0) {
            // -----------------------------------------------------
            //                    CHILD PROCESS
            // -----------------------------------------------------
            char *cmd_name = current->call[0];
            execvp(cmd_name, current->call);    // execvp searches for command[0] in PATH, and then calls command

            printf("bash: %s: command not found\n", cmd_name);    // if we reach this, exec couldn't find the command

            exit(49);    // and then we exit with 2, signaling an error. This also frees ressources
        }
    }

    // PID is correct
    waitpid(pid, &exitCode, 0); // Wait for the child process to exit.
    int x = WIFEXITED(exitCode);
    if (!x) return 0;

    x = WEXITSTATUS(exitCode);
    if (x != 0) return 0;
    return 1;
}

error_code callCommands(command *current) {
    if (current == NULL || current->call == NULL) return 0;

    int ret = callCommand(current);
    if (ret == 7) return 7;
    if (ret == -1) return ERROR;

    operator op = current->op;
    command *next = current->next;

    switch (op) {
        default:
        case NONE:
        case BIDON:
            return 0;
        case AND:
            if (ret) return callCommands(next);
            return 0;
        case OR:
            if (ret) {
                next = next->next;
                while (next != NULL && (next->op == OR || next->op == NONE)) next = next->next;
                if (next != NULL && next->op == AND) next = next->next;
            }
            return callCommands(next);
    }
}
/**
 * Utilisez cette fonction pour y placer la boucle d'exécution (REPL)
 * de votre shell. Vous devez aussi y créer le thread banquier
 */
void run_shell() {
    int exit_code = 0;
    char *line;
    command_head *head;
    command *f;

    while (1) {
        if (HAS_ERROR(readLine(&line))) goto bot;
        if (strlen(line) == 0) continue;

        if (HAS_ERROR(create_command_chain(line, &head))) goto bot;
        evaluate_whole_chain(&head);
        for(int i = 0; i < conf->ressources_count; i++) {
            printf("%d, ", head->max_resources[i]);
        }
        printf("\n");
        free(line);
        f = head->command;
        if (head->background) {
            pid_t pid = fork();
            if (pid == -1) {        // forking failed
                freeCommands(f);
                free(head);
            } else if (pid == 0) {    // child
                if (HAS_ERROR(exit_code = callCommands(f))) goto toptop;
                exit(0);    // and then we exit with 2, signaling an error
            }

        } else exit_code = callCommands(f);

        freeCommands(f);
        free(head);
        if(exit_code == 7) exit(0);
    }

    toptop:
    freeCommands(f);
    free(head);
    bot:
    free(line);
    top:
    printf("An error has occured");
    exit(-1);
}

/**
 * Vous ne devez pas modifier le main!
 * Il contient la structure que vous devez utiliser. Lors des tests,
 * le main sera complètement enlevé!
 */
int main(void) {
    char * line  = "r20(echo aa) && f10(echo bb) || echo cc &";
    char *line_one = "echo,sed,ls&7,8,9&10&11&12&13";
    parse_first_line(line_one);
    command_head *h;
    create_command_chain(line, &h);
    evaluate_whole_chain(&h);
    printf("%d, %d, %d, %d, %d, %d, %d\n", h->max_resources[0], h->max_resources[1], h->max_resources[2], h->max_resources[3], h->max_resources[4], h->max_resources[5], h->max_resources[6]);
    freeConfiguration(conf);
    conf = NULL;
    if (HAS_NO_ERROR(init_shell())) {
        run_shell();
        close_shell();
    } else {
        printf("Error while executing the shell.");
    }
}
