#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

#define true 1
#define false 0
#define bool int

/**
 * fonction qui li le stdin et qui en fait une chaine de caracteres
 * @return une chaine de caractere representant une ligne entree dans le terminal.
 */
char* readLine(void) {
    char *line;
    size_t max_len = 1024;
    char car;
    int position = 0;
    // On alloue mex_len+1 pour avoir de la place pour la chaine '\0'
    // Qui est la terminaison standard et necessaire des chaines en C.
    line = (char *)malloc(max_len);
    if (line == NULL) {
        printf("Memory allocation error in readline\n");
        return NULL;
    }
    car = (char) fgetc(stdin);
    while((car != '\n') && (car != EOF)) {
        line[position++] = car;
        car = (char) fgetc(stdin);
        if(position >= max_len) {
            printf("line longer than max_len\n");
            free(line);
            return NULL;
        }
    }
    line[position] = '\0';
    line = realloc(line, position+1);
    return line;
}

/**
 * Fonction utilitaire qui libere l'espace memoire occupe par un tableau de pointeurs de caracteres
 * @param args : un tableau de pointeurs vers de pointeurs de caracteres, dont le dernier indice est occupe par NULL
 * @return un entier representant un code d'erreur
 **/
int freeArgs(char **args) {
    if (args == NULL) {
        return 0;
    }
    int i = 0;
    while(args[i] != NULL) {
        free(args[i++]);
    }
    free(args[i]);
    free(args);
    return 0;
}
/**
 * Separe une instruction en une liste contenant les instructions et les arguments.
 * @param instruction : une chaine de caractere representant une instruction.
 * @return un tableau de chaine de caracteres.
 * */
char **parse_instruction(char *string) {
    char *delimiter;
    char last_char;
    char *copy;
    char *token;
    char **table;
    delimiter = " ()";
    int count = 0;
    // On compte d'abord combien d'elements nous avons dans l'instruction
    int i = 0;
    last_char = *delimiter;
    while (string[i] != '\0') {
        if ((string[i] != delimiter[0] && string[i] != delimiter[1] && string[i] != delimiter[2]) &&
            (last_char == delimiter[0] || last_char == delimiter[1] || last_char == delimiter[2])) {
            count++;
        }
        last_char = string[i];
        i++;
    }
    // On peut maintenant allouer de l'espace memoire pour le tableau
    table = (char **)malloc(sizeof(char *) * (count+1));
    if (table == NULL) {
        printf("Memory allocation error in parse_instruction\n");
        return NULL;
    }
    // Ensuite on peut effectuer la separation
    copy = strdup(string);
    if(copy == NULL) {
        printf("Memory allocation error in strdup\n");
        return NULL;
    }
    token = strtok(copy, delimiter);
    if (token == NULL) {
        free(copy);
        free(table);
        return NULL;
    }
    int j = 0;
   while(token != NULL) {
        table[j] = strdup(token);
        if(table[j] == NULL) {
           printf("memory allocation error in parse_instruction\n");
           free(token);
           free(copy);
           freeArgs(table);
        }
        j++;
        token = strtok(NULL, delimiter);
    }
    free(token);
    free(copy);
    table[count] = NULL;
    return table;
}
/**
 * Separe une ligne en suite d'instructions separees par des && et des ||
 * @param line : une chaine de caracteres representant une ligne entree par l'utilisateur.
 * @return un tableau de chaines de caracteres representant des instructions a separees.
 * */
char **parse_line(char *line, int **state) {
    char *ET = "&&";
    char *OU = "||";
    char sub_string[3];
    char **table;
    int debut;
    int no_of_instructions;
    int len;
    debut = 0;
    no_of_instructions = 1;
    len = (int) strlen(line);
    sub_string[2] = '\0';
    table = (char **)malloc(sizeof(char *) * no_of_instructions);
    if (table == NULL) {
        printf("Memory allocation error in parse_line\n");
        return NULL;
    }
    int i = 0;
    int j = 0;
    (*state)[j] = 1; // On execute toujours le premier statement.
    for(i = 0; i < len; i++) {
        sub_string[0] = line[i];
        sub_string[1] = line[i+1];
        if (strcmp(sub_string, ET) == 0) {
            no_of_instructions++;
            table[j] = strndup(line+debut, i-debut);
            if (table[j] == NULL) {
                printf("Memory allocation error in parse_line\n");
                freeArgs(table);
                return NULL;
            }
            j++;
            (*state)[j] = 1; // Indique la presence d'un ET
            table = realloc(table, sizeof(char *) * no_of_instructions);
            if (table == NULL) {
                printf("Memory reallocation error in parse_line\n");
                return NULL;
            }
            debut = i + 2;
        } else if (strcmp(sub_string, OU) == 0) {
            no_of_instructions++;
            table[j] = strndup(line+debut, i-debut);
            if (table[j] == NULL) {
                printf("Memory allocation error in parse_line\n");
                freeArgs(table);
                return NULL;
            }
            j++;
            (*state)[j] = 0; // Indique la presence d'un OU
            table = realloc(table, sizeof(char *) * no_of_instructions);
            if (table == NULL) {
                printf("Memory reallocation error in parse_line\n");
                return NULL;
            }
            debut = i + 2;
        }
    }
    // On ajoute la derniere instruction
    if (i != 0) {
        no_of_instructions++;
        table[j] = strndup(line + debut, i - debut);
        if (table[j] == NULL) {
            printf("Memory allocation error in parse_line\n");
            freeArgs(table);
            return NULL;
        }
        j++;
        table = realloc(table, sizeof(char *) * no_of_instructions);
        if (table == NULL) {
            printf("Memory reallocation error in parse_line\n");
            return NULL;
        }
    }
    // On rajoute la terminaison par null
    table[j] = NULL;
    return table;
}

bool is_in_background(char **line) {
  int len;
  if (line == NULL) {
    return false;
  }
  len = (int)strlen(*line);
  int i = len-1;
  while((*line)[i] == ' ' || (*line)[i] == '\n') {
    i--;
    if(i < 0) return false;
  }
    if ((*line)[i] == '&' && (*line)[i-1] == ' ') {
        (*line)[i] = '\0';
        return true;
    } else return false;
}

/**
 * Fonction executant la commande et les arguments passes en parametre. Elle execute un systemcall.
 * @param args : Un tableau ayant en premier indice la commande et la liste des arguments par la suite. Se termine par NULL.
 * @return un int : -1 pour une erreur, 0 pour indiquer l'arret du programme, 1 pour indiquer la continuation.
 */
void execute(char **args, int *status) {
    pid_t pid;
    int exit_status;
    int n;
    // Intercepter la commande exit
    if (strcmp(args[0], "exit") == 0) {
        *status = 0;
        return;
    } else if ((args[0][0] == 'r') && (args[0][1] >= '0' && args[0][1] <= '9')) {
        n = (int) strtol((*args)+1, NULL, 10); // Transforme string en int.
        int i;
        for(i = 0; i < n; i++) {
            pid = fork();
            if (pid < 0) {
                fprintf(stderr, "fork failed\n");
                *status = -1;
                exit(-1);
            } else if (pid == 0) { // child use to exec process
                if (execvp(args[1], args+1) == -1) {
                    printf("bash: %s: command not found\n", args[1]);
                    exit(EXIT_FAILURE);
                }
            } else { // Parent process
                wait(&exit_status);
                if(exit_status != 0) {
                    *status = -1;
                }
            }
        }
        return;
    } else if ((args[0][0] == 'f') && (args[0][1] >= '0' && args[0][1] <= '9')) {
        *status = 1;
        return;

    } else {
        pid = fork();
        if (pid < 0) {
            fprintf(stderr, "fork failed\n");
            *status = -1;
            exit(-1);
        } else if (pid == 0) { // child use to exec process
            if (execvp(args[0], args) == -1) {
                printf("bash: %s: command not found\n", args[0]);
                exit(EXIT_FAILURE);
            }
        } else { // Parent process
            wait(&exit_status);
            if(exit_status != 0) {
                *status = -1;
                return;
            }
        }
        *status = 1;
        return;
    }
}

/**
 * Fonction maire contenant la boucle d'execution principale du shell.
 * @return void.
 */
void shell() {
    char *line;
    int *and_stat;
    char **instructions;
    char **args;
    int status;
    bool background;
    pid_t pid = 1; // valeur placeholder
    status = 1;
    int i;
    // Boucle d'execution principale du shell
    while(status != 0) {
        waitpid(-1, NULL, WNOHANG); // Evalue si un enfant qui a termine sont execution doit etre rattrappe.
        status = 1; // Reset le status a chaque nouvelle ligne
        line = readLine();
        if(line == NULL) {
            continue;
        }
        background = is_in_background(&line);
        and_stat = (int *)malloc((strlen(line)/2)* sizeof(int)); // Un tableau de marquers int. Bien assez d'espace
        if(and_stat == NULL) {
            free(line);
            printf("Memory allocation error in shell.\n");
            return;
        }
        instructions = parse_line(line, &and_stat);
        if(instructions == NULL) {
            free(and_stat);
            printf("Memory allocation error in shell.\n");
            free(line);
            continue;
        }
        if (background) {
            pid =fork();
            if(pid < 0) {
                printf("An error occured during a fork. Execution terminated");
                free(line);
                free(and_stat);
                return;
            } else if (pid != 0) { // Si on est parent on poursuit l'execution de et on affiche la prochaine ligne
                free(line);
                free(and_stat);
                continue;
            }
        }
        i = 0;
        // Boucle d'execution d'une ligne
        while(instructions[i] != NULL) {
            // Si on touche un OU et que la commande precedente a fonctionnee on passe a la prochaine comande
            if (and_stat[i] == 0 && status == 1) {
                i++;
                continue;
            } if (and_stat[i] == 1 && status == -1) { // Si on voit un ET et la commande precedente n'a pas fonctioinne
                i++;
                continue;
            }
            args = parse_instruction(instructions[i++]);
            if (args == NULL){
                continue;
            }
            execute(args, &status);
            freeArgs(args);
            if(status == 0) break;
        }
        free(line);
        free(and_stat);
        freeArgs(instructions);
        // A la fin de l'execution de la commande, si on est dans un etat en background et que on est dans l'enfant, on arrete.
        if (pid == 0) {
           exit(0);
        }
    }
}
/*
 * Dont change main!
 *
 * You may add unit tests here, but we will rip out main entirely when we grade your work.
 */
int main (void) {
    shell();
   printf("Shell execution ended !!\n");
}
