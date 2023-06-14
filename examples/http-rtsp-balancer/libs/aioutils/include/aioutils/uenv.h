//
// Created by sergei on 19.06.22.
//

#ifndef VS_BALANCER_UENV_H
#define VS_BALANCER_UENV_H

namespace aioutils::uenv {
    std::string getEnvironmentVariable(const char *variableName);

    void setVariableFromEnvironment(const std::string& variableName, int &variableToSet, bool greaterZero);
    void setVariableFromEnvironment(const char *variableName, int &variableToSet, bool greaterZero = false);

    void setVariableFromEnvironment(const std::string& variableName, std::string &variableToSet);
    void setVariableFromEnvironment(const char* variableName, std::string &variableToSet);

    void setVariableFromEnvironment(const std::string& variableName, bool &variableToSet);
    void setVariableFromEnvironment(const char* variableName, bool &variableToSet);
}

#endif //VS_BALANCER_UENV_H
