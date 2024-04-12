#pragma once

#include <iostream>
#include <drogon/HttpController.h>
#include <chrono>
#include <string>

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
    ADD_METHOD_TO(productController::getProduct, "/bhss/get/product/{barcode}", Get);
    ADD_METHOD_TO(productController::addProduct, "/bhss/add/product", Post);
    ADD_METHOD_TO(productController::deleteProducts, "/bhss/delete/product", Post);
    ADD_METHOD_TO(productController::addSubCategory, "/bhss/add/subcategory", Post);
    ADD_METHOD_TO(productController::addCategory, "/bhss/add/category", Post);
    ADD_METHOD_TO(productController::getCategoriesAndSubCategories, "/bhss/get/categoriesandsubcategories", Get);
    ADD_METHOD_TO(productController::getCategories, "/bhss/get/categories", Get);
    ADD_METHOD_TO(productController::getSubCategories, "/bhss/get/subcategories", Get);
    ADD_METHOD_TO(productController::getUnits, "/bhss/get/units", Get);
    ADD_METHOD_TO(productController::addProducer, "/bhss/add/producer/{producer}", Get);
    ADD_METHOD_TO(productController::getProducers, "/bhss/get/producers", Get);

    METHOD_LIST_END
    // your declaration of processing function maybe like this:
    // void get(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int p1, std::string p2);
    // void your_method_name(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, double p1, int p2) const;
    void sayHello(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback);
    void getProduct(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback, string pBarcode);
    void addProduct(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback);
    void deleteProducts(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback);
    void addSubCategory(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback);
    void addCategory(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback);
    void getCategoriesAndSubCategories(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback);
    void getCategories(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback);
    void getSubCategories(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback);
    void getUnits(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback);
    void getProducers(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback);
    void addProducer(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback, string pProducer);
};
