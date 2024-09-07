#ifndef _fiducial_volume_selector_h_
#define _fiducial_volume_selector_h_

#include "TVector3.h"
#include "AnalysisEvent.h"
#include "Selector.h"
#include <vector>
#include <cmath>

class FiducialVolumeSelector : public Selector {
public:
    enum FiducialVolume { kOldFV, kWholeTPC, kWirecell, kWholeTPCPadded, kWirecellPadded };

    FiducialVolumeSelector(int version, double padding = 0.0) 
        : version_(version), padding_(padding) 
    {
        if (version_ == kWirecell || version_ == kWirecellPadded) 
        {
            setup_wirecell_fv();
        }
    }

    bool pass_selection(const AnalysisEvent& e) const override 
    {
        TVector3 position(e.nu_vtx_x, e.nu_vtx_y, e.nu_vtx_z); 
        return point_inside_fv(position);
    }

    bool is_point_inside_fv(const TVector3& point) const
    {
        return point_inside_fv(point);
    }

private:
    int version_;
    double padding_;

    // TPC boundaries and constants
    const double tpc_xmin_ = -1.55;
    const double tpc_xmax_ = 254.8;
    const double tpc_ymin_ = -115.53;
    const double tpc_ymax_ = 117.47;
    const double tpc_zmin_ = 0.1;
    const double tpc_zmax_ = 1036.9;
    const double deadz_min_ = 675.1;
    const double deadz_max_ = 775.1;
    const double boundary_dis_cut_ = 3.0;

    // Wirecell FV
    const double yx_top_y1_array_     = 116;
    const double yx_top_x1_array_[11] = {0, 150.00, 132.56, 122.86, 119.46, 114.22, 110.90, 115.85, 113.48, 126.36, 144.21};
    const double yx_top_y2_array_[11] = {0, 110.00, 108.14, 106.77, 105.30, 103.40, 102.18, 101.76, 102.27, 102.75, 105.10};
    const double yx_top_x2_array_     = 256;

    const double yx_bot_y1_array_     = -115;
    const double yx_bot_x1_array_[11] = {0, 115.71, 98.05, 92.42, 91.14, 92.25, 85.38, 78.19, 74.46, 78.86, 108.90};
    const double yx_bot_y2_array_[11] = {0, -101.72, -99.46, -99.51, -100.43, -99.55, -98.56, -98.00, -98.30, -99.32, -104.20};
    const double yx_bot_x2_array_     = 256;

    /// ZX view has Y dependence: Y sub-range from -116 to 116cm per 24cm
    const double zx_up_z1_array_ = 0;
    const double zx_up_x1_array_ = 120;
    const double zx_up_z2_array_ = 11;
    const double zx_up_x2_array_ = 256;

    const double zx_dw_z1_array_     = 1037;
    const double zx_dw_x1_array_[11] = {0, 120.00, 115.24, 108.50, 110.67, 120.90, 126.43, 140.51, 157.15, 120.00, 120.00};
    const double zx_dw_z2_array_[11] = {0, 1029.00, 1029.12, 1027.21, 1026.01, 1024.91, 1025.27, 1025.32, 1027.61, 1026.00, 1026.00};
    const double zx_dw_x2_array_     = 256;

    std::vector<double> boundary_xy_x, boundary_xy_y;
    std::vector<double> boundary_xz_x, boundary_xz_z;

    std::vector<std::vector<double>> boundary_xy_x_array_, boundary_xy_y_array_;
    std::vector<std::vector<double>> boundary_xz_x_array_, boundary_xz_z_array_;

    const double m_anode_ = 0;
    const double m_top_ = 117;
    const double m_bottom_ = -116;
    const double m_upstream_ = 0;
    const double m_downstream_ = 1037;

    void setup_wirecell_fv() 
    {
        for (int idx = 0; idx <= 9; idx++) 
        {
            boundary_xy_x_array_.push_back({0, 0, 0, 0, 0, 0});
            boundary_xy_y_array_.push_back({0, 0, 0, 0, 0, 0});
            boundary_xz_x_array_.push_back({0, 0, 0, 0, 0, 0});
            boundary_xz_z_array_.push_back({0, 0, 0, 0, 0, 0});

            boundary_xy_x_array_[idx][0] = m_anode_ + boundary_dis_cut_;
            boundary_xy_x_array_[idx][1] = yx_bot_x1_array_[idx] - boundary_dis_cut_;
            boundary_xy_x_array_[idx][2] = yx_bot_x2_array_ - boundary_dis_cut_;
            boundary_xy_x_array_[idx][3] = yx_top_x2_array_ - boundary_dis_cut_;
            boundary_xy_x_array_[idx][4] = yx_top_x1_array_[idx] - boundary_dis_cut_;
            boundary_xy_x_array_[idx][5] = m_anode_ + boundary_dis_cut_;

            boundary_xy_y_array_[idx][0] = m_bottom_ + boundary_dis_cut_;
            boundary_xy_y_array_[idx][1] = yx_bot_y1_array_ + boundary_dis_cut_;
            boundary_xy_y_array_[idx][2] = yx_bot_y2_array_[idx] + boundary_dis_cut_;
            boundary_xy_y_array_[idx][3] = yx_top_y2_array_[idx] - boundary_dis_cut_;
            boundary_xy_y_array_[idx][4] = yx_top_y1_array_ - boundary_dis_cut_;
            boundary_xy_y_array_[idx][5] = m_top_ - boundary_dis_cut_;

            boundary_xz_x_array_[idx][0] = m_anode_ + boundary_dis_cut_;
            boundary_xz_x_array_[idx][1] = zx_up_x1_array_ - boundary_dis_cut_;
            boundary_xz_x_array_[idx][2] = zx_up_x2_array_ - boundary_dis_cut_;
            boundary_xz_x_array_[idx][3] = zx_dw_x2_array_ - boundary_dis_cut_;
            boundary_xz_x_array_[idx][4] = zx_dw_x1_array_[idx] - boundary_dis_cut_;
            boundary_xz_x_array_[idx][5] = m_anode_ + boundary_dis_cut_;

            boundary_xz_z_array_[idx][0] = m_upstream_ + boundary_dis_cut_ + 1;
            boundary_xz_z_array_[idx][1] = zx_up_z1_array_ + boundary_dis_cut_ + 1;
            boundary_xz_z_array_[idx][2] = zx_up_z2_array_ + boundary_dis_cut_ + 1;
            boundary_xz_z_array_[idx][3] = zx_dw_z2_array_[idx] - boundary_dis_cut_ - 1;
            boundary_xz_z_array_[idx][4] = zx_dw_z1_array_ - boundary_dis_cut_ - 1;
            boundary_xz_z_array_[idx][5] = m_downstream_ - boundary_dis_cut_ - 1;
        }
    }

    // Point-in-polygon function
    int pnpoly(const std::vector<double>& vertx, const std::vector<double>& verty, double testx, double testy) const 
    {
        int i, j, c = 0;
        for (i = 0, j = int(vertx.size()) - 1; i < int(vertx.size()); j = i++) 
        {
            if (((verty[i] > testy) != (verty[j] > testy)) &&
                (testx < (vertx[j] - vertx[i]) * (testy - verty[i]) / (verty[j] - verty[i]) + vertx[i]))
                c = !c;
        }
        return c;
    }

    bool point_inside_fv(const TVector3& position) const 
    {
        switch (version_) 
        {
            case kOldFV:
                return point_inside_old_fv(position);
            case kWholeTPC:
            case kWholeTPCPadded:
                return point_inside_whole_tpc_padded(position);
            case kWirecell:
                return point_inside_wirecell(position);
            case kWirecellPadded:
                return point_inside_wirecell_padded(position);
            default:
                return false;
        }
    }

    bool point_inside_old_fv(const TVector3& position) const 
    {
        if (position.X() > tpc_xmax_ || position.X() < tpc_xmin_) return false;
        if (position.Y() > tpc_ymax_ || position.Y() < tpc_ymin_) return false;
        if (position.Z() > tpc_zmax_ || position.Z() < tpc_zmin_) return false;
        if (position.Z() < deadz_max_ && position.Z() > deadz_min_) return false;
        return true;
    }

    bool point_inside_whole_tpc_padded(const TVector3& position) const 
    {
        if (position.X() > tpc_xmax_ - padding_ || position.X() < tpc_xmin_ + padding_) return false;
        if (position.Y() > tpc_ymax_ - padding_ || position.Y() < tpc_ymin_ + padding_) return false;
        if (position.Z() > tpc_zmax_ - padding_ || position.Z() < tpc_zmin_ + padding_) return false;
        if (position.Z() < deadz_max_ && position.Z() > deadz_min_) return false;
        return true;
    }

    bool point_inside_wirecell(const TVector3& position) const 
    {
        if (position.Z() > 1000 || position.Z() < 0) return false;
        if (position.Z() < deadz_max_ && position.Z() > deadz_min_) return false;

        int c1 = 0, c2 = 0;
        int index_y = floor((position.Y() + 116) / 24);
        int index_z = floor(position.Z() / 100);
        if (index_y < 0) index_y = 0;
        else if (index_y > 9) index_y = 9;
        if (index_z < 0) index_z = 0;
        else if (index_z > 9) index_z = 9;

        c1 = pnpoly(boundary_xy_x_array_[index_z], boundary_xy_y_array_[index_z], position.X(), position.Y());
        c2 = pnpoly(boundary_xz_x_array_[index_y], boundary_xz_z_array_[index_y], position.X(), position.Z());

        return c1 && c2;
    }

    bool point_inside_wirecell_padded(const TVector3& position) const 
    {
        return point_inside_wirecell(position) && point_inside_whole_tpc_padded(position);
    }
};

#endif