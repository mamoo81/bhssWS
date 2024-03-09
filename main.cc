#include <drogon/drogon.h>
#include <iostream>
#include <locale.h>

using namespace std;
using namespace drogon;

int main() {

    setlocale(LC_ALL, "Turkish");

    const char *homeDir = getenv("HOME");
    std::string uploadDir = homeDir + string("/product-images");

    //Set HTTP listener address and port
    drogon::app()
        .setUploadPath(string(uploadDir))
        .setThreadNum(4)
        // .enableRunAsDaemon()
        .setFileTypes({"json", "png"})
        .addListener("0.0.0.0", 8189)
        .run();
    //Load config file
    //drogon::app().loadConfigFile("../config.json");
    //drogon::app().loadConfigFile("../config.yaml");
    //Run HTTP framework,the method will block in the internal event loop
    return 0;
}
