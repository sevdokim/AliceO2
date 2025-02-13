// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#ifndef O2_TRD_TRACKLETTRANSFORMER_H
#define O2_TRD_TRACKLETTRANSFORMER_H

#include "TRDBase/Geometry.h"
#include "DataFormatsTRD/Tracklet64.h"
#include "DataFormatsTRD/CalibratedTracklet.h"
#include "DataFormatsTRD/CalVdriftExB.h"

namespace o2
{
namespace trd
{

class TrackletTransformer
{
 public:
  TrackletTransformer() = default;
  ~TrackletTransformer() = default;

  void init();

  void setCalVdriftExB(const CalVdriftExB* cal) { mCalVdriftExB = cal; };
  void setApplyXOR() { mApplyXOR = true; }
  void setApplyShift(bool f) { mApplyShift = f; }
  bool isShiftApplied() const { return mApplyShift; }

  float calculateZ(int padrow, const PadPlane* padPlane) const;

  float calculateDy(int hcid, int slope, const PadPlane* padPlane) const;

  float calibrateX(double x) const;

  std::array<float, 3> transformL2T(int hcid, std::array<double, 3> spacePoint) const;

  CalibratedTracklet transformTracklet(Tracklet64 tracklet, bool trackingFrame = true) const;

  double getTimebin(int detector, double x) const;

 private:
  Geometry* mGeo{nullptr};
  bool mApplyXOR{false};
  bool mApplyShift{true};

  float mXAnode;

  const CalVdriftExB* mCalVdriftExB{nullptr};
};

} // namespace trd
} // namespace o2

#endif
