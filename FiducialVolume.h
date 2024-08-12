#pragma once
#include "TVector3.h"

double FV_X_MIN =   21.5;
double FV_X_MAX =  234.85;

double FV_Y_MIN = -95.0;
double FV_Y_MAX =  95.0;

double FV_Z_MIN =   21.5;
double FV_Z_MAX =  966.8;

template <typename Number> bool point_inside_FV( Number x, Number y, Number z )
{
    bool x_inside_FV = ( FV_X_MIN < x ) && ( x < FV_X_MAX );
    bool y_inside_FV = ( FV_Y_MIN < y ) && ( y < FV_Y_MAX );
    bool z_inside_FV = ( FV_Z_MIN < z ) && ( z < FV_Z_MAX );
    
    return ( x_inside_FV && y_inside_FV && z_inside_FV );
}

inline bool point_inside_FV( const TVector3& pos ) {
    return point_inside_FV( pos.X(), pos.Y(), pos.Z() );
}