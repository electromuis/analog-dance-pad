#include "hal/hal_Lights.hpp"
#include "ModuleLights.hpp"

ModuleLights ModuleLightsInstance = ModuleLights();

void ModuleLights::Setup()
{
    HAL_Lights_Setup();
}

void ModuleLights::Update()
{
    HAL_Lights_Update();
}

void ModuleLights::SetManualMode(bool mode)
{

}

void ModuleLights::SetManual(int index, rgb_color color)
{

}

void ModuleLights::DataCycle()
{
    
}
