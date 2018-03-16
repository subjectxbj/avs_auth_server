cJSON* getCleanConfig(char *filename);
int writeConfig(cJSON *json, char *filename);
int update_config_param(cJSON *config, char *name, char *value) ;
char *get_config_param_value(cJSON *config, char *name);
cJSON *get_config_param(cJSON *config, char *name);


extern cJSON *json_config;
