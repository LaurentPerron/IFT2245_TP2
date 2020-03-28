
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

#define print(...) do{printf(__VA_ARGS__); fflush(stdout); } while(0)

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

typedef struct {
    command *com;
    int exit_code;
} t_args;

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


//Fonctions de libération de mémoire
void freeStringArray(char **arr) {
    if (arr != NULL) {
        for (int i = 0; arr[i] != NULL; i++) {
            free(arr[i]);
        }
    }
    free(arr);
}

command *freeAndNext(command *current) {
    if (current == NULL) return current;

    command *next = current->next;
    freeStringArray(current->call);
    free(current->ressources);
    free(current);
    return next;
}

void freeCommands(command *head) {
    command *current = head;
    while (current != NULL) current = freeAndNext(current);
}

void freeConfiguration(configuration *config) {
    if (config == NULL) return;

    if (config->commands != NULL) {
        freeStringArray(config->commands);
    }
    if (config->command_caps != NULL) free(conf->command_caps);
    free(config);
}


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

/**
 * Cette fonction analyse la première ligne et remplie la configuration
 * @param line la première ligne du shell
 * @return un code d'erreur (ou rien si correct)
 */
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
    return NO_ERROR;
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
            int cat = FS_CMD_TYPE;
            return cat;
        }
    }
    //network system?
    for (int i=0; i < NETWORK_CMDS_COUNT; i++) {

        if(strcmp(res_name, NETWORK_CMDS[i]) == 0) {
            int cat = NET_CMD_TYPE;
            return cat;
        }
    }
    //system?
    for (int i=0; i < SYS_CMD_COUNTS; i++) {

        if(strcmp(res_name, SYSTEM_CMDS[i]) == 0) {
            int cat = SYS_CMD_TYPE;
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
        case FS_CMD_TYPE:  return conf->file_system_cap;

        case NET_CMD_TYPE: return conf->network_cap;

        case SYS_CMD_TYPE: return conf->system_cap;

        default:           break;
    }

    //on calcule le nombre de catégories définies à l'initialisation.
    unsigned int cat_count = conf->command_count;
    //catégorie définie à l'initialisation?
    if (resource_no > SYS_CMD_TYPE && resource_no < cat_count + 3) return conf->command_caps[resource_no-3];
    //other?
    else if (resource_no == cat_count + 3) return conf->any_cap;
    else return ERROR;
}

// Forward declaration
error_code evaluate_whole_chain(command_head *head);

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
    if(h == NULL) return ERROR;

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
    h->mutex = malloc(sizeof(pthread_mutex_t));
    if (h->mutex== NULL) goto error;
    pthread_mutex_init(h->mutex, NULL);
    result[0] = h;
    return NO_ERROR;

    error:
    free(wordPtr);
    free(line);
    freeStringArray(call);
    freeAndNext(c);
    freeCommands(head);
    return ERROR;
}

/**
 * Cette fonction compte les ressources utilisées par un block
 * La valeur interne du block est mise à jour
 * @param command_block le block de commande
 * @return un code d'erreur
 */
error_code count_ressources(command_head *head, command *command_block) {
    char *c = command_block->call[0];
    int count = command_block->count;
    unsigned int len;
    len = conf->ressources_count;
    int *ressources = (int *)malloc(sizeof(int) * len);
    if(ressources == NULL) return ERROR;
    for(int i = 0; i < len; i++) {
        ressources[i] = 0;
    }
    int index = resource_no(c);
    ressources[index] = count;

    command_block->ressources = ressources;
    //head->max_resources[index] += count;
    return NO_ERROR;
}

/**
 * Cette fonction parcours la chaîne et met à jour la liste des commandes
 * @param head la tête de la chaîne
 * @return un code d'erreur
 */
error_code evaluate_whole_chain(command_head *head) {
    int len = (int)conf->ressources_count;
    head->max_resources_count = len;
    int *max_ressources = (int *)malloc(sizeof(int) * len);
    for(int i = 0; i < len; i++) {
        max_ressources[i] = 0;
    }
    head->max_resources = max_ressources;
    command *c = head->command;

    // Count ressources
    while(c != NULL) {
        count_ressources(head, c);
        c = c->next;
    }
    // Adds them up
    for(int j = 0; j < len; j++) {
        int max = 0;
        int sum = 0;
        command *current = head->command;
        while(current != NULL) {
            sum += current->ressources[j];
            if(max < sum) {
                max += current->ressources[j];
            }

            current  = current->next;
        }
        head->max_resources[j] = max;
    }

    return NO_ERROR;
}

// ---------------------------------------------------------------------------------------------------------------------
//                                              BANKER'S FUNCTIONS
// ---------------------------------------------------------------------------------------------------------------------

pthread_t banker_tid;
static banker_customer *first;
static pthread_mutex_t *register_mutex = NULL;
static pthread_mutex_t *available_mutex = NULL;
// Do not access directly!
int *_available = NULL;
int stop_banker = 0;


/**
 * Cette fonction enregistre une chaîne de commande pour être exécutée.
 * Elle retourne NULL si la chaîne de commande est déjà enregistrée ou
 * si une erreur se produit pendant l'exécution.
 * @param head la tête de la chaîne de commande
 * @return le pointeur vers le compte client retourné
 */
banker_customer *register_command(command_head *head) {
    banker_customer *customer = (banker_customer *) malloc(sizeof(banker_customer));
    if (customer == NULL) {
        free(customer);
        goto error;
    }

    customer->head = head;
    customer->next = NULL;
    customer->prev = NULL;
    //customer->depth = 0;
    customer->depth = -1;
    customer->current_resources = (int *)malloc(sizeof(int) * head->max_resources_count);
    if (customer->current_resources == NULL) {
        free(customer);
        goto error;
    }
    command *current = head->command;
    int count = 0;

    while (current != NULL) {
        count++;
        current = current->next;
    }

    //on crée la liste chaînée
    //int depth = 1;
    for (int i=1; i<count; i++) {
        banker_customer *temp = (banker_customer *) malloc(sizeof(banker_customer));
        if (temp == NULL) {
            free(customer);
            goto error;
        }
        customer->next = temp;
        temp->head = head;
        temp->prev = customer;
        temp->current_resources = (int *)malloc(sizeof(int) * head->max_resources_count);
        if (temp->current_resources == NULL) {
            free(temp->current_resources);
            free(temp);
            goto error;
        }
       //temp->depth = depth;
        temp->depth = -1;

        //depth++;
        customer = temp;
    }

    while (customer->prev != NULL) {
        customer = customer->prev;
    }
    return customer;

    error:
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
    customer->depth = -1; return NO_ERROR;
    //le client est le premier de la liste
    if (customer->prev == NULL) {
        //il n'y a qu'un client
        if (customer->next == NULL) {
            free(first->current_resources);
            free(first);
            first = NULL;
            return NO_ERROR;
        }

        //il y a plus d'un client
        else {
            first = customer->next;
            first->prev = NULL;
        }
    }

    //le client n'est pas premier de la liste
    else {
        //le client est dernier de la liste
        if ((banker_customer *)customer->next == NULL) customer->prev->next = NULL;

        //le client n'est ni dernier ni premier
        else {
            customer->prev->next = customer->next;
            customer->next->prev = customer->prev;
        }
    }

    end:
    free(customer->current_resources);
    free(customer);
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
    //print("into banker\n");
    // 1.
    // Already initialize
    command *c;
    int len = 0;
    banker_customer *current = first;
    while(current != NULL) {
        if(current->depth > -1) len++;
        current = current->next;
    }

    // 2.
    for(int i = 0; i < len; i++) {
        if(finish[i] == 0) {
            // On trouve le client correspondant a la position i dans finish
            int j = 0;
            current = first;
            while(current->next != NULL) {
                if(current->depth > -1) {
                    if(j == i) break;
                    else j++;
                }
                current = current->next;
            }
            c = current->head->command;
            for(int k = 0; k < current->depth; k++) {
                c = c->next;
            }

            // Creation de neaded
            int *needed = (int *)malloc(sizeof(int) * conf->ressources_count);
            needed = memcpy(needed, c->ressources, conf->ressources_count * sizeof(int));

            // Si on a deja pre-accorde les ressources, alors on les soustrait
            for(int n = 0; n < conf->ressources_count; n++) {
                needed[n] -= current->current_resources[n];
            }
            // Est-ce qu'on a besoin de trop de ressources ?
            int stop = 0;
            for(int l = 0; l < conf->ressources_count; l++) {
                if(needed[l] > work[l]) stop = 1;
            }
            if (stop == 1) {
                free(needed);
                break;
            }
            // 3. on libere les ressources occupes
            for(int m = 0; m < conf->ressources_count; m++) {
                work[m] += current->current_resources[m];
            }
            free(needed);
            finish[i] = 1;
        } // else continue
    }
    // 4.
    for(int n =  0; n < len; n++) {
        if(finish[n] == 0) return 0;
    }
    return 1;
}

/**
 * Prépare l'algo. du banquier.
 *
 * Doit utiliser des mutex pour se synchroniser. Doit allouer des structures en mémoire. Doit finalement faire "comme si"
 * la requête avait passé, pour défaire l'allocation au besoin...
 *
 * @param customer
 */
void call_bankers(banker_customer *customer) {
    pthread_mutex_lock(available_mutex);
    int safe_state;
    int *finish;
    command * c = customer->head->command;
    for(int j = 0; j < customer->depth; j++) {
        c = c->next;
    }
    memcpy(customer->current_resources, c->ressources, sizeof(int) * conf->ressources_count);

    // Assignation provisoire des ressources
    for(int i = 0; i < conf->ressources_count; i++) {
        _available[i] -= customer->current_resources[i];
        //print("%d, ", customer->current_resources[i]);
    }
   //print("\n");

    int len = (int)conf->ressources_count;
    int *work = (int *)malloc(sizeof(int) * len);
    work = memcpy(work, _available, sizeof(int) * len);

    int finish_len = 0;

    banker_customer *current = first;
    while(current != NULL) {
        // On compte les clients ayant fait une demande
        if(current->depth > -1) finish_len++;
        current = current->next;
    }
    finish = (int *)malloc(sizeof(int) * finish_len);
    if(finish == NULL) return;
    // On rempli finish avec des 0.
    for(int k = 0; k < finish_len; k++) {
        finish[k] = 0;
    }

    safe_state = bankers(work, finish);
    if(safe_state) {
        // On commence l'execution et on enleve le client
        unregister_command(customer);
        pthread_mutex_unlock(customer->head->mutex);
    } else {
        // On retire le
        for(int j = 0; j < conf->ressources_count; j++) {
            _available[j] += customer->current_resources[j];
            customer->current_resources[j] = 0;
        }
    }
    pthread_mutex_unlock(available_mutex);
    free(work);
    free(finish);
}

/**
 * Parcours la liste de clients en boucle. Lorsqu'on en trouve un ayant fait une requête, on l'évalue et recommence
 * l'exécution du client, au besoin
 *
 * @return
 */
void *banker_thread_run() {
    banker_customer *customer = first;
    while(1) {
        if(stop_banker) break;
        if (first == NULL) {
            continue;
        }
        pthread_mutex_lock(register_mutex);
        while(customer != NULL && customer->depth < 0) {
            customer = customer->next;
        }
        if(customer == NULL) {
            customer = first;
            pthread_mutex_unlock(register_mutex);
            continue;
        }
        // Quand on a trouver un client on appel le banquier
        call_bankers(customer);
        // Et on passe au prochain
        customer = customer->next;
        // Le client est alors retiré de la file
        pthread_mutex_unlock(register_mutex);
    }
    pthread_exit(0);
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
    // Verifie d'abord si on demande trop de ressources
    if(customer->head->max_resources[0] > conf->file_system_cap || customer->head->max_resources[0] < 0) {
        return ERROR;
    }
    if(customer->head->max_resources[1] > conf->network_cap || customer->head->max_resources[1] < 0) {
        print("oh oh !\n");
        return ERROR;
    }
    if(customer->head->max_resources[2] > conf->system_cap || customer->head->max_resources[2] < 0) {
        return ERROR;
    }
    int j = 3;
    for(int i = 0; i < conf->command_count; i++) {
        if(customer->head->max_resources[j] > conf->command_caps[i] || customer->head->max_resources[j++] < 0) {
            return ERROR;
        }
    }
    if (customer->head->max_resources[j] > conf->any_cap || customer->head->max_resources[j] < 0) {
        return ERROR;
    }
    // Attend que ce soit son tour
    pthread_mutex_lock(register_mutex);

    // demande des ressources
    customer->depth = cmd_depth;

    pthread_mutex_unlock(register_mutex);
    return NO_ERROR;
}

/**
 * Fonction utilisee pour liberer les customers
 * @param customer : la variable globale du premier client (first)
 * @return un code d'erreur.
 * */
error_code freeCustomers(banker_customer *customer) {
    banker_customer *current = customer;
    banker_customer *next;
    while(current != NULL) {
        next = current->next;
        free(current->current_resources);
        free(current);
        current = next;
    }
    return NO_ERROR;
}

/**
 * Fonction qui libere les ressource utilisees par une ligne de commande
 * @param cmd_depth : la profondeur de la ligne de commande
 * @param customer : le dernier client de la ligne de commande
 * @return : un code d'erreur
 * */
error_code freeRessources(banker_customer *customer, int  cmd_depth) {
    pthread_mutex_lock(available_mutex);
    //pthread_mutex_lock(register_mutex);
    for(int i = 0; i <= cmd_depth; i++) {
        for(int j = 0; j < conf->ressources_count; j++) {
            _available[j] += customer->current_resources[j];
        }
        customer = customer->prev;
    }
    pthread_mutex_unlock(available_mutex);
    //pthread_mutex_unlock(register_mutex);
    return NO_ERROR;
}

/**
 * Utilisez cette fonction pour initialiser votre shell
 * Cette fonction est appelée uniquement au début de l'exécution
 * des tests (et de votre programme).
 */
error_code init_shell() {
    char *line;
    register_mutex = malloc(sizeof(pthread_mutex_t));
    if(register_mutex == NULL) return ERROR;

    available_mutex = malloc(sizeof(pthread_mutex_t));
    if(available_mutex == NULL) {
        free(register_mutex);
        return ERROR;
    }

    pthread_mutex_init(register_mutex, NULL);
    pthread_mutex_init(available_mutex, NULL);

    //initialisation et configuration
    while(1) {
        if (HAS_ERROR(readLine(&line))) goto error;
        if (strlen(line) == 0) continue;
        if(HAS_ERROR(parse_first_line(line))) goto error;
        free(line);
        break;
    }
    _available = (int *)malloc(sizeof(int) * conf->ressources_count);
    if(_available == NULL) goto error;
    int j = 0;
    _available[j++] = conf->file_system_cap;
    _available[j++] = conf->network_cap;
    _available[j++] = conf->system_cap;
    for(int i = 0; i < conf->command_count; i++) {
        _available[j++] = conf->command_caps[i];
    }
    _available[j] = conf->any_cap;
    pthread_create(&banker_tid, NULL, banker_thread_run, NULL);
    return NO_ERROR;

    error:
    free(register_mutex);
    free(available_mutex);
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
    stop_banker = 1;
    pthread_join(banker_tid, 0);
    free(_available);
    free(register_mutex);
    free(available_mutex);
    freeConfiguration(conf);
    freeCustomers(first);
}

int callCommand(command *current) {
    if (current->count <= 0) return 1;
    if (strcmp(current->call[0], "exit") == 0) return 7;

    int exitCode = -1;     // the exit code of the child
    pid_t pid = 0;
    for (int i = 0; i < current->count; i++) {
        pid = fork();
        if (pid < 0) {        // forking failed
            printf("Fork failed\n");
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


error_code callCommands(banker_customer *customer, command *current, int cmd_depth) {
    if (current == NULL || current->call == NULL) return 0;
    pthread_mutex_unlock(customer->head->mutex);
    if(HAS_ERROR(request_resource(customer, cmd_depth))) return ERROR; // Pas d'execution
    pthread_mutex_lock(customer->head->mutex);
    pthread_mutex_lock(customer->head->mutex);

    int ret = callCommand(current);
    if (ret == 7) {
        freeRessources(customer, cmd_depth);
        return 7;
    }
    if (ret == -1) {
        freeRessources(customer, cmd_depth);
        return ERROR;
    }

    operator op = current->op;
    command *next = current->next;

    switch (op) {
        default:
        case NONE:
        case BIDON:
            freeRessources(customer, cmd_depth);
            return 0;
        case AND:
            if (ret) return callCommands(customer->next, next, ++cmd_depth);
            return 0;
        case OR:
            if (ret) {
                next = next->next;
                while (next != NULL && (next->op == OR || next->op == NONE)) next = next->next;
                if (next != NULL && next->op == AND) next = next->next;
            }
            return callCommands(customer->next, next, ++cmd_depth);
    }
}

void *runner(void *arg) {
    command_head *h;
    h = (command_head *)arg;
    pthread_mutex_lock(h->mutex); // Auto-block
    pthread_mutex_lock(register_mutex);

    /**** Section critique ****/
    // Enregistrement de la commande
    banker_customer *customer;
    if(first == NULL) {
        customer = register_command(h);
        first = customer;
    } else {
        banker_customer *current = first;
        while(current->next != NULL) {
            current = current->next;
        } // va s'enregistrer a la fin
        customer = register_command(h);
        current->next = customer;
    }
    pthread_mutex_unlock(register_mutex);
    /**** Fin section critique ****/

    int cmd_depth = 0;
    callCommands(customer, h->command, cmd_depth);
    pthread_mutex_unlock(h->mutex);

    free(h->max_resources);
    free(h->mutex);
    freeCommands(h->command);
    free(h);
    pthread_exit(0);
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
    pthread_t tid;

    while (1) {
        if (HAS_ERROR(readLine(&line))) goto bot;
        if (strlen(line) == 0) continue;

        if (HAS_ERROR(create_command_chain(line, &head))) goto bot;
        evaluate_whole_chain(head);
        free(line);
        f = head->command;
        if (head->background) {
            command_head *arg = head;
            pthread_create(&tid, NULL, runner, (void *)arg);
        } else {
            pthread_mutex_lock(head->mutex);
            pthread_mutex_lock(register_mutex);

            /**** Section critique ****/
            // Enregistrement de la commande
            banker_customer *customer;
            if(first == NULL) {
                customer = register_command(head);
                first = customer;
            } else {
                banker_customer *current = first;
                while(current->next != NULL) {
                    current = current->next;
                } // va s'enregistrer a la fin
                customer = register_command(head);
                current->next = customer;
            }
            pthread_mutex_unlock(register_mutex);
            /**** Fin Section critique ****/

            int depth = 0;
            exit_code = callCommands(customer, f, depth);
            pthread_mutex_unlock(head->mutex);
            free(head->max_resources);
            free(head->mutex);
            freeCommands(f);
            free(head);
        }

        if(exit_code == 7) {
            return;
        }
    }
    bot:
    free(line);
    toptop:
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
    if (HAS_NO_ERROR(init_shell())) {
        run_shell();
        close_shell();
    } else {
        printf("Error while executing the shell.");
    }
}

