#pragma once

#include <iostream>
#include <drogon/HttpController.h>
#include <chrono>

using namespace drogon;
using namespace std;

class productController : public drogon::HttpController<productController>
{
  public:
    METHOD_LIST_BEGIN
    // use METHOD_ADD to add your custom processing function here;
    // METHOD_ADD(productController::get, "/{2}/{1}", Get); // path is /productController/{arg2}/{arg1}
    // METHOD_ADD(productController::your_method_name, "/{1}/{2}/list", Get); // path is /productController/{arg1}/{arg2}/list
    // ADD_METHOD_TO(productController::your_method_name, "/absolute/path/{1}/{2}/list", Get); // path is /absolute/path/{arg1}/{arg2}/list
    ADD_METHOD_TO(productController::sayHello, "/", Get);
    ADD_METHOD_TO(productController::getProduct, "/getProduct/{barcode}", Get);
    ADD_METHOD_TO(productController::newProduct, "/newProduct", Post);

    METHOD_LIST_END
    // your declaration of processing function maybe like this:
    // void get(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int p1, std::string p2);
    // void your_method_name(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, double p1, int p2) const;
    void sayHello(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback);
    void getProduct(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback, std::string pBarcode);
    void newProduct(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback);

};
