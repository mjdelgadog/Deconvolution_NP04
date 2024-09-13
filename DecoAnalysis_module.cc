//===========================================================================
// DecoAnalysis_module.cc
// This analyzer module for Deconvolution creates histograms
// with information from OpDetWaveforms and OpWaveforms.
// @authors     : Daniele Guffanti, Maritza Delgado, Sergio Manthey Corchado
// @created     : 2022 
//=========================================================================== 

#ifndef DecoAnalysis_h
#define DecoAnalysis_h
 
// LArSoft includes
#include "larcore/CoreUtils/ServiceUtil.h"
#include "larcore/Geometry/Geometry.h"
#include "lardataobj/RawData/OpDetWaveform.h"
#include "lardataobj/RecoBase/OpHit.h"
#include "lardataobj/RecoBase/OpWaveform.h"
#include "lardata/DetectorInfoServices/DetectorClocksService.h"

// Framework includes
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Principal/Handle.h"
#include "canvas/Persistency/Common/Ptr.h"
#include "canvas/Persistency/Common/PtrVector.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art_root_io/TFileService.h"
#include "art_root_io/TFileDirectory.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
  
// ROOT includes
#include "TH1.h"
#include "THStack.h"
#include "TF1.h"
#include "TLorentzVector.h"
#include "TVector3.h"
#include "TTree.h"
  
// C++ Includes
#include <map>
#include <vector>
#include <iostream>
#include <cstring>
#include <sstream>
#include "math.h"
#include <climits>

namespace opdet {
  class DecoAnalysis : public art::EDAnalyzer{
    public:
      // Standard constructor and destructor for an ART module.
      DecoAnalysis(const fhicl::ParameterSet&);
      virtual ~DecoAnalysis();
      // The analyzer routine, called once per event. 
      void analyze (const art::Event&); 

    private:
      // The parameters we'll read from the .fcl file.
      std::string fInputModuleDeco;          // Input tag for OpDet collection
      std::string fInputModuleDigi;          // Input tag for OpDet collection
      std::string fInstanceName;             // Input tag for OpDet collection
      double fSampleFreq;                    // in MHz
    //float fTimeBegin;                      // in us
    // float fTimeEnd;                        // in us
    std::vector<int> channel_numbers = {4,14,24,34,40,42,45,46,47,49,50,52,55,56,57,59,60,62, 65,66,67,68,69,70,71,84,85,86,87,88,89,90,91,92,93,94,95,114,115,116,117,118,119,138,139,140,141,142,143,144,145,146,147,148,149}; 
    size_t fChNumber; //channel number
    //int fChNumber; //channel number
    std::vector<int>fFBkChannel = {4,14,24,34,40,42,45,46,47,49,50,52,55,56,57,59,60,62, 65,66}; // (all set in fcl)
    std::vector<int>fHPKChannel = {0, 1,2,3,5,6,7,8,9,10,11,12,13,15,16,17,18,19,20,21,22,23}; // (all set in fcl)

  };  
} 

#endif 
namespace opdet {
  DEFINE_ART_MODULE(DecoAnalysis)
}
  
namespace opdet {
  //-----------------------------------------------------------------------
  // Constructor
  DecoAnalysis::DecoAnalysis(fhicl::ParameterSet const& pset)
  : EDAnalyzer(pset){
    // Read the fcl-file
    fInputModuleDeco  =   pset.get< std::string >("InputModuleDeco");
    fInputModuleDigi  =   pset.get< std::string >("InputModuleDigi","daq");
    fInstanceName     =   pset.get< std::string >("InstanceName");
    fFBkChannel       =   pset.get<std::vector<int> >("FBkChannel");
    fHPKChannel       =   pset.get<std::vector<int> >("HPKChannel");
    
    // Obtain parameters from DetectorClocksService
    auto const clockData = art::ServiceHandle<detinfo::DetectorClocksService const>()->DataForJob();     
    fSampleFreq = clockData.OpticalClock().Frequency();

    art::ServiceHandle< art::TFileService > tfs;
  }
  
  //-----------------------------------------------------------------------
  // Destructor
  DecoAnalysis::~DecoAnalysis() {  
  }
    
  //-----------------------------------------------------------------------
  void DecoAnalysis::analyze(const art::Event& evt){ 
    // Map to store how many waveforms are on one optical channel
    std::map< int, int > mapChannelWF;
  
    // Get deconvolved "OpWaveforms" from the event
    art::Handle< std::vector< recob::OpWaveform >> deconv;
    evt.getByLabel(fInputModuleDeco, deconv);
    std::vector< recob::OpWaveform > OpWaveform;
    for (auto const& wf : *deconv) {OpWaveform.push_back(wf);}
    
    // Get Waveforms "OpDetWaveforms" from the event
    art::Handle< std::vector< raw::OpDetWaveform >> digi;
    evt.getByLabel(fInputModuleDigi, fInstanceName, digi);
    std::vector< raw::OpDetWaveform > OpDetWaveform;

     
    for (auto const& wf : *digi) {OpDetWaveform.push_back(wf);}
    
    // Access ART's TFileService, which will handle creating and writing histograms for us
    art::ServiceHandle< art::TFileService > tfs;
    
    double firstWaveformTime = -9999;
    for (auto const& waveform : *digi)
    {
      if (firstWaveformTime < 0) firstWaveformTime = waveform.TimeStamp();
    }
   
    for (size_t i = 0; i < fFBkChannel.size(); i++){
      raw::OpDetWaveform waveform = OpDetWaveform.at(i);
      recob::OpWaveform decowaveform = OpWaveform.at(i);

     //fChNumber= decowaveform.Channel();
     fChNumber= fFBkChannel.at(i);
        
      std::cout << "deco_ch" <<  decowaveform.Channel() << std::endl;
      std::cout << "Channelfc" << fChNumber << std::endl;
      
      // Implement different end time for waveforms of variable length
      double startTime = waveform.TimeStamp() - firstWaveformTime;
      double endTime = double(waveform.size())/fSampleFreq + startTime;

      // Make a name for the histogram
      std::stringstream histName;
      histName << "event_"      << evt.id().event() 
              << "_opchannel_" << fChNumber
              << "_decowaveform_"  << mapChannelWF[fChNumber];     //improve, this changes the name of the channel         
      // Increase counter for number of waveforms on this optical channel
       mapChannelWF[fChNumber]++;

      // Create a new histogram
      TH1D *decowaveformHist = tfs->make< TH1D >(histName.str().c_str(),  TString::Format(";t - %f (#mus);",firstWaveformTime), decowaveform.NSignal(), startTime, endTime);
      // Copy values from the waveform into the histogram
      for(size_t tick = 0; tick < decowaveform.NSignal(); tick++){
        // Fill histogram with waveform                        
        decowaveformHist->SetBinContent(tick+1, decowaveform.Signal()[tick]);
      }    
      // }   
    }                                      
  } 
} // namespace opdet
