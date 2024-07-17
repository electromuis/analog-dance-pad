#pragma once

#pragma once

class Module {
public:
    virtual void Setup() = 0;
    virtual void Update() {};
    virtual bool ShouldMainUpdate() {
        if(taskPrority == -1)
            return true;

        #ifndef FEATURE_RTOS_ENABLED
            return true;
        #endif

        return false;
    }
protected:
    int taskPrority = -1;
};

void ModulesSetup();
void ModulesUpdate();
