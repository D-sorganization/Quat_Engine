// SPDX-License-Identifier: MIT
// Copyright (c) 2026 D-Sorganization. All rights reserved.
/**
 * @file RuntimeSession.h
 * @brief Pure session helpers for the QuatEngine demo runtime.
 */

#ifndef QE_DEMO_RUNTIME_SESSION_H
#define QE_DEMO_RUNTIME_SESSION_H

#include "core/Entity.h"

#include <iterator>
#include <string_view>
#include <vector>

namespace qe::demo {

inline int count_alive(const std::vector<qe::core::Entity>& entities) {
    int alive = 0;
    for (const auto& entity : entities) {
        if (entity.alive) {
            ++alive;
        }
    }
    return alive;
}

inline std::string_view weapon_name_for_index(int index) {
    constexpr std::string_view names[] = {
        "Pistol",
        "Shotgun",
        "RailGun",
        "Rocket",
        "MiniGun",
    };

    if (index < 0 || index >= static_cast<int>(std::size(names))) {
        return "?";
    }
    return names[index];
}

}  // namespace qe::demo

#endif  // QE_DEMO_RUNTIME_SESSION_H
