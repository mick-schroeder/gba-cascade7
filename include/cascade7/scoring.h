#ifndef CASCADE7_SCORING_H
#define CASCADE7_SCORING_H

#include <array>

namespace cascade7::scoring
{
    constexpr int full_clear_bonus = 70000;
    constexpr int rise_bonus = 7000;
    constexpr int large_chain_threshold = 3;
    constexpr int large_popup_score_threshold = 1500;
    constexpr int popup_frames = 45;
    constexpr std::array<int, 16> per_disc_curve = {
        0, 7, 40, 109, 224, 391, 617, 907, 1267, 1702, 2217, 2817, 3506, 4290, 5172, 6157
    };

    [[nodiscard]] inline int per_disc_score(int chain_depth)
    {
        if(chain_depth < int(per_disc_curve.size()))
        {
            return per_disc_curve[chain_depth];
        }

        const int last_value = per_disc_curve.back();
        const int extra_depth = chain_depth - int(per_disc_curve.size()) + 1;
        return last_value + (extra_depth * 1200);
    }
}

#endif
