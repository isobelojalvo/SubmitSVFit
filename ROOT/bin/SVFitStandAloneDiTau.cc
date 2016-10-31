#include "PhysicsTools/FWLite/interface/CommandLineParser.h" 
#include "TFile.h"
#include "TROOT.h"
#include "TLorentzVector.h"
#include "TKey.h"
#include "TTree.h"
#include "TH1F.h"
#include "TF1.h"
#include <math.h> 
#include "TMath.h" 
#include <limits>
#include "TSystem.h"

#include "FWCore/ParameterSet/interface/FileInPath.h"

#include "HTT-utilities/RecoilCorrections/interface/RecoilCorrector.h"
#include "TauAnalysis/SVfitStandalone/interface/SVfitStandaloneAlgorithm.h"

#include "TFile.h"
#include "TTree.h"
#include "TH1.h"

void copyFiles( optutl::CommandLineParser parser, TFile* fOld, TFile* fNew) ;
void readdir(TDirectory *dir, optutl::CommandLineParser parser, char TreeToUse[]) ;
//void copyFiles( optutl::CommandLineParser parser, TFile* fOld, TFile* fNew);
void CopyFile(const char *fname, optutl::CommandLineParser parser);
void CopyDir(TDirectory *source,optutl::CommandLineParser parser);
void runSVFit(std::vector<svFitStandalone::MeasuredTauLepton> & measuredTauLeptons, TFile * inputFile_visPtResolution, double measuredMETx, double measuredMETy, TMatrixD &covMET, float num, float &svFitMass, float& svFitPt, float &svFitEta, float &svFitPhi, float &svFitMET, float &svFitTransverseMass);

int main (int argc, char* argv[]) 
{
   optutl::CommandLineParser parser ("Sets Event Weights in the ntuple");
   parser.addOption("branch",optutl::CommandLineParser::kString,"Branch","svFit");
   parser.addOption("newFile",optutl::CommandLineParser::kString,"newFile","newFile");
   parser.addOption("newOutputFile",optutl::CommandLineParser::kDouble,"New Output File",0.0);
   parser.addOption("isWJets",optutl::CommandLineParser::kDouble,"isWJets",0.0);
   //parser.addOption("--newOutputFile",optutl::CommandLineParser::kBool,"NewOutputFile",true);
   parser.parseArguments (argc, argv);
   
   char TreeToUse[80]="first" ;

   TFile *fProduce;

   TFile *f = new TFile(parser.stringValue("outputFile").c_str(),"UPDATE");

   //if(parser.boolValue("--newOutputFile")){
   if(parser.doubleValue("newOutputFile")>0){
     std::cout<<"Creating new outputfile"<<std::endl;
     std::string newFileName = parser.stringValue("newFile");
     fProduce = new TFile(newFileName.c_str(),"RECREATE");
     std::cout<<"copying filese"<<std::endl;
     copyFiles(parser, f, fProduce);
     std::cout<<"close f"<<std::endl;
     fProduce->Close();
     f->Close();
     std::cout<<"reopen file"<<std::endl;
     fProduce = new TFile(newFileName.c_str(),"UPDATE");
     std::cout<<"listing the directories================="<<std::endl;
     fProduce->ls();
     readdir(fProduce,parser,TreeToUse);

     fProduce->Close();
   }
   else{
     readdir(f,parser,TreeToUse);
     f->Close();
   }
   std::cout<<"Closing=============================="<<std::endl;



} 


void readdir(TDirectory *dir, optutl::CommandLineParser parser, char TreeToUse[]) 
{
  std::string recoilFileName = "HTT-utilities/RecoilCorrections/data/TypeIPFMET_2016BCD.root";
  int j = 0;
  TDirectory *dirsav = gDirectory;
  TIter next(dir->GetListOfKeys());
  TKey *key;
  char stringA[80]="first";
  edm::FileInPath inputFileName_visPtResolution("TauAnalysis/SVfitStandalone/data/svFitVisMassAndPtResolutionPDF.root");
  TH1::AddDirectory(false);  
  TFile* inputFile_visPtResolution = new TFile(inputFileName_visPtResolution.fullPath().data());  
  std::string previousKey = "";
  std::cout<<"reading dir"<<std::endl;
  while ((key = (TKey*)next())) {
    std::cout<<"reading keys"<<std::endl;
    printf("Found key=%s \n",key->GetName());
    previousKey = key->GetName();
    if(previousKey != "eventTree" && previousKey != "diTauEventTree" && previousKey != "diTauEventTreeFinal"){
      std::cout<<"previous key is "<<previousKey<< " continuing"<<std::endl;
      continue;
    }

    TObject *obj = key->ReadObj();
    
    if (obj->IsA()->InheritsFrom(TDirectory::Class())) {
      dir->cd(key->GetName());
      TDirectory *subdir = gDirectory;
      sprintf(TreeToUse,"%s",key->GetName());
      readdir(subdir,parser,TreeToUse);
      dirsav->cd();
    }
    else if(obj->IsA()->InheritsFrom(TTree::Class())) {
      std::cout<<"Object inherits from TTree Class Tree to use: "<<TreeToUse<<std::endl;
      
      TTree *t = (TTree*)obj;

      float svFitMass = -1;
      float svFitMass2 = -1;
      TBranch *newBranch = t->Branch(parser.stringValue("branch").c_str(),&svFitMass2,(parser.stringValue("branch")+"/F").c_str());
      float pt1, eta1, phi1, decayMode1;
      float pt2, eta2, phi2, decayMode2;
      float mvacov00, mvacov10, mvacov01, mvacov11;
      float genPx, genPy, visPx, visPy;
      float metcorr_ex, metcorr_ey;
      int recoilNJets;

      // define MET
      double measuredMETx, measuredMETy;
      float met, metphi;
      TLorentzVector TMet(0,0,0,0);

      // define MET covariance
      TMatrixD covMET(2, 2);

      //Tau variables
      svFitStandalone::kDecayType decayType1 = svFitStandalone::kTauToHadDecay; //svFitStandalone::kTauToElecDecay
      svFitStandalone::kDecayType decayType2 = svFitStandalone::kTauToHadDecay; //svFitStandalone::kTauToElecDecay
      float mass1, mass2;

      if(std::string(TreeToUse).find("diTauEvent")== std::string::npos){
	std::cout<<"TreeToUse "<< std::string(TreeToUse)<<" does not match diTauEvent or diTauEvent... Skipping!!"<<std::endl;
	continue;
      }
      
      //MET, MET Covariance, lepton objects,
      t->SetBranchAddress("pt_1",&pt1);
      t->SetBranchAddress("eta_1",&eta1);
      t->SetBranchAddress("phi_1",&phi1);
      t->SetBranchAddress("m_1",&mass1);
      t->SetBranchAddress("pt_2",&pt2);
      t->SetBranchAddress("eta_2",&eta2);
      t->SetBranchAddress("phi_2",&phi2);
      t->SetBranchAddress("m_2",&mass2);
      t->SetBranchAddress("decayMode_1",&decayMode1);
      t->SetBranchAddress("decayMode_2",&decayMode2);
      t->SetBranchAddress("metcov00",&mvacov00);
      t->SetBranchAddress("metcov01",&mvacov01);
      t->SetBranchAddress("metcov10",&mvacov10);
      t->SetBranchAddress("metcov11",&mvacov11);
      t->SetBranchAddress("met",&met);
      t->SetBranchAddress("metphi",&metphi);
      t->SetBranchAddress("njetspt20",&recoilNJets);
      t->SetBranchAddress("genpX",&genPx);
      t->SetBranchAddress("genpY",&genPy);
      t->SetBranchAddress("vispX",&visPx);
      t->SetBranchAddress("vispY",&visPy);

      if(parser.doubleValue("isWJets")>0) {
        recoilNJets = recoilNJets + 1;
        std::cout << "recoilNJets: " << recoilNJets << std::endl;
      }

      RecoilCorrector* recoilCorrector = new RecoilCorrector(recoilFileName);
      
      printf("Found tree -> weighting\n");
      for(Int_t i=0;i<t->GetEntries();++i)
	{
	  //if(j>1)break ;
	  t->GetEntry(i);
	  
	  TMet.SetPtEtaPhiM(met,0,metphi,0);
	  
	  measuredMETx = met*TMath::Cos(metphi);
	  measuredMETy = met*TMath::Sin(metphi);

	  covMET[0][0] =  mvacov00;
	  covMET[1][0] =  mvacov10;
	  covMET[0][1] =  mvacov01;
	  covMET[1][1] =  mvacov11;

	  int recoilType = 1;
	  if(recoilType != 0){
	    recoilCorrector->Correct(
				     measuredMETx, // uncorrected mva met px (float)
				     measuredMETy, // uncorrected mva met py (float)
				     genPx, // generator Z/W/Higgs px (float)
				     genPy, // generator Z/W/Higgs py (float)
				     visPx, // generator visible Z/W/Higgs px (float)
				     visPy, // generator visible Z/W/Higgs py (float)
				     recoilNJets,  // number of jets (hadronic jet multiplicity) (int)
				     metcorr_ex, // corrected met px (float)
				     metcorr_ey  // corrected met py (float)
				     );
	    std::cout << " - MEASURED:  met_ex: " << measuredMETx << "  met_ey: " << measuredMETy << std::endl;
	    std::cout << " - CORRECTED: met_ex: " << metcorr_ex << "  met_ey: " << metcorr_ey << std::endl;
	  }
	  else{
	    metcorr_ex = measuredMETx;
	    metcorr_ey = measuredMETy;
	  }
	  
	  float metcor, metcorphi;
	  metcor = TMath::Sqrt( metcorr_ex*metcorr_ex + metcorr_ey*metcorr_ey);
	  metcorphi = TMath::ATan2( metcorr_ey, metcorr_ex );
	  std::cout << " - metcor "<<metcor<<" metcorphi "<<metcorphi<<std::endl;


	  //covMET[0][0] =  787.352;
	  //covMET[1][0] = -178.63;
	  //covMET[0][1] = -178.63;
	  //covMET[1][1] =  179.545;

	  // define lepton four vectors
	  std::vector<svFitStandalone::MeasuredTauLepton> measuredTauLeptons;
	  // check if electron or muon
	  measuredTauLeptons.push_back(
				       svFitStandalone::MeasuredTauLepton(decayType1, pt1, eta1,  phi1, mass1, decayMode1)
				       ); 
	  
	  measuredTauLeptons.push_back(
				       svFitStandalone::MeasuredTauLepton(decayType2,  pt2, eta2, phi2,  mass2, decayMode2)
				       ); 

	  float svFitPt, svFitEta, svFitPhi, svFitMET, svFitTransverseMass;
	  //std::cout<<"pt1,eta1,phi1,mass1: "<<pt1<<","<<eta1<<","<<phi1<<","<<mass1<<","<<decayMode1<<std::endl;
	  //std::cout<<"pt2,eta2,phi2,mass2: "<<pt2<<","<<eta2<<","<<phi2<<","<<mass2<<","<<decayMode2<<std::endl;
	  //std::cout<<"measuredMETx,measuredMETy: "<<measuredMETx<<","<<measuredMETy<<std::endl;
	  //std::cout<<"cov00,cov10,cov01,cov11: "<<mvacov00<<","<<mvacov10<<","<<mvacov01<<","<<mvacov11<<std::endl;
	  runSVFit(measuredTauLeptons, inputFile_visPtResolution, metcorr_ex, metcorr_ey, covMET, 0, svFitMass, svFitPt, svFitEta, svFitPhi, svFitMET, svFitTransverseMass);

	  svFitMass2 = svFitMass;
	  newBranch->Fill();
	}
      delete inputFile_visPtResolution;
      dir->cd();      
      t->Write("",TObject::kOverwrite);
      strcpy(TreeToUse,stringA) ;
    }
  }
}

void runSVFit(std::vector<svFitStandalone::MeasuredTauLepton> & measuredTauLeptons, TFile * inputFile_visPtResolution, double measuredMETx, double measuredMETy, TMatrixD &covMET, float num, float &svFitMass, float& svFitPt, float &svFitEta, float &svFitPhi, float &svFitMET, float &svFitTransverseMass){

  //edm::FileInPath inputFileName_visPtResolution("TauAnalysis/SVfitStandalone/data/svFitVisMassAndPtResolutionPDF.root");
  //TH1::AddDirectory(false);  
  //TFile* inputFile_visPtResolution = new TFile(inputFileName_visPtResolution.fullPath().data());  
  //float svFitMass = 0;
  SVfitStandaloneAlgorithm algo(measuredTauLeptons, measuredMETx, measuredMETy, covMET, 0);
  algo.addLogM(false);  
  algo.shiftVisPt(true, inputFile_visPtResolution);
  algo.integrateMarkovChain();
  svFitMass = algo.getMass(); // return value is in units of GeV
  svFitPt = algo.pt();
  svFitEta = algo.eta();
  svFitPhi = algo.phi();
  svFitMET = algo.fittedMET().Rho();
  svFitTransverseMass = algo.transverseMass();
  if ( algo.isValidSolution() ) {
    std::cout << "found mass = " << svFitMass << std::endl;
  } else {
    std::cout << "sorry -- status of NLL is not valid [" << algo.isValidSolution() << "]" << std::endl;
  }
  //delete inputFile_visPtResolution;

}


//Thank you Renee Brun :)
void CopyDir(TDirectory *source, optutl::CommandLineParser parser) {
  //copy all objects and subdirs of directory source as a subdir of the current directory
  source->ls();
  TDirectory *savdir = gDirectory;
  std::cout<<"source getname "<<source->GetName()<<std::endl;
  TDirectory *adir = savdir; 
  if(source->GetName()!=parser.stringValue("outputFile")){
    adir = savdir->mkdir(source->GetName());
    std::cout<<"Source name is not outputfile name"<<std::endl;
    adir->cd();
  }

  //loop on all entries of this directory
  TKey *key;
  TIter nextkey(source->GetListOfKeys());
  while ((key = (TKey*)nextkey())) {
    const char *classname = key->GetClassName();
    TClass *cl = gROOT->GetClass(classname);
    if (!cl) continue;
    if (cl->InheritsFrom(TDirectory::Class())) {
      source->cd(key->GetName());
      TDirectory *subdir = gDirectory;
      adir->cd();
      CopyDir(subdir,parser);
      adir->cd();
    } else if (cl->InheritsFrom(TTree::Class())) {
      TTree *T = (TTree*)source->Get(key->GetName());
      adir->cd();
      TTree *newT = T->CloneTree(-1,"fast");
      newT->Write();
    } else {
      source->cd();
      TObject *obj = key->ReadObj();
      adir->cd();
      obj->Write();
      delete obj;
    }
  }
  adir->SaveSelf(kTRUE);
  savdir->cd();
}
void CopyFile(const char *fname, optutl::CommandLineParser parser) {
  //Copy all objects and subdirs of file fname as a subdir of the current directory
  TDirectory *target = gDirectory;
  TFile *f = TFile::Open(fname);
  if (!f || f->IsZombie()) {
    printf("Cannot copy file: %s\n",fname);
    target->cd();
    return;
  }
  target->cd();
  CopyDir(f,parser);
  delete f;
  target->cd();
}
void copyFiles( optutl::CommandLineParser parser, TFile* fOld, TFile* fNew) 
{
  //prepare files to be copied
  if(gSystem->AccessPathName(parser.stringValue("outputFile").c_str())) {
    gSystem->CopyFile("hsimple.root", parser.stringValue("outputFile").c_str());
  }
  //main function copying 4 files as subdirectories of a new file
  //std::string newFileName = parser.stringValue("newFile");
  //fNew = new TFile(newFileName.c_str(),"recreate");
  fNew->cd();
  CopyFile(parser.stringValue("outputFile").c_str(),parser);
  //fNew->ls();

}
