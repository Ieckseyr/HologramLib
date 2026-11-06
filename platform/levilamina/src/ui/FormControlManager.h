#pragma once

#include "hologramlib/HologramLib.h"

#include <string>

namespace debugshape_export {

class FormControlManager {
public:
    static FormControlManager& getInstance();

    bool closeForm(std::string const& playerName);
    int  closeAllForms();

private:
    FormControlManager() = default;
};

} // namespace debugshape_export
