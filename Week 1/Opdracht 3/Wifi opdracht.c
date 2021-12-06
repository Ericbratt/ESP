#include <WiFi.h>
#include "secret.h"
#include "time.h"
#define "secret.h

void WiFi_connect(){
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      printf(".");
  }
  printf(" CONNECTED\r\n");
}

void setup(){
  WiFi_connect();
}

void loop(){
}
