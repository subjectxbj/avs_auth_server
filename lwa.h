extern char config_path[256];
char redirect_uri[256];
int handleUserRequest(int client, json_object *config);
int handleAuthCodeGrant(int client, char *request, json_object *config);
