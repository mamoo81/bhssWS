#include "productController.h"
#include "pqxx/pqxx"
#include "jsoncpp/json/json.h"
#include <string>
#include <fmt/format.h> // string ifadelere argüman eklemek için. c++20 ile geldi bu özellik ama bunda çalıştıramadım. o yüzden fmt kütüphanesini kullandım. örnek; fmt::format("Merhaba {0}.", "Mehmet")

void productController::sayHello(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback)
{
    auto resp = HttpResponse::newHttpResponse();

    resp->setBody("Merhaba, Drogon çalışıyor...");

    callback(resp);
}

void productController::getProduct(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback, std::string pBarcode)
{
    pqxx::connection conn("hostaddr=127.0.0.1 port=5432 dbname=bhssdb user=postgres password=postgres");
    pqxx::work work(conn);
    pqxx::result result = work.exec_params("select id, barcode, name, unit, category, subcategory, price, kdv, otv, date, lastdate, producer from productcards where barcode = $1", pBarcode);

    Json::Value json;

    if(result.size() <= 0){
        
        Json::Value jVal;
        jVal["getProduct"] = "null";
        json = jVal;
    }
    else{
        for (auto &&row : result)
        {   
            Json::Value jProduct;
            jProduct["id"] = row[0].as<int>();
            jProduct["barcode"] = row[1].as<string>();
            jProduct["name"] = row[2].as<string>();
            jProduct["unit"] = row[3].as<int>();
            jProduct["category"] = row[4].as<int>();
            jProduct["subcategory"] = row[5].as<int>();
            jProduct["price"] = row[6].as<double>();
            jProduct["kdv"] = row[7].as<int>();
            jProduct["otv"] = row[8].as<int>();
            jProduct["date"] = pqxx::to_string(row[9]);
            jProduct["lastdate"] = pqxx::to_string(row[10]);
            jProduct["producer"] = row[11].as<int>();

            Json::Value jVal;
            jVal["getProduct"] = jProduct;
            json = jVal;
        }
    }
    auto resp = HttpResponse::newHttpJsonResponse(json);
    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);// zorunlu değil
    callback(resp);
}

void productController::newProduct(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback)
{
    auto jsonReq = req->getJsonObject();

    Json::Value jProduct = (*jsonReq)["newProduct"];
    Json::Value jResp;

    pqxx::connection conn("hostaddr=127.0.0.1 port=5432 dbname=bhssdb user=postgres password=postgres");
    pqxx::work work(conn);

    //barkod varmı kontrol
    pqxx::result result = work.exec_params("select * from productcards where barcode = $1", jProduct["barcode"].asString());

    if(result.size() > 0)
    {
        jResp["newProduct"] = "This barcode is assigned to another product";
    }
    else{
        try
        {
            work.exec_params("insert into productcards(barcode, name, unit, category, subcategory, price, kdv, otv, producer) "
                            "values($1, $2, $3, $4, $5, $6, $7, $8, $9)", jProduct["barcode"].asString(), jProduct["name"].asString(), jProduct["unit"].asInt(), jProduct["category"].asInt(), jProduct["subcategory"].asInt(), jProduct["price"].asDouble(), jProduct["kdv"].asInt(), jProduct["otv"].asInt(), jProduct["producer"].asInt());
            work.commit();

            jResp["newProduct"] = "ok";
        }
        catch(const pqxx::sql_error e)
        {
            jResp["newProduct"] = "error";
            jResp["Error Message"] = e.what();
            std::cerr << "SQL HATASI: " << e.what() << '\n';
        }
    }

    auto resp = HttpResponse::newHttpJsonResponse(jResp);
    resp->setContentTypeCode(CT_APPLICATION_JSON);
    callback(resp);
}
