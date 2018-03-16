extern char config_out_path[256];
char redirect_uri[256];
int handleUserRequest(int client);
int handleAuthCodeGrant(int client, char *request);
