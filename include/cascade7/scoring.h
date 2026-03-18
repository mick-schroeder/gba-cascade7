#ifndef CASCADE7_SCORING_H
#define CASCADE7_SCORING_H

namespace cascade7::scoring
{
    constexpr int full_clear_bonus = 70000;
    constexpr int rise_bonus = 7000;
    constexpr int large_chain_threshold = 3;
    constexpr int large_popup_score_threshold = 1500;
    constexpr int popup_frames = 72;

    [[nodiscard]] inline int _isqrt(int value)
    {
        if(value <= 0)
        {
            return 0;
        }

        int bit = 1 << 30;

        while(bit > value)
        {
            bit >>= 2;
        }

        int result = 0;

        while(bit != 0)
        {
            if(value >= result + bit)
            {
                value -= result + bit;
                result = (result >> 1) + bit;
            }
            else
            {
                result >>= 1;
            }

            bit >>= 2;
        }

        return result;
    }

    [[nodiscard]] inline int per_disc_score(int chain_depth)
    {
        if(chain_depth <= 0)
        {
            return 0;
        }

        // Scoring curve: 7 * chain^2.5
        // Approximated as 7 * chain^2 * sqrt(chain) using integer math.
        const int scaled_root = _isqrt(chain_depth << 10);   // sqrt(chain * 1024) ~= sqrt(chain) * 32
        return (7 * chain_depth * chain_depth * scaled_root) >> 5;
    }
}

#endif
