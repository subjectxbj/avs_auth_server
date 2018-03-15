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
#include "lwa.h"
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
char * productId = "GC4026E";
char * deviceSerialNumber = "CXNK00012345";
char * clientId = "amzn1.application-oa2-client.b91a4d2fd2f641f2a15ea469";
char * clientSecret = "70c955eaa6b998bb3eff97f051b18a5f72b21d38e3ba102da1e83aafdd0efa1f";
char * scope = "alexa:all";
char * responseType = "code";
char * redirectUri="http://localhost:3000/authresponse";

char*  getRedirectUrl(){
        static char lwaUrl[1000];
        char *scopeData;
        cJSON *alexa_all, *attr;
        cJSON *root = cJSON_CreateObject();
        cJSON_AddItemToObject(root, "alexa:all", alexa_all=cJSON_CreateObject());
        cJSON_AddStringToObject(alexa_all, "productID", productId);
        cJSON_AddItemToObject(alexa_all, "productInstanceAttributes", attr=cJSON_CreateObject());
        cJSON_AddStringToObject(attr, "deviceSerialNumber", deviceSerialNumber);
        scopeData = cJSON_Print(root);
        printf("%s", scopeData);

        memset(lwaUrl, 0, sizeof(lwaUrl));
        strcat(lwaUrl, "https://www.amazon.com/ap/oa/?");

        char param[512];
        sprintf(param, "client_id=%s", clientId);
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
        sprintf(param, "redirect_uri=%s", urlencode(redirectUri));
        strcat(lwaUrl, param);

        printf("%s",lwaUrl);
        return lwaUrl;
      

}

int requestRefreshToken(char *code) {
	CURL *curl;
	CURLcode res;
	struct curl_resp_content resp;
	
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	if (curl) {
		struct curl_slist *headers = NULL;
		char buf[1024]={0};
		char params[1024]={0};

		snprintf(buf, sizeof(buf), "Content-Type: application/x-www-form-urlencoded");
		headers = curl_slist_append(headers, buf);
		
		curl_easy_setopt(curl, CURLOPT_URL, "https://api.amazon.com/auth/o2/token");

		sprintf(params, "grant_type=%s&code=%s&client_id=%s&client_secret=%s%redirect_uri=%s",
			"authorization_code",
			code,
			clientId,
			clientSecret,
			redirectUri
			)
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);	
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, params);
		curl_easy_setopt(curl, CURLOPT_HTTPPOST, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&resp);
		
		res = curl_easy_perform(curl);	
		if (res != CURLE_OK) {
			printf("ERROR: Failed to get refresh token");
		}
		else{
			if (resp.size!=0) {
				printf("Response: %s", resp.payload);		
			}
		}
		curl_easy_cleanup(curl);	
	}	
	curl_easy_cleanup();
	return 0;
}

int handleUserRequest(int client) {
	printf("\nReceive User Request");
	char *redirectUrl = getRedirectUrl();
	printf("\nRedirect usr to URL: %s", redirectUrl);
	char content[1000];
	sprintf(content, "Location: %s\n",redirectUrl);
	write(client, "HTTP/1.0 307\n",13);
	write(client, content, strlen(content)+1);
	write(client, "\n", 1);
	return 0;
}
int handleAuthCodeGrant(int client, char *request) {
	char *code;
	printf("\nReceive authresponse");
	printf("\nRequest: %s", request);
	code = strtok(request, "code=");
	strtok(NULL, "&");
	printf("\nCode: %s", code);
        	
}
