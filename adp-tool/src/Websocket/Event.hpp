#pragma once

#include "Message.hpp"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace adp
{

class Event
{
    public:
    Event(const string& event, const json& json, const string& rawJson)
    :m_isJson(true), m_event(event), m_json(&json), m_stringdata(rawJson)
    {
    }
    
    Event(const string& event, const string& data)
    :m_isJson(false), m_event(event), m_json(0), m_stringdata(data)
    {
    }

    bool isJson() const
    {
        return m_isJson;
    }

    string name() const
    {
        return m_event;
    }

    string data() const
    {
        return m_stringdata;
    }

    bool   m_isJson;
    string m_event;
    const json* m_json;
    string m_stringdata;
};

}