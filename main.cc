#include <drogon/drogon.h>
#include <iostream>
#include <locale.h>

using namespace std;
using namespace drogon;

int main() {

    setlocale(LC_ALL, "Turkish");

    const char *homeDir = getenv("HOME");
    std::string uploadDir = homeDir + string("/product-images");

    cout << "Drogon 0.0.0.0:8189 'da çalışıyor..." << endl;

    //Set HTTP listener address and port
    // drogon::app()
    //     .setUploadPath(string(uploadDir)).setClientMaxBodySize(8*1024*1024).setClientMaxMemoryBodySize(1024*1024)
    //     .enableSession()
    //     .setThreadNum(4)
    //     // .enableRunAsDaemon()
    //     .setFileTypes({"json", "png"})
    //     .addListener("0.0.0.0", 8189)
    //     .run();
    //Load config file
    drogon::app()
        .loadConfigFile("../config.json")
        .run();
    //drogon::app().loadConfigFile("../config.yaml");
    //Run HTTP framework,the method will block in the internal event loop
    return EXIT_SUCCESS;
}
