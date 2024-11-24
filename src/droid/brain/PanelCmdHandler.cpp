/*
 * Droid Brain Program
 * Author: Kizmit99
 * License: CC BY-NC-SA 4.0
 *
 * This source code is open-source for non-commercial use. 
 * For commercial use, please obtain a license from the author.
 * For more information, visit https://github.com/kizmit99/DroidBrain
 */

#include "droid/brain/PanelCmdHandler.h"
#include "droid/services/DroidState.h"

namespace droid::brain {
    PanelCmdHandler::PanelCmdHandler(const char* name, droid::core::System* system) :
        CmdHandler(name, system) {}

    bool PanelCmdHandler::process(const char* device, const char* command) {
        //TODO
        return false;
    }

    void PanelCmdHandler::init() {
        //TODO
    }

    void PanelCmdHandler::factoryReset() {
        //TODO
    }

    void PanelCmdHandler::task() {
        //TODO
    }

    void PanelCmdHandler::logConfig() {
        //TODO
    }

    void PanelCmdHandler::failsafe() {
        //TODO
    }
}