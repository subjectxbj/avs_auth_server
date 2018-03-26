#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<signal.h>
#include<fcntl.h>
#include "json.h"
#include "config.h"

int isCommentLine(char *line){
	char *pc = line;
	
	while(*pc != '\0'){
		if(*pc == '\t' || *pc == ' ') {
			pc++;
			continue;
		}else{
			if (*pc == '/') {
				if(*(pc+1) == '/') {
					return 1;
				} else{
					return 0;
				}
			}else{
				return 0;
			}
		}
		
	}
	return 0;
}

json_object* getCleanConfig(char *filename) {
	FILE *fd;
	char buffer[102400]={0};
	char *pline = NULL;
	ssize_t read;
	//printf("\n==============>Parse Config: %s\n", filename);
	fd = fopen(filename, "r");
	if(!fd) {
		printf("ERROR: Failed to open file %s\n", filename);
		return NULL;
	}

	while((read = getline(&pline, &read, fd)) != -1){
		//printf("%s\n", pline);
		if (isCommentLine(pline)){
			continue;
		}else{
			strcat(buffer, pline);
			strcat(buffer, "\n");
		}
	}
	free(pline);
	fclose(fd);
	//printf("\n==============>Clean Config: %s\n", buffer);

	json_object *json = NULL;
	json = json_tokener_parse(buffer);
	if (json) {
		//printf("JSON parse OK.\n");
		//printf(cJSON_Print(json));
		//printf("\n");
	}else{
		printf("Failed to parse config\n");
		return NULL;
	}
	return json;

}

int writeConfig(json_object *json, char *filename) {
	FILE *fd;
	fd = fopen(filename, "w+");
	if (fd) {
		char *output;
		output = json_object_to_json_string(json);
		if (output) {
			size_t len = strlen(output);
			fwrite(output, len, 1, fd);
		}
	}else{
		printf("ERROR: Failed to open file %s\n", filename);
		return -1;
	}
	fclose(fd);
	return 0;
}


json_object *get_config_param(json_object *config, char *name){
	json_object *authDelegate = json_object_object_get(config, "authDelegate");
	if(authDelegate == NULL) {
		return NULL;
	}
	
	if (strcmp(name, "clientSecret") == 0) {
		json_object *clientSecret = json_object_object_get(authDelegate, "clientSecret");
		return clientSecret;
	}else if (strcmp(name, "deviceSerialNumber") == 0) {
		json_object *deviceSerialNumber = json_object_object_get(authDelegate, "deviceSerialNumber");
		return deviceSerialNumber;
	}else if (strcmp(name, "refreshToken") == 0) {
		json_object *refreshToken = json_object_object_get(authDelegate, "refreshToken");
		return refreshToken;
	}else if (strcmp(name, "clientId") == 0) {
		json_object *clientId = json_object_object_get(authDelegate, "clientId");
		return clientId;
	}else if (strcmp(name, "productId") == 0) {
		json_object *productId = json_object_object_get(authDelegate, "productId");
		return productId;
	}
	return NULL;
}
char *get_config_param_value(json_object *config, char *name){
	json_object *param;
	param = get_config_param(config, name);
	return param? json_object_get_string(param): NULL;
}

int update_config_param(json_object *config, char *name, char *value) {
	json_object *authDelegate = json_object_object_get(config, "authDelegate");
	if(authDelegate == NULL) {
		return -1;
	}
	if (strcmp(name, "refreshToken") == 0){
        json_object_object_del(authDelegate, "refreshToken");

        json_object_object_add(authDelegate, "refreshToken", json_object_new_string(value));
        
        return 0;
	}
	
	return -1;
	
}


