#include "productController.h"
#include "pqxx/pqxx"
#include "jsoncpp/json/json.h"
#include <string>
#include <fmt/format.h> // string ifadelere argüman eklemek için. c++20 ile geldi bu özellik ama bunda çalıştıramadım. o yüzden fmt kütüphanesini kullandım. örnek; fmt::format("Merhaba {0}.", "Mehmet")
#include <vector>
#include <openssl/sha.h>

pqxx::connection sqlConn("hostaddr=127.0.0.1 port=5432 dbname=bhssdb user=postgres password=postgres");

void productController::sayHello(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback)
{
    LOG_INFO << "Talep gelen ip adresi: " << req->peerAddr().toIp();
    auto resp = HttpResponse::newHttpResponse();
    resp->setBody("Merhaba, Drogon çalışıyor...");
    callback(resp);
}

void productController::getProduct(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback, std::string pBarcode)
{
    pqxx::result sqlResult;
    pqxx::work work(sqlConn);
    sqlResult = work.exec_params("select p.id, p.barcode, p.name, p.fullname, p.unit, p.category, p.subcategory, p.price, p.kdv, p.otv, p.date, p.lastdate, p.producer, i.hash from productcards p "
                                    "left join images i on "
                                    "i.barcode = p.barcode "
                                    "where p.barcode = $1", pBarcode);

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
            jProduct["image-hash"] = row[13].as<string>();

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
        1 ise tüm kayıt başarılı
        2 ise hata veya hatalar var   
*/
void productController::addProduct(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback)
{
    // bu metoda request ile 2 dosya geliyor.
    // biri "product.json" dosyası diğeri ürün resmi "resim.json"
    // bunları işleyip geri dönüş yapar.
    pqxx::result sqlResult;
    pqxx::work work(sqlConn);

    Json::Value jsonResponse;
    // jsonResponse["addProduct"] = "ok";

    MultiPartParser fileUpload;
    fileUpload.parse(req);

    if(fileUpload.getFiles().size() != 0){

        Json::Value json;

        for(auto &file : fileUpload.getFiles()){

            if(file.getFileType() == FileType::FT_CUSTOM){

                // char datayı Json::Value nesnesine çevirme
                // requeste gelen "product.json" dosyasını burada işliyorum.
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
                }
                else{
                    work.exec_params("insert into productcards(barcode, name, fullname, unit, category, subcategory, price, kdv, otv, producer) "
                                "values($1, $2, $3, $4, $5, $6, $7, $8, $9, $10)", json["barcode"].asString(), json["name"].asString(), json["fullname"].asString(), json["unit"].asInt(), json["category"].asInt(), json["subcategory"].asInt(), json["price"].asDouble(), json["kdv"].asInt(), json["otv"].asInt(), json["producer"].asInt());
                    
                    work.commit();

                    jsonResponse["addProduct"] = "ok";
                }
            }
            else if(file.getFileType() == FileType::FT_IMAGE){
                
                string fileMD5 = file.getMd5();
                string newFileName = fileMD5 + "." + std::string(file.getFileExtension());
                file.saveAs(newFileName);

                //veritabanına da bu bilgileri kaydedelim.
                /*
                    burayı iyileştir. daha önce kaydedilmişmi vb. şeyler.
                */
                work.exec_params("insert into images (barcode, hash) values($1, $2)", json["barcode"].asString(), fileMD5);
                work.commit();
            }
        }
    }
    else{
        jsonResponse["addProduct"] = "hata";
        jsonResponse["error_message"] = "requestte hiç dosya yok.";
    }

    LOG_INFO << "Talep gelen ip adresi: " << req->peerAddr().toIp();
    auto resp = HttpResponse::newHttpJsonResponse(jsonResponse);
    resp->setContentTypeCode(CT_APPLICATION_JSON);
    callback(resp);
}

void productController::addSubCategory(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback)
{
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
    pqxx::work work(sqlConn);

    Json::Value jsonResponse;

    string query = "select id, name from categories";
    pqxx::result sqlResult(work.exec(query));

    if(sqlResult.size() > 0){

        Json::Value jsonRoot;

        for(const auto &row : sqlResult){

            Json::Value category;
            
            category["id"] = row[0].as<int>();
            category["name"] = row[1].as<std::string>();
            category["sub_categories"] = Json::Value(Json::arrayValue);

            string query = "select id, name from subcategories where parent = $1";
            pqxx::result sqlResult(work.exec_params(query, row[0].as<int>()));

            if(sqlResult.size() > 0){

                for(const auto &row : sqlResult){

                    Json::Value subCategory;
                    subCategory["id"] = row[0].as<int>();
                    subCategory["name"] = row[1].as<std::string>();
                    category["sub_categories"].append(subCategory);
                }
            }

            jsonRoot.append(category);
        }

        jsonResponse = jsonRoot;

    }
    else {
        jsonResponse["categoriesandsubcategories"] = Json::nullValue;
        jsonResponse["error_message"] = "Her hangi bir kategori veya alt kategori bulunamadı.";
    }

    auto response = HttpResponse::newHttpJsonResponse(jsonResponse);
    response->setContentTypeCode(CT_APPLICATION_JSON);
    callback(response);
}

void productController::getUnits(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback)
{
    Json::Value jRoot;

    pqxx::work work(sqlConn);
    string queryText = "select * from units order by id";
    pqxx::result sqlResult(work.exec(queryText));

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

    auto response = HttpResponse::newHttpJsonResponse(json);
    response->setContentTypeCode(CT_APPLICATION_JSON);
    callback(response);
}

void productController::getCategories(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback)
{
    pqxx::work work(sqlConn);

    Json::Value jsonResponse;

    string query = "select id, name from categories";
    pqxx::result sqlResult(work.exec(query));

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

    auto response = HttpResponse::newHttpJsonResponse(jsonResponse);
    response->setContentTypeCode(CT_APPLICATION_JSON);
    callback(response);
}

void productController::getSubCategories( const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)> &&callback)
{
    pqxx::work work(sqlConn);

    Json::Value jsonResponse;

    string query = "select id, parent, name from subcategories";
    pqxx::result sqlResult(work.exec(query));

    if(sqlResult.size() > 0){

        Json::Value subCategories = Json::Value(Json::arrayValue);

        for(const auto& row : sqlResult){
            
            Json::Value subcategory;
            subcategory["id"] = row[0].as<int>();
            subcategory["parent"] = row[1].as<int>();
            subcategory["name"] = row[2].as<string>();
            subCategories.append(subcategory);
        }

        jsonResponse["subcategories"] = subCategories;
    }
    else {
        jsonResponse["subcategories"] = Json::nullValue;
        jsonResponse["error_message"] = "Her hangi bir altkategori bulunamadı.";
    }

    auto response = HttpResponse::newHttpJsonResponse(jsonResponse);
    response->setContentTypeCode(CT_APPLICATION_JSON);
    callback(response);
}