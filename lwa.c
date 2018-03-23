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
#include "lwa.h"
#include "curl.h"
#include "config.h"

#ifdef HARDCODE_CONFIG
char * productId = "GC4026E";
char * deviceSerialNumber = "CXNK000ABCDE";
char * clientId = "amzn1.application-oa2-client.a308f3dc39a54349b970f19b13c86f5d";
char * clientSecret = "70c955eaa6b998bb3eff97f051b18a5f72b21d38e3ba102da1e83aafdd0efa1f";
#endif
char * scope = "alexa:all";
char * responseType = "code";
char * urlencode(char *json_string)
{
        static char output[1024];
        memset(output, 0, sizeof(output));
        char *p_in = json_string;
        char *p_out=&output[0];
        while(*p_in!='\0') {
                char c = *p_in;
                if(c == '\n' || c == '\t') {
                }
                else if(!(c >= 'A' && c <='Z' || c >= 'a' && c <= 'z')){
                        sprintf(p_out, "%%%02X", c);
                        p_out+=3;
                }
                else{
                        *p_out++ = *p_in;
                }
                p_in++;
        }
        return output;
}


char*  getRedirectUrl(){
        static char lwaUrl[1000];
        char *scopeData;
        json_object *alexa_all, *attr;
        json_object *root = json_object_new_object();
        json_object_object_add(root, "alexa:all", alexa_all=json_object_new_object());
#ifdef HARDCODE_CONFIG
        json_object_object_add(alexa_all, "productID", json_object_new_string(productId));
#else
        json_object_object_add(alexa_all, "productID",json_object_new_string(get_config_param_value(json_config, "productId")));
#endif
        json_object_object_add(alexa_all, "productInstanceAttributes", attr=json_object_new_object());
#ifdef HARDCODE_CONFIG
        json_object_object_add(attr, "deviceSerialNumber", json_object_new_string(deviceSerialNumber));
#else
        json_object_object_add(attr, "deviceSerialNumber", json_object_new_string(get_config_param_value(json_config, "deviceSerialNumber")));
#endif
        scopeData = json_object_to_json_string(root);
        json_object_put(root);
        //printf("%s", scopeData);

        memset(lwaUrl, 0, sizeof(lwaUrl));
        strcat(lwaUrl, "https://www.amazon.com/ap/oa/?");

        char param[512];
#ifdef HARDCODE_CONFIG
        sprintf(param, "client_id=%s", clientId);
#else
        sprintf(param, "client_id=%s", get_config_param_value(json_config, "clientId"));
#endif
        printf("param:%s\n",param);
        strcat(lwaUrl, param);
        strcat(lwaUrl, "&");

        memset(param, 0, sizeof(param));
        sprintf(param, "scope=%s", urlencode(scope));
        strcat(lwaUrl, param);
        strcat(lwaUrl, "&");

        memset(param, 0, sizeof(param));
        sprintf(param, "scope_data=%s", urlencode(scopeData));
        strcat(lwaUrl, param);
        strcat(lwaUrl, "&");

        memset(param, 0, sizeof(param));
        sprintf(param, "response_type=%s", responseType);
        strcat(lwaUrl, param);
        strcat(lwaUrl, "&");

        memset(param, 0, sizeof(param));
        sprintf(param, "redirect_uri=%s", urlencode(redirect_uri));
        strcat(lwaUrl, param);

        //printf("%s",lwaUrl);
        return lwaUrl;
      

}
int parseResponse(char *response) {
		json_object *json = NULL;
		int ret;
		json = json_tokener_parse(response);
		if (json) {
			json_object *refreshTokenObject = json_object_object_get(json, "refresh_token");
			if(refreshTokenObject) {
				char *refreshToken = json_object_get_string(refreshTokenObject);
				if(refreshToken) {
					printf("\n==============>Get Refresh Token: [%s]\n", refreshToken);
					ret = update_config_param(json_config, "refreshToken", refreshToken);
					if (ret != 0){
						printf("\nERROR: Failed to update refresh token\n");
						json_object_put(json);
						return -1;
					}
					ret = writeConfig(json_config, config_out_path);
					if (ret != 0){
						printf("\nERROR: Failed to write config to [%s]\n", config_out_path);
						json_object_put(json);
						return -1;
					}
				}
			}else{
				printf("\nERROR: Failed to get refresh_token from response\n");
				json_object_put(json);
				return -1;
			}
			json_object_put(json);
			return 0;
		}else{
			printf("\nERROR: Failed to parse response\n");
			return -1;
		}	
}
char resp[10240]={0};
size_t write_data(void* buffer, size_t size, size_t nmemb, void* response) {
	int len = size*nmemb;
	memset(resp, 0, sizeof(resp));
	memcpy(response, buffer, len);
	return len;
}

int requestRefreshToken(char *code) {
	CURL *curl;
	CURLcode res;
	int ret;

	printf("\n==============>Request Refresh token\n");
	curl_global_init(CURL_GLOBAL_SSL);
	curl = curl_easy_init();
	if (curl) {
		struct curl_slist *headers = NULL;
		char buf[1024]={0};
		char params[1024]={0};

		snprintf(buf, sizeof(buf), "Content-Type: application/x-www-form-urlencoded");
		headers = curl_slist_append(headers, buf);
		
		

		sprintf(params, "grant_type=%s&code=%s&client_id=%s&client_secret=%s&redirect_uri=%s",
			"authorization_code",
			code,
#ifdef HARDCODE_CONFIG
			clientId,
#else
			get_config_param_value(json_config, "clientId"),
#endif
#ifdef HARDCODE_CONFIG
			clientSecret,
#else
			get_config_param_value(json_config, "clientSecret"),
#endif
			urlencode(redirect_uri)
			);
		//printf("Params: %s\n",params);
		
		curl_easy_setopt(curl, CURLOPT_URL, "https://api.amazon.com/auth/o2/token");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);	
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, params);
		curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTPS);
		curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_TRY);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
		//curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2);
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)resp);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		
		
		res = curl_easy_perform(curl);	
		if (res != CURLE_OK) {
			printf("ERROR: curl_easy_perform failed, res=%d\n", res);
			curl_easy_cleanup(curl);
			curl_global_cleanup();
			return -1;
		}
		else{
			printf("OK\n");
		}
		printf("\n==============>Get RESPONSE: %s\n", resp);
		ret = parseResponse(resp);
		if (ret != 0){
			printf("ERROR: parseResponse failed\n");
			curl_easy_cleanup(curl);
			curl_global_cleanup();
			return -1;
		}
		
		curl_easy_cleanup(curl);
		curl_global_cleanup();
		if (res == CURLE_OK)
			return 0;
		else
			return -1;	
	}else{
		printf("ERROR: curl_easy_init failed\n");
		curl_global_cleanup();
		return -1;
	}	
}

int handleUserRequest(int client) {
	printf("\n==============>Receive User Request");
	char *redirectUrl = getRedirectUrl();
	if (redirectUrl == NULL) {
		return -1;
	}
	printf("\n==============>Redirect usr to URL: [%s]", redirectUrl);
	char content[1000];
	sprintf(content, "Location: %s\n",redirectUrl);
	write(client, "HTTP/1.0 307\n",13);
	write(client, content, strlen(content)+1);
	write(client, "\n", 1);
	return 0;
}
int returnResult(int client, int status, char *info) {
	char content[1024]={0};
	write(client, "HTTP/1.0 200\n",13);
	write(client, "\n", 1);
	if (status == 0){
		sprintf(content, "<html><head><title>AVS LWA Success</title></head><body>Success</body></html>\n");
	}else{
		sprintf(content, "<html><head><title>AVS LWA Failure</title></head><body>Failed.%s</body></html>\n", info);
	}
	write(client, content, strlen(content));
}
int handleAuthCodeGrant(int client, char *request) {
	char *tag, *code; 
	int ret;
	printf("\n==============>Receive authresponse");
	//printf("\nRequest: %s", request);
	tag = strtok(request, "=");
	code = strtok(NULL, "&");
	if(code)
		printf("\n==============>Get Auth Code: [%s]", code);
	else{
		printf("\nERROR: Failed to extract auth code\n");
		returnResult(client, -1, "Failed to extract auth code");
		return -1;
	}
	ret = requestRefreshToken(code);
	if (ret != 0){
		printf("\nERROR: Failed to get refresh token\n");
		returnResult(client, -1, "Failed to get refresh token");
		return -1;
	}
	returnResult(client, 0, NULL);
	printf("\n==============>Finish\n");
   
	return 0;
        	
}
