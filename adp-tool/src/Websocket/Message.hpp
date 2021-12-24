#pragma once

namespace adp
{

    using namespace std;

// encoding text,json
struct Message
{
    bool isJson;
    string sender;

    int type;
    int messageId;
    bool idHandledByUser;
    string endpoint;
    string data;
};

}