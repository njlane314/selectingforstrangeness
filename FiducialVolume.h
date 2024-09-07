#pragma once

inline bool point_inside_fv(float x, float y, float z) {
    const float fv_x_min = 0.0;
    const float fv_x_max = 256.0;
    const float fv_y_min = -116.0;
    const float fv_y_max = 116.0;
    const float fv_z_min = 0.0;
    const float fv_z_max = 1036.0;

    return (x > fv_x_min && x < fv_x_max &&
            y > fv_y_min && y < fv_y_max &&
            z > fv_z_min && z < fv_z_max);
}