#include "productController.h"
#include "pqxx/pqxx"
#include "jsoncpp/json/json.h"
#include <string>
#include <fmt/format.h> // string ifadelere argüman eklemek için. c++20 ile geldi bu özellik ama bunda çalıştıramadım. o yüzden fmt kütüphanesini kullandım. örnek; fmt::format("Merhaba {0}.", "Mehmet")
#include <vector>
#include <openssl/sha.h>



void productController::sayHello(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback)
{
    LOG_INFO << "Talep gelen ip adresi: " << req->peerAddr().toIp();
    auto resp = HttpResponse::newHttpResponse();
    resp->setBody("Merhaba, Drogon çalışıyor...");
    callback(resp);
}

void productController::getProduct(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback, std::string pBarcode)
{
    pqxx::connection sqlConn("hostaddr=127.0.0.1 port=5432 dbname=bhssdb user=postgres password=postgres");
    pqxx::result sqlResult;
    pqxx::work work(sqlConn);
    sqlResult = work.exec_params("select p.id, p.barcode, p.name, p.fullname, p.unit, p.category, p.subcategory, p.price, p.kdv, p.otv, p.date, p.lastdate, p.producer, i.hash from productcards p "
                                    "left join images i on "
                                    "i.barcode = p.barcode "
                                    "where p.barcode = $1", pBarcode);

    Json::Value json;

    if(sqlResult.size() <= 0){
        
        Json::Value jVal;
        jVal["getProduct"] = Json::nullValue;
        jVal["message"] = "Ürün bulunamadı.";
        json = jVal;
    }
    else{
        for (auto &&row : sqlResult)
        {   
            Json::Value jProduct;
            jProduct["id"] = row[0].as<int>();
            jProduct["barcode"] = row[1].as<string>();
            jProduct["name"] = row[2].as<string>();
            jProduct["fullname"] = row[3].as<string>();
            jProduct["unit"] = row[4].as<int>();
            jProduct["category"] = row[5].as<int>();
            jProduct["subcategory"] = row[6].as<int>();
            jProduct["price"] = row[7].as<double>();
            jProduct["kdv"] = row[8].as<int>();
            jProduct["otv"] = row[9].as<int>();
            jProduct["date"] = pqxx::to_string(row[10]);
            jProduct["lastdate"] = pqxx::to_string(row[11]);
            jProduct["producer"] = row[12].as<int>();
            if(!row[13].is_null()){
                jProduct["image-hash"] = row[13].as<string>();
            }
            else{
                jProduct["image-hash"] = Json::nullValue;
            }
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

void productController::addProduct(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback)
{
    // bu metoda request ile resim dosyası ve text olarak json data geliyor.
    // bunları işleyip geri dönüş yapar.
    pqxx::connection sqlConn("hostaddr=127.0.0.1 port=5432 dbname=bhssdb user=postgres password=postgres");
    pqxx::result sqlResult;
    pqxx::work work(sqlConn);

    Json::Value jsonResponse;

    Json::Value json;

    MultiPartParser multiPart;
    multiPart.parse(req);

    if(multiPart.getFiles().size() > 0){
        
        for(auto &file : multiPart.getFiles()){

            if(file.getContentType() == ContentType::CT_APPLICATION_JSON){
                // char datayı Json::Value nesnesine çevirme
                // requeste gelen "data.json" dosyasını burada işliyorum.
                Json::CharReaderBuilder builder;
                std::istringstream jsonStream(file.fileData());
                Json::parseFromStream(builder, jsonStream, &json, nullptr);
                // char datayı çevirme bitti.

                // veritabanına işleme başlangıç.
                sqlResult = work.exec_params("select * from productcards where barcode = $1", json["barcode"].asString());
                if(sqlResult.size() > 0){
                    // barkod var ise kodlanacak...
                    // var olan ürünün barkodunu cevaba false olarak kayıt etme

                    jsonResponse["addProduct"] = "hata";
                    jsonResponse["error_message"] = "Bu barkod başka bir ürüne tanımlı.";
                    break;// ürün zaten var isefor'dan çıkması için.
                }
                else{
                    work.exec_params("insert into productcards(barcode, name, fullname, unit, category, subcategory, price, kdv, otv, producer) "
                                "values($1, $2, $3, $4, $5, $6, $7, $8, $9, $10)", json["barcode"].asString(), json["name"].asString(), json["fullname"].asString(), json["unit"].asInt(), json["category"].asInt(), json["subcategory"].asInt(), json["price"].asDouble(), json["kdv"].asInt(), json["otv"].asInt(), json["producer"].asInt());
                    
                    work.commit();

                    Json::Value jsonFileValue;
                    jsonFileValue["json"] = "ok";
                    jsonResponse["files"].append(jsonFileValue);
                    jsonResponse["addProduct"] = "ok";
                    LOG_INFO << "Stok kartı kaydedildi.";
                }
            }
            if(file.getContentType() == ContentType::CT_IMAGE_PNG){
                
                string fileMD5 = file.getMd5();
                string newFileName = fileMD5 + "." + std::string(file.getFileExtension());
                file.saveAs(newFileName);
                //Magick++ ile resim dosyasını 512x512 olarak yenien boyutlandırma ve aynı isimle tekrar kayıt etme.
                Image img(app().getUploadPath() + "/" + newFileName);
                img.resize("512x512");
                img.write(app().getUploadPath() + "/" + newFileName);

                //veritabanına da bu bilgileri kaydedelim.
                /*
                    burayı iyileştir. daha önce kaydedilmişmi vb. şeyler.
                */
                work.exec_params("insert into images (barcode, hash) values($1, $2)", json["barcode"].asString(), fileMD5);
                work.commit();

                Json::Value imageJson;
                imageJson["image"] = "ok";
                jsonResponse["files"].append(imageJson);
                LOG_INFO << "resim kaydedildi.";
            }
        }
    }
    else{
        jsonResponse["addProduct"] = "hata";
        jsonResponse["error_message"] = "requestte hiç dosya yok.";
    }

    LOG_INFO << "Metod: addProduct()";
    LOG_INFO << "Talep gelen ip adresi: " << req->peerAddr().toIp();
    auto resp = HttpResponse::newHttpJsonResponse(jsonResponse);
    resp->setContentTypeCode(CT_APPLICATION_JSON);
    callback(resp);
}

void productController::addSubCategory(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback)
{
    pqxx::connection sqlConn("hostaddr=127.0.0.1 port=5432 dbname=bhssdb user=postgres password=postgres");
    pqxx::result sqlResult;
    pqxx::work work(sqlConn);

    auto json = req->getJsonObject();
    Json::Value jsonResponse;

    string newSubCategoryName;
    Json::Value jsonSubCategory = (*json)["addSubCategory"];

    newSubCategoryName = jsonSubCategory["name"].asString();
    int parentID = jsonSubCategory["parentID"].asInt();

    sqlResult = work.exec_params("select * from subcategories where parent = $1 and name = $2", parentID, newSubCategoryName);
    work.commit();
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
    pqxx::connection sqlConn("hostaddr=127.0.0.1 port=5432 dbname=bhssdb user=postgres password=postgres");
    pqxx::result sqlResult;
    pqxx::work work(sqlConn);

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
    pqxx::connection sqlConn("hostaddr=127.0.0.1 port=5432 dbname=bhssdb user=postgres password=postgres");
    pqxx::result sqlResult;
    pqxx::work work(sqlConn);

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

void productController::getCategoriesAndSubCategories(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback)
{
    pqxx::connection sqlConn("hostaddr=127.0.0.1 port=5432 dbname=bhssdb user=postgres password=postgres");
    pqxx::work work(sqlConn);

    string query = "select id, name from categories";
    pqxx::result sqlResult(work.exec(query));

    Json::Value json;

    if(sqlResult.size() > 0){

        Json::Value categories = Json::Value(Json::arrayValue);
        for(const auto &row : sqlResult){

            Json::Value category;
            category["id"] = row[0].as<int>();
            category["name"] = row[1].as<std::string>();

            string query = "select id, name from subcategories where parent = $1";
            pqxx::result sqlResult(work.exec_params(query, row[0].as<int>()));

            if(sqlResult.size() > 0){

                category["subcategories"] = Json::Value(Json::arrayValue);

                for(const auto &row : sqlResult){

                    Json::Value subCategory;
                    subCategory["id"] = row[0].as<int>();
                    subCategory["name"] = row[1].as<std::string>();
                    category["subcategories"].append(subCategory);
                }
            }
            else{
                category["subcategories"] = Json::Value(Json::arrayValue);
            }

            categories.append(category);
        }
        json["categoriesandsubcategories"] = categories;
    }
    else {
        json["categoriesandsubcategories"] = Json::nullValue;
        json["error_message"] = "Her hangi bir kategori veya alt kategori bulunamadı.";
    }

    LOG_INFO << "Talep gelen ip adresi: " << req->peerAddr().toIp();
    auto response = HttpResponse::newHttpJsonResponse(json);
    response->setContentTypeCode(CT_APPLICATION_JSON);
    callback(response);
}

void productController::getUnits(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback)
{
    pqxx::connection sqlConn("hostaddr=127.0.0.1 port=5432 dbname=bhssdb user=postgres password=postgres");

    pqxx::work work(sqlConn);
    string queryText = "select * from units order by id";
    pqxx::result sqlResult(work.exec(queryText));

    Json::Value jRoot;

    if(sqlResult.size() > 0){

        Json::Value unitsAArray = Json::Value(Json::arrayValue);
        for (const auto &row : sqlResult)
        {
            Json::Value jObject;
            jObject["id"] = row[0].as<int>();
            jObject["name"] = row[1].as<string>();
            unitsAArray.append(jObject);
        }
        jRoot["units"] = unitsAArray;
    }
    else{
        jRoot["units"] = Json::nullValue;
        jRoot["error_message"] = "Hiç bir birim bulunamadı.";
    }

    Json::Value json = jRoot;

    LOG_INFO << "Talep gelen ip adresi: " << req->peerAddr().toIp();
    auto response = HttpResponse::newHttpJsonResponse(json);
    response->setContentTypeCode(CT_APPLICATION_JSON);
    callback(response);
}

void productController::getCategories(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback)
{
    pqxx::connection sqlConn("hostaddr=127.0.0.1 port=5432 dbname=bhssdb user=postgres password=postgres");
    pqxx::work work(sqlConn);

    string query = "select id, name from categories";
    pqxx::result sqlResult(work.exec(query));

    Json::Value jsonResponse;

    if(sqlResult.size() > 0){
        
        Json::Value categories = Json::Value(Json::arrayValue);
        for(const auto& row : sqlResult){
            
            Json::Value category;
            category["id"] = row[0].as<int>();
            category["name"] = row[1].as<string>();
            categories.append(category);
        }

        jsonResponse["categories"] = categories;
    }
    else {
        jsonResponse["categories"] = Json::nullValue;
        jsonResponse["error_message"] = "Her hangi bir kategori bulunamadı.";
    }

    LOG_INFO << "Talep gelen ip adresi: " << req->peerAddr().toIp();
    auto response = HttpResponse::newHttpJsonResponse(jsonResponse);
    response->setContentTypeCode(CT_APPLICATION_JSON);
    callback(response);
}

void productController::getSubCategories( const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback)
{
    pqxx::connection sqlConn("hostaddr=127.0.0.1 port=5432 dbname=bhssdb user=postgres password=postgres");
    pqxx::work work(sqlConn);

    string query = "select id, parent, name from subcategories";
    pqxx::result sqlResult(work.exec(query));

    Json::Value json;

    if(sqlResult.size() > 0){

        Json::Value subCategories = Json::Value(Json::arrayValue);

        for(const auto& row : sqlResult){
            
            Json::Value subcategory;
            subcategory["id"] = row[0].as<int>();
            subcategory["parent"] = row[1].as<int>();
            subcategory["name"] = row[2].as<string>();
            subCategories.append(subcategory);
        }

        json["subcategories"] = subCategories;
    }
    else {
        json["subcategories"] = Json::nullValue;
        json["error_message"] = "Her hangi bir altkategori bulunamadı.";
    }

    LOG_INFO << "Talep gelen ip adresi: " << req->peerAddr().toIp();
    auto response = HttpResponse::newHttpJsonResponse(json);
    response->setContentTypeCode(CT_APPLICATION_JSON);
    callback(response);
}

void productController::getProducers(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback)
{
    pqxx::connection sqlConn("hostaddr=127.0.0.1 port=5432 dbname=bhssdb user=postgres password=postgres");
    pqxx::work work(sqlConn);

    string query = "select id, name from producers";
    pqxx::result sqlResult(work.exec(query));

    Json::Value json;

    if(sqlResult.size() > 0){
        Json::Value producers = Json::Value(Json::arrayValue);

        for(const auto& row : sqlResult){
            
            Json::Value producer;
            producer["id"] = row[0].as<int>();
            producer["name"] = row[1].as<string>();
            producers.append(producer);
        }

        json["producers"] = producers;
    }
    else{
        json["producers"] = Json::nullValue;
        json["error_message"] = "Herhangi bir üretici bulunamadı.";
    }

    LOG_INFO << "Talep gelen ip adresi: " << req->peerAddr().toIp();
    auto response = HttpResponse::newHttpJsonResponse(json);
    response->setContentTypeCode(CT_APPLICATION_JSON);
    callback(response);
}

void productController::addProducer(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback, std::string pProducer)
{
    pqxx::connection sqlConn("hostaddr=127.0.0.1 port=5432 dbname=bhssdb user=postgres password=postgres");
    pqxx::work work(sqlConn);

    Json::Value json;

    pqxx::result sqlResult(work.exec_params("select * from producers where name = $1", pProducer));

    if(sqlResult.size() <= 0){
        
        pqxx::result res = work.exec_params("insert into producers(name) values($1)", pProducer);

        if(res.affected_rows() > 0){// insert başarılı.
            
            work.commit();
            json["addProducer"] = "ok";

            LOG_INFO << "\n" << "Talep Metod: addProdducer()" << "\n" << "Talep ip adresi: " << req->getPeerAddr().toIp();
            auto response = HttpResponse::newHttpJsonResponse(json);
            response->setContentTypeCode(CT_APPLICATION_JSON);
            response->setStatusCode(drogon::k200OK);
            callback(response);
        }
        else{
            json["addProducer"] = Json::nullValue;
            json["error_message"] = "insert işlemi yapılamadı.";

            LOG_INFO << "\n" << "Talep Metod: addProdducer()" << "\n" << "Talep ip adresi: " << req->getPeerAddr().toIp();
            auto response = HttpResponse::newHttpJsonResponse(json);
            response->setContentTypeCode(CT_APPLICATION_JSON);
            response->setStatusCode(drogon::k200OK);
            callback(response);
        }
    }
    else{
        json["addProducer"] = Json::nullValue;
        json["error_message"] = "Bu üretici zaten mevcut.";

        LOG_INFO << "\n" << "Talep Metod: addProdducer()" << "\n" << "Talep ip adresi: " << req->getPeerAddr().toIp();
        
        auto response = HttpResponse::newHttpJsonResponse(json);
        response->setContentTypeCode(CT_APPLICATION_JSON);
        response->setStatusCode(drogon::k200OK);
        callback(response);
    }
}

void productController::getProductCards(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback)
{
    pqxx::connection sqlConn("hostaddr=127.0.0.1 port=5432 dbname=bhssdb user=postgres password=postgres");
    pqxx::result sqlResult;
    pqxx::work work(sqlConn);

    auto json = req->getJsonObject();

    // sql sorgu cümlesini hazırlama başlangıç...
    string sqlQuery = "";

    if((*json)["categories"].isArray()){ // array ise kategori belirtilmiş. değil se "categories" = "all" gelir.
        
        sqlQuery = "select p.id, p.barcode, p.name, p.fullname, p.unit, p.category, p.subcategory, p.price, p.kdv, p.otv, p.date, p.lastdate, p.producer, i.hash from productcards p "
                            "left join images i on i.barcode = p.barcode "
                            "where category in(";

        Json::Value categories = Json::Value(Json::arrayValue);
        categories = (*json)["categories"];

        int last = categories.size() -1;
        for(const auto &id : categories){
            sqlQuery.append(id.as<std::string>());
            if(id != categories[last]){
                sqlQuery += ", ";
            }
        }
        sqlQuery.append(")");
        // sql sorgu cümlesi hazırlama bitiş.........
    }
    else if((*json)["categories"].asString() == "all"){
        
        sqlQuery = "select p.id, p.barcode, p.name, p.fullname, p.unit, p.category, p.subcategory, p.price, p.kdv, p.otv, p.date, p.lastdate, p.producer, i.hash from productcards p "
                        "left join images i on i.barcode = p.barcode ";
    }

    sqlResult = work.exec(sqlQuery);
    work.commit();

    Json::Value jsonResp;

    if(sqlResult.size() > 0){
        
        Json::Value productCards = Json::Value(Json::arrayValue);

        for(const auto &row : sqlResult){

            Json::Value product;
            product["id"] = row[0].as<int>();
            product["barcode"] = row[1].as<string>();
            product["name"] = row[2].as<string>();
            product["fullname"] = row[3].as<string>();
            product["unit"] = row[4].as<int>();
            product["category"] = row[5].as<int>();
            product["subcategory"] = row[6].as<int>();
            product["price"] = row[7].as<double>();
            product["kdv"] = row[8].as<int>();
            product["otv"] = row[9].as<int>();
            product["date"] = pqxx::to_string(row[10]);
            product["lastdate"] = pqxx::to_string(row[11]);
            product["producer"] = row[12].as<int>();
            if(!row[13].is_null()){
                product["image-hash"] = row[13].as<string>();
            }
            else{
                product["image-hash"] = Json::nullValue;
            }
            productCards.append(product);
        }

        jsonResp["productCards"] = productCards;
    }

    LOG_INFO << "\n" << "Talep gelen ip: " << req->getPeerAddr().toIp();
    LOG_INFO << "\n" << "Metod: " << "getProductCards()";
    auto response = HttpResponse::newHttpJsonResponse(jsonResp);
    response->setContentTypeCode(CT_APPLICATION_JSON);
    response->setStatusCode(drogon::k200OK);
    callback(response);
}