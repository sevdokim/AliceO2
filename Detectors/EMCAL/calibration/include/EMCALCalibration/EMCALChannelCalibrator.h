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

/// \class EMCALChannelCalibrator
/// \brief  Perform the EMCAL bad channel calibration
/// \author Hannah Bossi, Yale University
/// \ingroup EMCALCalib
/// \since Feb 11, 2021

#ifndef EMCAL_CHANNEL_CALIBRATOR_H_
#define EMCAL_CHANNEL_CALIBRATOR_H_

#include "EMCALCalibration/EMCALTimeCalibData.h"
#include "EMCALCalibration/EMCALChannelData.h"
#include "EMCALCalibration/EMCALCalibExtractor.h"
#include "EMCALCalibration/EMCALCalibParams.h"
#include "EMCALCalib/GainCalibrationFactors.h"
#include "DetectorsCalibration/TimeSlotCalibration.h"
#include "DetectorsCalibration/TimeSlot.h"
#include "DataFormatsEMCAL/Cell.h"
#include "EMCALBase/Geometry.h"
#include "CCDB/CcdbObjectInfo.h"
#include "EMCALCalib/CalibDB.h"
#include "DataFormatsParameters/GRPECSObject.h"

#include "Framework/Logger.h"
#include "CommonUtils/MemFileHelper.h"
#include "CCDB/CcdbApi.h"
#include "DetectorsCalibration/Utils.h"
#include <boost/histogram.hpp>
#include <boost/histogram/ostream.hpp>
#include <boost/format.hpp>

#include <array>
#include <boost/histogram.hpp>
#include <fstream>

namespace o2
{
namespace emcal
{
/// \brief class used for managment of bad channel and time calibration
/// template DataInput can be ChannelData or TimeData   // o2::emcal::EMCALChannelData, o2::emcal::EMCALTimeCalibData
template <typename DataInput, typename DataOutput>
class EMCALChannelCalibrator : public o2::calibration::TimeSlotCalibration<DataInput>
{
  using TFType = o2::calibration::TFType;
  using Slot = o2::calibration::TimeSlot<DataInput>;
  using Cell = o2::emcal::Cell;
  using CcdbObjectInfo = o2::ccdb::CcdbObjectInfo;
  using CcdbObjectInfoVector = std::vector<CcdbObjectInfo>;

 public:
  EMCALChannelCalibrator(int nb = 1000, float r = 0.35) : mNBins(nb), mRange(r)
  {
    this->setSaveDirectory(EMCALCalibParams::Instance().filePathSave);
  };

  ~EMCALChannelCalibrator() final = default;

  /// \brief Checking if all channels have enough data to do calibration.
  bool hasEnoughData(const Slot& slot) const final;
  /// \brief Initialize the vector of our output objects.
  void initOutput() final;
  void finalizeSlot(Slot& slot) final;
  o2::calibration::TimeSlot<DataInput>& emplaceNewSlot(bool front, TFType tstart, TFType tend) final;

  bool saveLastSlotData(TFile& fl) final;

  bool adoptSavedData(const o2::calibration::TimeSlotMetaData& metadata, TFile& fl) final;

  ///\brief Set the testing status.
  void setIsTest(bool isTest) { mTest = isTest; }
  bool isTest() const { return mTest; }

  const CcdbObjectInfoVector& getInfoVector() const { return mInfoVector; }
  const std::vector<DataOutput>& getOutputVector() const { return mCalibObjectVector; }

  /// \brief get if has enough data should be circumvented at EOR
  bool getSaveAtEOR() const { return mSaveAtEOR; }
  /// \brief set if has enough data should be circumvented at EOR
  void setSaveAtEOR(bool tmp) { mSaveAtEOR = tmp; }

  /// \brief get if has enough data should be circumvented at EOR
  bool getLoadAtSOR() const { return mLoadAtSOR; }
  /// \brief set if has enough data should be circumvented at EOR
  void setLoadAtSOR(bool tmp) { mLoadAtSOR = tmp; }

  /// \brief Configure the calibrator
  std::shared_ptr<EMCALCalibExtractor> getCalibExtractor() { return mCalibrator; } // return shared pointer!
  /// \brief setter for mCalibrator
  void SetCalibExtractor(std::shared_ptr<EMCALCalibExtractor> extr) { mCalibrator = extr; };

  bool setGainCalibrationFactors(o2::emcal::GainCalibrationFactors* gainCalibFactors);

  /// \brief Set current fill number
  /// \param fn fill number
  void setFillNr(int fn) { mFillNr = fn; }

  /// \brief Set current run type
  /// \param rt tun type
  void setRunType(o2::parameters::GRPECSObject::RunType rt) { mRunType = rt; }

  /// \brief Set current timestamp obtained from data
  /// \param ts timestamp in hours
  void setCurrTSInHours(int ts) { mStartTSCalib = ts; }

 private:
  int mNBins = 0;                                                                               ///< bins of the histogram for passing
  float mRange = 0.;                                                                            ///< range of the histogram for passing
  bool mTest = false;                                                                           ///< flag to be used when running in test mode: it simplify the processing (e.g. does not go through all channels)
  bool mSaveAtEOR = false;                                                                      ///< flag to pretend to have enough data in order to trigger the saving of the calib histograms for loading them at the next run
  bool mLoadAtSOR = false;                                                                      ///< flag weather to load the calibration histograms from the previous run at the SOR
  o2::parameters::GRPECSObject::RunType mRunType = o2::parameters::GRPECSObject::RunType::NONE; ///< Run type needed if previous calibration is loaded.
  int mFillNr = 0;                                                                              ///< fill nr. needed if previous calibration is loaded.
  int mStartTSCalib = -1;                                                                       ///< First timestamp for calibration. Needed to check if calibration data from previous slot should be loaded
  std::shared_ptr<EMCALCalibExtractor> mCalibrator;

  // output
  CcdbObjectInfoVector mInfoVector; // vector of CCDB Infos , each element is filled with the CCDB description of the accompanying TimeSlewing object
  std::vector<DataOutput> mCalibObjectVector;

  ClassDefOverride(EMCALChannelCalibrator, 1);
};

//_____________________________________________
template <typename DataInput, typename DataOutput>
void EMCALChannelCalibrator<DataInput, DataOutput>::initOutput()
{
  mInfoVector.clear();
  mCalibObjectVector.clear();
  std::string nameFile = "tcp.root";
  if constexpr (std::is_same<DataInput, o2::emcal::EMCALChannelData>::value) {
    nameFile = "bcm.root";
  }

  // this->setSaveFileName(nameFile);
  // mNEvents = 0;
  return;
}

//_____________________________________________
template <typename DataInput, typename DataOutput>
bool EMCALChannelCalibrator<DataInput, DataOutput>::hasEnoughData(const o2::calibration::TimeSlot<DataInput>& slot) const
{
  // in case of the end of run, we pretend to have enough data to trigger the saving of the calib objects to load them in the next run
  if (mSaveAtEOR) {
    return true;
  }
  const DataInput* c = slot.getContainer();
  return (mTest ? true : c->hasEnoughData());
}

//_____________________________________________
template <typename DataInput, typename DataOutput>
void EMCALChannelCalibrator<DataInput, DataOutput>::finalizeSlot(o2::calibration::TimeSlot<DataInput>& slot)
{

  // Extract results for the single slot
  DataInput* c = slot.getContainer();
  LOG(info) << "Finalize slot " << slot.getTFStart() << " <= TF <= " << slot.getTFEnd();
  // check if slot contains a minimum amount of data
  if (c->getNEvents() < EMCALCalibParams::Instance().minNEventsSaveSlot) {
    LOG(info) << "Slot only contains " << c->getNEvents() << " events. Not saving this slot. " << EMCALCalibParams::Instance().minNEventsSaveSlot << " required";
    return;
  }

  if constexpr (std::is_same<DataInput, o2::emcal::EMCALChannelData>::value) {
    if (c->getNEvents() < EMCALCalibParams::Instance().minNEvents_bc) {
      LOG(info) << "saving the slot with " << c->getNEvents() << " events. " << EMCALCalibParams::Instance().minNEvents_tc << " events needed for calibration";
      this->saveLastSlot();
      return;
    }
  } else if constexpr (std::is_same<DataInput, o2::emcal::EMCALTimeCalibData>::value) {
    if (c->getNEvents() < EMCALCalibParams::Instance().minNEvents_tc) {
      LOG(info) << "saving the slot with " << c->getNEvents() << " events. " << EMCALCalibParams::Instance().minNEvents_tc << " events needed for calibration";
      this->saveLastSlot();
      return;
    }
  }

  std::map<std::string, std::string> md;
  if constexpr (std::is_same<DataInput, o2::emcal::EMCALChannelData>::value) {
    LOG(debug) << "Launching the calibration.";
    o2::emcal::BadChannelMap bcm = mCalibrator->calibrateBadChannels(c->getHisto(), c->getHistoTime());
    LOG(debug) << "Done with the calibraiton";
    // for the CCDB entry
    auto clName = o2::utils::MemFileHelper::getClassName(bcm);
    auto flName = o2::ccdb::CcdbApi::generateFileName(clName);
    mInfoVector.emplace_back(CalibDB::getCDBPathBadChannelMap(), clName, flName, md, slot.getStartTimeMS(), slot.getEndTimeMS() + EMCALCalibParams::Instance().endTimeMargin, true);
    mCalibObjectVector.push_back(bcm);

    if ((EMCALCalibParams::Instance().localRootFilePath).find(".root") != std::string::npos) {
      std::ifstream ffile(EMCALCalibParams::Instance().localRootFilePath.c_str());

      TFile fLocalStorage((EMCALCalibParams::Instance().localRootFilePath).c_str(), ffile.good() == true ? "update" : "recreate");
      fLocalStorage.cd();

      TH2F* histBCMap = (TH2F*)bcm.getHistogramRepresentation();
      std::string nameBCHist = "BadChannels_" + std::to_string(slot.getStartTimeMS());
      histBCMap->Write(nameBCHist.c_str(), TObject::kOverwrite);

      TH2F hCalibHist = o2::utils::TH2FFromBoost(c->getHisto());
      std::string nameBCInputHist = "EnergyVsCellID_" + std::to_string(slot.getStartTimeMS());
      hCalibHist.Write(nameBCInputHist.c_str(), TObject::kOverwrite);

      TH2F hCalibHistTime = o2::utils::TH2FFromBoost(c->getHistoTime());
      std::string nameBCInputHistTime = "TimeVsCellID_" + std::to_string(slot.getStartTimeMS());
      hCalibHistTime.Write(nameBCInputHistTime.c_str(), TObject::kOverwrite);

      fLocalStorage.Close();
    }
  } else if constexpr (std::is_same<DataInput, o2::emcal::EMCALTimeCalibData>::value) {
    o2::emcal::TimeCalibrationParams tcd = mCalibrator->calibrateTime(c->getHisto(), EMCALCalibParams::Instance().minTimeForFit_tc, EMCALCalibParams::Instance().maxTimeForFit_tc, EMCALCalibParams::Instance().restrictFitRangeToMax_tc);

    // for the CCDB entry
    auto clName = o2::utils::MemFileHelper::getClassName(slot);
    auto flName = o2::ccdb::CcdbApi::generateFileName(clName);

    // prepareCCDBobjectInfo
    mInfoVector.emplace_back(CalibDB::getCDBPathTimeCalibrationParams(), clName, flName, md, slot.getStartTimeMS(), slot.getEndTimeMS() + EMCALCalibParams::Instance().endTimeMargin, true);
    mCalibObjectVector.push_back(tcd);

    if ((EMCALCalibParams::Instance().localRootFilePath).find(".root") != std::string::npos) {
      std::ifstream ffile(EMCALCalibParams::Instance().localRootFilePath.c_str());

      TFile fLocalStorage((EMCALCalibParams::Instance().localRootFilePath).c_str(), ffile.good() == true ? "update" : "recreate");
      fLocalStorage.cd();
      TH1F* histTCparams = (TH1F*)tcd.getHistogramRepresentation(false); // high gain calibration
      std::string nameTCHist = "TCParams_HG_" + std::to_string(slot.getStartTimeMS());
      histTCparams->Write(nameTCHist.c_str(), TObject::kOverwrite);

      TH1F* histTCparams_LG = (TH1F*)tcd.getHistogramRepresentation(true); // low gain calibration
      std::string nameTCHist_LG = "TCParams_LG_" + std::to_string(slot.getStartTimeMS());
      histTCparams_LG->Write(nameTCHist_LG.c_str(), TObject::kOverwrite);

      TH2F hCalibHist = o2::utils::TH2FFromBoost(c->getHisto());
      std::string nameTCInputHist = "TimeVsCellID_" + std::to_string(slot.getStartTimeMS());
      hCalibHist.Write(nameTCInputHist.c_str(), TObject::kOverwrite);
      fLocalStorage.Close();
    }
  }
}

template <typename DataInput, typename DataOutput>
o2::calibration::TimeSlot<DataInput>& EMCALChannelCalibrator<DataInput, DataOutput>::emplaceNewSlot(bool front, TFType tstart, TFType tend)
{
  auto& cont = o2::calibration::TimeSlotCalibration<DataInput>::getSlots();
  auto& slot = front ? cont.emplace_front(tstart, tend) : cont.emplace_back(tstart, tend);
  slot.setContainer(std::make_unique<DataInput>());
  return slot;
}

/// \brief Write histograms for energy and time vs cell ID to file
/// \param fl file that we write the histograms to
template <typename DataInput, typename DataOutput>
bool EMCALChannelCalibrator<DataInput, DataOutput>::saveLastSlotData(TFile& fl)
{
  LOG(info) << "EMC calib histos are saved in " << fl.GetName();
  // we only have 1 slot
  auto& cont = o2::calibration::TimeSlotCalibration<DataInput>::getSlots();
  auto& slot = cont.at(0);
  DataInput* c = slot.getContainer();

  // timestamp in hours
  int timeNow = static_cast<int>(o2::ccdb::getCurrentTimestamp() / o2::ccdb::CcdbObjectInfo::HOUR);

  if constexpr (std::is_same<DataInput, o2::emcal::EMCALChannelData>::value) {
    auto hist = c->getHisto();
    auto histTime = c->getHistoTime();

    TH2F hEnergy = o2::utils::TH2FFromBoost(hist);
    TH2F hTime = o2::utils::TH2FFromBoost(histTime, "histTime");
    TH1D hNEvents("hNEvents", "hNEvents", 1, 0, 1);
    hNEvents.SetBinContent(1, c->getNEvents());
    TH1I hGlobalProperties("hGlobalProperties", "hGlobalProperties", 3, -0.5, 2.5);
    hGlobalProperties.GetXaxis()->SetBinLabel(1, "Fill nr.");
    hGlobalProperties.GetXaxis()->SetBinLabel(2, "run type");
    hGlobalProperties.GetXaxis()->SetBinLabel(3, "ts in hours");
    hGlobalProperties.SetBinContent(1, mFillNr);
    hGlobalProperties.SetBinContent(2, mRunType);
    hGlobalProperties.SetBinContent(3, timeNow);

    fl.cd();
    hEnergy.Write("EnergyVsCellID");
    hTime.Write("TimeVsCellID");
    hNEvents.Write("NEvents");
    hGlobalProperties.Write("GlobalProperties");
  } else if constexpr (std::is_same<DataInput, o2::emcal::EMCALTimeCalibData>::value) {
    auto histTime = c->getHisto();
    TH2F hTime = o2::utils::TH2FFromBoost(histTime);
    TH1D hNEvents("hNEvents", "hNEvents", 1, 0, 1);
    hNEvents.SetBinContent(1, c->getNEvents());
    TH1I hGlobalProperties("hGlobalProperties", "hGlobalProperties", 3, -0.5, 2.5);
    hGlobalProperties.GetXaxis()->SetBinLabel(1, "Fill nr.");
    hGlobalProperties.GetXaxis()->SetBinLabel(2, "run type");
    hGlobalProperties.GetXaxis()->SetBinLabel(3, "ts in hours");
    hGlobalProperties.SetBinContent(1, mFillNr);
    hGlobalProperties.SetBinContent(2, mRunType);
    hGlobalProperties.SetBinContent(3, timeNow);

    fl.cd();
    hTime.Write("TimeVsCellID");
    hNEvents.Write("NEvents");
    hGlobalProperties.Write("GlobalProperties");
  }

  return true;
}

/// \brief Read histograms for energy and time vs cell ID to file
/// \param metadata metadata description of the data
/// \param fl file that we write the histograms to
template <typename DataInput, typename DataOutput>
bool EMCALChannelCalibrator<DataInput, DataOutput>::adoptSavedData(const o2::calibration::TimeSlotMetaData& metadata, TFile& fl)
{
  LOG(info) << "Loading data from previous run";

  if (!this->getSavedSlotAllowed() || !this->getLoadAtSOR())
    return true;

  auto& cont = o2::calibration::TimeSlotCalibration<DataInput>::getSlots();

  if (cont.size() == 0) {
    LOG(warning) << "cont.size() is 0, calibration objects from previous run cannot be loaded...";
    return true;
  }
  auto& slot = cont.at(0);
  DataInput* c = slot.getContainer();

  // check run type and fill
  TH1I* hGlobalProperties = (TH1I*)fl.Get("GlobalProperties");
  if (!hGlobalProperties) {
    LOG(error) << "GlobalProperties histogram not found. Will not load previous calibration histograms";
  } else {
    int fillNr = hGlobalProperties->GetBinContent(1);
    int runType = hGlobalProperties->GetBinContent(2);
    int tsOld = hGlobalProperties->GetBinContent(3);
    int tsDiff = (mStartTSCalib > 0 ? mStartTSCalib : static_cast<int>(o2::ccdb::getCurrentTimestamp() / o2::ccdb::CcdbObjectInfo::HOUR)) - tsOld; // get current timestamp if mStartTSCalib is not set
    LOG(debug) << "tsOld " << tsOld << "   tsNow " << (mStartTSCalib > 0 ? mStartTSCalib : static_cast<int>(o2::ccdb::getCurrentTimestamp() / o2::ccdb::CcdbObjectInfo::HOUR)) << "   tsDiff " << tsDiff;

    if (EMCALCalibParams::Instance().requireSameRunType && runType != static_cast<int>(mRunType)) {
      LOG(info) << "adoptSavedData: Same run type required but run types differ: " << runType << " != " << static_cast<int>(mRunType);
      return false;
    }
    if (EMCALCalibParams::Instance().requireSameFill && fillNr != mFillNr) {
      LOG(info) << "adoptSavedData: Same fill nr. required but fills differ: " << fillNr << " != " << mFillNr;
      return false;
    }
    if (EMCALCalibParams::Instance().tsDiffMax > 0 && (EMCALCalibParams::Instance().tsDiffMax < tsDiff || tsDiff < 0)) {
      LOG(info) << "adoptSavedData: Maximum difference in ts is: " << EMCALCalibParams::Instance().tsDiffMax << " but " << tsDiff << " is given";
      return false;
    }
  }

  if constexpr (std::is_same<DataInput, o2::emcal::EMCALChannelData>::value) {
    TH2D* hEnergy = (TH2D*)fl.Get("EnergyVsCellID");
    TH2D* hTime = (TH2D*)fl.Get("TimeVsCellID");
    if (!hEnergy || !hTime) {
      return false;
    }
    auto hEnergyBoost = o2::utils::boostHistoFromRoot_2D<boostHisto2d>(hEnergy);
    auto hTimeBoost = o2::utils::boostHistoFromRoot_2D<boostHisto2d>(hTime);

    c->setHisto(hEnergyBoost);
    c->setHistoTime(hTimeBoost);

  } else if constexpr (std::is_same<DataInput, o2::emcal::EMCALTimeCalibData>::value) {
    TH2D* hTime = (TH2D*)fl.Get("TimeVsCellID");
    if (!hTime) {
      return false;
    }
    auto hTimeBoost = o2::utils::boostHistoFromRoot_2D<boostHisto2d>(hTime);

    c->setHisto(hTimeBoost);
  }
  TH1D* hEvents = (TH1D*)fl.Get("NEvents");
  if (!hEvents) {
    return false;
  }
  c->setNEvents(hEvents->GetBinContent(1));
  LOG(info) << "successfully loaded histograms with " << hEvents->GetBinContent(1) << " events";

  return true;
}

template <typename DataInput, typename DataOutput>
bool EMCALChannelCalibrator<DataInput, DataOutput>::setGainCalibrationFactors(o2::emcal::GainCalibrationFactors* gainCalibFactors)
{

  auto& cont = o2::calibration::TimeSlotCalibration<DataInput>::getSlots();
  if (cont.size() == 0) {
    return false; // time slot object not yet there
  }

  auto& slot = cont.at(0);
  DataInput* c = slot.getContainer();
  c->setGainCalibFactors(gainCalibFactors);

  return true;
}

} // end namespace emcal
} // end namespace o2

#endif /*EMCAL_CHANNEL_CALIBRATOR_H_ */
