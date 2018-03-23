json_object* getCleanConfig(char *filename);
int writeConfig(json_object *json, char *filename);
int update_config_param(json_object *config, char *name, char *value) ;
char *get_config_param_value(json_object *config, char *name);
json_object *get_config_param(json_object *config, char *name);


extern json_object *json_config;
