#ifndef CONSTANTS_HH
#define CONSTANTS_HH

constexpr float BOGUS = 9999.;
constexpr int BOGUS_INT = 9999;
constexpr int BOGUS_INDEX = -1;
constexpr float LOW_FLOAT = -1e30;
constexpr float DEFAULT_WEIGHT = 1.;

constexpr int CHARGED_CURRENT = 0;
constexpr int NEUTRAL_CURRENT = 1;

constexpr int ELECTRON_NEUTRINO = 12;
constexpr int MUON = 13;
constexpr int MUON_NEUTRINO = 14;
constexpr int TAU_NEUTRINO = 16;
constexpr int PROTON = 2212;
constexpr int PI_ZERO = 111;
constexpr int PI_PLUS = 211;

constexpr float DEFAULT_PROTON_PID_CUT = 0.2;
constexpr float LEAD_P_MIN_MOM_CUT = 0.250; // GeV/c
constexpr float LEAD_P_MAX_MOM_CUT = 1.; // GeV/c
constexpr float MUON_P_MIN_MOM_CUT = 0.100; // GeV/c
constexpr float MUON_P_MAX_MOM_CUT = 1.200; // GeV/c
constexpr float CHARGED_PI_MOM_CUT = 0.; // GeV/c
constexpr float MUON_MOM_QUALITY_CUT = 0.25; // fractional difference

constexpr float TOPO_SCORE_CUT = 0.1;
constexpr float COSMIC_IP_CUT = 10.; // cm

constexpr float MUON_TRACK_SCORE_CUT = 0.8;
constexpr float MUON_VTX_DISTANCE_CUT = 4.; // cm
constexpr float MUON_LENGTH_CUT = 10.; // cm
constexpr float MUON_PID_CUT = 0.2;

constexpr float TRACK_SCORE_CUT = 0.5;

constexpr double PCV_X_MIN = 10.;
constexpr double PCV_X_MAX = 246.35;

constexpr double PCV_Y_MIN = -106.5;
constexpr double PCV_Y_MAX = 106.5;

constexpr double PCV_Z_MIN = 10.;
constexpr double PCV_Z_MAX = 1026.8;

constexpr double TARGET_MASS = 37.215526; // 40Ar, GeV
constexpr double NEUTRON_MASS = 0.93956541; // GeV
constexpr double PROTON_MASS = 0.93827208; // GeV
constexpr double MUON_MASS = 0.10565837; // GeV
constexpr double PI_PLUS_MASS = 0.13957000; // GeV
constexpr double BINDING_ENERGY = 0.02478; // 40Ar, GeV

#endif // CONSTANTS_HH