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
