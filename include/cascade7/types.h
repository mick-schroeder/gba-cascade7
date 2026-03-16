#ifndef CASCADE7_TYPES_H
#define CASCADE7_TYPES_H

namespace cascade7
{
    enum class cell_kind
    {
        empty,
        numbered,
        blank,
        cracked_blank
    };

    struct cell
    {
        cell_kind kind = cell_kind::empty;
        int value = 0;

        [[nodiscard]] bool occupied() const
        {
            return kind != cell_kind::empty;
        }

        [[nodiscard]] bool numbered() const
        {
            return kind == cell_kind::numbered;
        }

        [[nodiscard]] bool blank() const
        {
            return kind == cell_kind::blank;
        }

        [[nodiscard]] bool cracked_blank() const
        {
            return kind == cell_kind::cracked_blank;
        }
    };
}

#endif
