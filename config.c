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
#include "cJSON.h"
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

cJSON* getCleanConfig(char *filename) {
	FILE *fd;
	char buffer[102400]={0};
	char *pline = NULL;
	ssize_t read;
	//printf("\n==============>Parse Config: %s\n", filename);
	fd = fopen(filename, "r");

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

	cJSON *json = NULL;
	json = cJSON_Parse(buffer);
	if (json) {
		//printf("JSON parse OK.\n");
		//printf(cJSON_Print(json));
		//printf("\n");
	}else{
		printf("Failed to parse response\n");
	}
	return json;

}

int writeConfig(cJSON *json, char *filename) {
	FILE *fd;
	fd = fopen(filename, "w+");
	if (fd) {
		char *output;
		output = cJSON_Print(json);
		if (output) {
			size_t len = strlen(output);
			fwrite(output, len, 1, fd);
		}
	}
	fclose(fd);
	return 0;
}


cJSON *get_config_param(cJSON *config, char *name){
	cJSON *authDelegate = cJSON_GetObjectItem(config, "authDelegate");
	if(authDelegate == NULL) {
		return NULL;
	}
	
	if (strcmp(name, "clientSecret") == 0) {
		cJSON *clientSecret = cJSON_GetObjectItem(authDelegate, "clientSecret");
		return clientSecret;
	}else if (strcmp(name, "deviceSerialNumber") == 0) {
		cJSON *deviceSerialNumber = cJSON_GetObjectItem(authDelegate, "deviceSerialNumber");
		return deviceSerialNumber;
	}else if (strcmp(name, "refreshToken") == 0) {
		cJSON *refreshToken = cJSON_GetObjectItem(authDelegate, "refreshToken");
		return refreshToken;
	}else if (strcmp(name, "clientId") == 0) {
		cJSON *clientId = cJSON_GetObjectItem(authDelegate, "clientId");
		return clientId;
	}else if (strcmp(name, "productId") == 0) {
		cJSON *productId = cJSON_GetObjectItem(authDelegate, "productId");
		return productId;
	}
	return NULL;
}
char *get_config_param_value(cJSON *config, char *name){
	cJSON *param;
	param = get_config_param(config, name);
	return param? param->valuestring: NULL;
}

int update_config_param(cJSON *config, char *name, char *value) {
	char *new_string = malloc(strlen(value)+1);
	
	strcpy(new_string, value);
	cJSON *param;
	param = get_config_param(config, name);
	if (param) {
		free(param->valuestring);
		param->valuestring = new_string;
		return 0;
	}else{
		return 1;
	}
	
}

