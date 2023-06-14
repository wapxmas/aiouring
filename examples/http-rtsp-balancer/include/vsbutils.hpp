//
// Created by sergei on 29.06.22.
//

#ifndef VSBALANCER_VSBUTILS_HPP
#define VSBALANCER_VSBUTILS_HPP

#include <optional>
#include <vector>
#include <string>
#include <numeric>

#include "vsbtypes.h"

namespace vsbutils {
    static std::optional<std::string> getUniqueUriParts(
            std::vector<vsbtypes::TemplateTokens> &templates,
            std::vector<std::string> &urlTokens) {
        std::vector<std::string> uriParts{};

        for(auto &tpl : templates) {

            if(tpl.size() > urlTokens.size()) {
                continue;
            }

            for(int i = 0; i < urlTokens.size(); i++) {
                if(i >= tpl.size()) {
                    uriParts.clear();
                    break;
                }
                if (tpl[i] == "*") {
                    break;
                } else if (tpl[i] == "$") {
                    uriParts.push_back(urlTokens[i]);
                } else if (tpl[i] == "_") {
                    continue;
                } else if(tpl[i] != urlTokens[i]) {
                    uriParts.clear();
                    break;
                }
            }

            if(!uriParts.empty()) {
                break;
            }
        }

        if(uriParts.empty()) {
            return std::nullopt;
        }

        return std::accumulate(uriParts.begin(), uriParts.end(), std::string{});
    }
}

#endif //VSBALANCER_VSBUTILS_HPP
