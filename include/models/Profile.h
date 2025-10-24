#ifndef PROFILE_H
#define PROFILE_H

#include <string>

namespace Models {

struct ShippingAddress {
    std::string name;
    std::string line1;
    std::string line2;
    std::string city;
    std::string state;
    std::string postCode;
    std::string phone;
};

struct PaymentDetails {
    std::string nameOnCard;
    std::string cardNumber;
    std::string cardCvv;
    std::string cardExpMonth;
    std::string cardExpYear;
};

struct Profile {
    std::string name;
    ShippingAddress shippingAddress;
    PaymentDetails paymentDetails;
};

} // namespace Models

#endif // PROFILE_H
