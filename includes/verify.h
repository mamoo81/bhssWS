#ifndef VERIFY_H
#define VERIFY_H

#include <iostream>
#include <unordered_map>
#include <random>
#include <chrono>
#include <string>
#include <thread>
#include <ctime>

using namespace std;

#include "mailio/message.hpp"
#include "mailio/smtp.hpp"

using mailio::message;
using mailio::mail_address;
using mailio::string_t;
using mailio::smtps;
using mailio::mime;
using mailio::smtp_error;
using mailio::dialog_error;

struct Otp
{
    public:
    Otp() = default;
    Otp(string c, chrono::system_clock::time_point ct) : code(c), creatingTime(ct) {}

    string getCode() {
        return code;
    }

    chrono::system_clock::time_point getTime(){
        return creatingTime;
    }

    private:
    string code;
    chrono::system_clock::time_point creatingTime;
};

class Verify
{
    public:
    Verify();

    enum VerifyChannel {
        SMS = 1,
        MAIL = 2,
        UNKNOWN = 3
    };
    enum VerifyErrors {
        Expired = 1,
        Inprocess = 2,
        Successful = 3,
        Correct = 4,
        IdentityMistake = 5
    };

    int OTPSend(string email, string phone, VerifyChannel channel);
    int verifyOTPCode(string email, string phone, string code, VerifyChannel channel);

    private:
    std::string generateRandomCode();
    std::unordered_map<string, Otp> OTPCodes;
};
#endif