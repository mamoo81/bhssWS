#include "verify.h"

#include <unordered_map>

string senderMail = "destek@basat.dev";
string senderMailPassword = "66,veRoNK,;";

Verify::Verify()
{
    
}

string Verify::generateRandomCode()
{
    const string characters = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::random_device rd; // rastgele cihaz oluşturur.
    std::mt19937 generator(rd()); // rastgele sayı üretici
    std::uniform_int_distribution<> distribution(0, characters.size() -1);

    string randomString;

    for (size_t i = 0; i < 6; i++)
    {
        randomString += characters[distribution(generator)];
    }

    return randomString;
}

int Verify::OTPSend(std::string email, std::string phone, VerifyChannel channel)
{

    if(channel == VerifyChannel::MAIL){

        cout << OTPCodes.size() << endl;
        auto it = OTPCodes.find(email);
        if(it == OTPCodes.end()){

            std::string generatedOTPCode = generateRandomCode(); // doğrulama için rastgele kod oluştur.
            
            try
            {
                message mailto;
                mailto.from(mail_address("Basat Yazilim", senderMail)); // mail kimden gidecek
                mailto.add_recipient(mail_address("", email)); // mail kime gidecek. "" içine alıcı ad-soyad yazılabilir.
                mailto.subject("Yeni üyelik doğrulama kodu"); // mail konu başlığı
                
                mailto.content(string("Doğrulama kodunuz: " + generatedOTPCode + "       Doğrulama kodunu kimseyle paylaşmayınız.")); // mail metni ve doğrulama kodu.

                smtps smtpsServer("mail.basat.dev", 587); // smtp serverini tanımlama.
                smtpsServer.authenticate("destek@basat.dev", "66,veRoNK,;", smtps::auth_method_t::START_TLS); // gönderici mail bilgileri ve doğrulama.
                smtpsServer.submit(mailto); // maili gönder.

                Otp code = Otp(generatedOTPCode, std::chrono::system_clock::now());
                OTPCodes.insert(std::make_pair(email, code));
            }
            catch (smtp_error& exc)
            {
                cout << exc.what() << endl;
            }
            catch (dialog_error& exc)
            {
                cout << exc.what() << endl;
            }

            return VerifyErrors::Successful;
        }
        else{
            // cout << email << endl;
            // cout << "kod: " << OTPCodes.at(email).code << endl;
            return VerifyErrors::Inprocess;
        }
    }
    if(channel == VerifyChannel::SMS){

    }
    return VerifyErrors::Correct;
}

int Verify::verifyOTPCode(std::string email, std::string phone, std::string code, VerifyChannel channel)
{
    if(channel == VerifyChannel::MAIL){
        
        auto it = OTPCodes.find(email);
        if(it != OTPCodes.end()){
            std::string CodeValue = OTPCodes[email].getCode();
            std::chrono::system_clock::time_point create_time = OTPCodes[email].getTime();

            auto duration = std::chrono::duration_cast<std::chrono::minutes>(std::chrono::system_clock::now() - create_time);

            if(duration.count() <= 10){

                if(CodeValue == code){
                    return VerifyErrors::Successful;
                }
                else{
                    return VerifyErrors::Correct;
                }
            }
            else{
                OTPCodes.erase(email);
                return VerifyErrors::Expired;
            }
        }
        else{
            return VerifyErrors::Correct;
        }
    }
    
    return VerifyErrors::Correct;
}