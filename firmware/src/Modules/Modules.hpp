#pragma once

#pragma once

class Module {
public:
    virtual void Setup() = 0;
    virtual void Update() {};
};

void ModulesSetup();
void ModulesUpdate();
