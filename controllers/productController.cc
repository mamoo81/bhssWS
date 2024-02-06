#include "productController.h"
#include "pqxx/pqxx"
#include "jsoncpp/json/json.h"
#include <string>
#include <fmt/format.h> // string ifadelere argüman eklemek için. c++20 ile geldi bu özellik ama bunda çalıştıramadım. o yüzden fmt kütüphanesini kullandım. örnek; fmt::format("Merhaba {0}.", "Mehmet")
#include <vector>

pqxx::connection sqlConn("hostaddr=127.0.0.1 port=5432 dbname=bhssdb user=postgres password=postgres");
pqxx::result sqlResult;
pqxx::work work(sqlConn);

void productController::sayHello(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback)
{
    LOG_INFO << "Talep gelen ip adresi: " << req->peerAddr().toIp();
    auto resp = HttpResponse::newHttpResponse();
    resp->setBody("Merhaba, Drogon çalışıyor...");
    callback(resp);
}

void productController::getProduct(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback, std::string pBarcode)
{
    sqlResult = work.exec_params("select id, barcode, name, unit, category, subcategory, price, kdv, otv, date, lastdate, producer from productcards where barcode = $1", pBarcode);

    Json::Value json;

    if(sqlResult.size() <= 0){
        
        Json::Value jVal;
        jVal["getProduct"] = "null";
        json = jVal;
    }
    else{
        for (auto &&row : sqlResult)
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

    LOG_INFO << "Talep gelen ip adresi: " << req->peerAddr().toIp();// zorunlu değil
    auto resp = HttpResponse::newHttpJsonResponse(json);
    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);// zorunlu değil
    callback(resp);
}

/*
    dönüş: addProcuts = 
        1 ise tüm kayıtlar başarılı
        2 ise hata veya hatalar var   
*/
void productController::addProduct(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback)
{
    auto jsonReq = req->getJsonObject();

    Json::Value jProducts = (*jsonReq)["addProducts"];
    Json::Value jResp; 
    Json::Value jBarcodes = Json::Value(Json::arrayValue);
    
    jResp["addProducts"] = 1; // ürün ekleme başarılı ise cevap = 1 başarılı.

    for(const auto &element : jProducts){
        
        sqlResult = work.exec_params("select * from productcards where barcode = $1", element["barcode"].asString());
        if(sqlResult.size() > 0){
            // barkod var ise kodlanacak...
            jResp["addProducts"] = 2;
            // var olan ürünün barkodunu cevaba false olarak kayıt etme
            Json::Value val;
            val[element["barcode"].asString()] = false;
            jBarcodes.append(val);
        }
        else{
            work.exec_params("insert into productcards(barcode, name, unit, category, subcategory, price, kdv, otv, producer) "
                            "values($1, $2, $3, $4, $5, $6, $7, $8, $9)", element["barcode"].asString(), element["name"].asString(), element["unit"].asInt(), element["category"].asInt(), element["subcategory"].asInt(), element["price"].asDouble(), element["kdv"].asInt(), element["otv"].asInt(), element["producer"].asInt());
            work.commit();

            // eklenen ürünün barkodunu cevaba true olarak kayıt etme.
            Json::Value val;
            val[element["barcode"].asString()] = true;
            jBarcodes.append(val);
        }
    }
    jResp["barcodes"] = jBarcodes;

    LOG_INFO << "Talep gelen ip adresi: " << req->peerAddr().toIp();
    auto resp = HttpResponse::newHttpJsonResponse(jResp);
    resp->setContentTypeCode(CT_APPLICATION_JSON);
    callback(resp);
}

void productController::addSubCategory(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback)
{
    auto json = req->getJsonObject();
    Json::Value jsonResponse;

    string newSubCategoryName;
    int parentID;
    Json::Value jsonSubCategory = (*json)["addSubCategory"];

    newSubCategoryName = jsonSubCategory["name"].asString();
    parentID = jsonSubCategory["parentID"].asInt();

    sqlResult = work.exec_params("select * from subcategories where parent = $1 and name = $2", parentID, newSubCategoryName);
    if(sqlResult.size() > 0){
        jsonResponse["addSubCategory"] = "error";
        jsonResponse["Message"] = "Bu alt kategori zaten mevcut";
    }
    else{
        try
        {
            work.exec_params("insert into subcategories(parent, name) values($1, $2)", parentID, newSubCategoryName);
            work.commit();
            jsonResponse["addSubCategory"] = "ok";
        }
        catch(const pqxx::sql_error& e)
        {
            LOG_INFO << e.what() << "\n";
            jsonResponse["addSubCategory"] = "error";
            jsonResponse["error_message"] = e.what();
        }
    }

    LOG_INFO << "Talep gelen ip adresi: " << req->peerAddr().toIp();
    auto resp = HttpResponse::newHttpJsonResponse(jsonResponse);
    resp->setContentTypeCode(CT_APPLICATION_JSON);
    callback(resp);
}

void productController::addCategory(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback)
{
    auto json = req->getJsonObject();
    string newCategoryName = (*json)["addCategory"].asString();
    
    Json::Value jsonResponse;

    sqlResult = work.exec_params("select * from categories where name = $1", newCategoryName);

    if(sqlResult.size() > 0){
        jsonResponse["addCategory"] = "error";
        jsonResponse["error_message"] = "This category already exists.";
    }
    else{
        work.exec_params("insert into categories(name) values($1)", newCategoryName);
        work.commit();
        jsonResponse["addCategory"] = "ok";
    }

    LOG_INFO << "Talep gelen ip adresi: " << req->peerAddr().toIp();
    auto resp = HttpResponse::newHttpJsonResponse(jsonResponse);
    resp->setContentTypeCode(CT_APPLICATION_JSON);
    callback(resp);
}

void productController::deleteProducts(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback)
{
    auto json = req->getJsonObject();
    Json::Value productIDs = Json::Value(Json::arrayValue);
    productIDs = (*json)["deleteProducts"];

    Json::Value jsonResponse;

    string sqlQueryText = "delete from productcards where id in(";

    int lastID = productIDs.size() -1;
    for(const auto& id : productIDs){
        sqlQueryText.append(id.asString());
        if(id != productIDs[lastID]){
            sqlQueryText.append(",");
        }
    }
    sqlQueryText.append(")");
    try
    {
        work.exec(sqlQueryText);
        work.commit();
        jsonResponse["deleteProducts"] = 1;
    }
    catch(const pqxx::sql_error& e)
    {
        jsonResponse["deleteProducts"] = 2; //hata veya hatalar var
        // jsonResponse["error_message"] = e.what();
    }
    
    LOG_INFO << "Talep gelen ip adresi: " << req->peerAddr().toIp();
    auto resp = HttpResponse::newHttpJsonResponse(jsonResponse);
    resp->setContentTypeCode(CT_APPLICATION_JSON);
    callback(resp);
}