//
// Created by sergei on 19.06.22.
//

#include <string>
#include <kklogging/kklogging.h>
#include "hputils/uenv.h"
#include "hputils/utext.h"

using namespace hputils;

namespace hputils::uenv {
    void setVariableFromEnvironment(const std::string& variableName, int &variableToSet, bool greaterZero) {
        setVariableFromEnvironment(variableName.c_str(), variableToSet, greaterZero);
    }

    void setVariableFromEnvironment(const char *variableName, int &variableToSet, bool greaterZero) {
        auto envVariableValueStr = getEnvironmentVariable(variableName);

        if(!envVariableValueStr.empty()) {
            try {
                int temp = std::stoi(envVariableValueStr);

                if(greaterZero && temp <= 0)
                {
                    return;
                }

                if(temp >= 0) {
                    variableToSet = temp;
                }
            } catch(const std::exception &e) {
                kklogging::ERROR("Exception while parsing environment variable " +
                               std::string(variableName) + ": " + std::string(e.what()));
            }
        }
    }

    void setVariableFromEnvironment(const std::string& variableName, bool &variableToSet) {
        setVariableFromEnvironment(variableName.c_str(), variableToSet);
    }

    void setVariableFromEnvironment(const char* variableName, bool &variableToSet) {
        auto envVariableValueStr = getEnvironmentVariable(variableName);

        utext::trim(envVariableValueStr);
        STRTOLOWER(envVariableValueStr);

        if(!envVariableValueStr.empty()) {
            variableToSet = envVariableValueStr[0] == 't';
        }
    }

    void setVariableFromEnvironment(const std::string& variableName, std::string &variableToSet) {
        setVariableFromEnvironment(variableName.c_str(), variableToSet);
    }

    void setVariableFromEnvironment(const char* variableName, std::string &variableToSet) {
        auto envVariableValueStr = getEnvironmentVariable(variableName);
        if(!envVariableValueStr.empty()) {
            variableToSet = envVariableValueStr;
        }
    }

    std::string getEnvironmentVariable(const char *variableName) {
        const char *variableValue = std::getenv(variableName);
        return std::string{variableValue == nullptr ? "" : variableValue};
    }
}