# SubmitSVFit
```
cmsrel CMSSW_9_4_11 
cd CMSSW_9_4_11/src/
cmsenv
git clone -b 2018_devel https://github.com/isobelojalvo/SubmitSVFit.git
cd SubmitSVFit
source recipe.sh
scram b -j 8
```
To run in interactive mode for example:
```
FastMTTStandAlonePUATauDM inputfile=tauDATA.root newFile=tauDATANew.root new  doES=0 metType=-1 recoilType=2 
```

 - inputFile = the input file (tauDATA.root)
 - newFile = name of output file, default is newFile.root if none specified
 - doES = apply energy scale adjustments providing nominal, shift UP and shift DOWN values of svFit
   - 0 = default, no shift
   - 1 = apply shifts
 - isWJets = this shifts the number of jets used in recoil corrections, it is critical for
WJets samples because we clear our jets to preven overlapping with out leptons, but
with WJets one of the leptons is a jet
   - 0 = non-WJets samples
   - 1 = WJets sample
 - metType = MVA MET vs. PF MET
   - 1 = Mva Met
   - -1 = PF Met

To submit jobs to farmout job:
```
cd test
python svFitSubmitterFast.py -sd /hdfs/store/user/ojalvo/${jobIDMC}_ggH125_ext-SUB_MC          -iswj=0 -es=1 -mt=1 -r=2  --jobName ${svfitJobID}
OR
bash submitMyJobsAll-2018.sh
```

 - -dr = dryRun and outputs a command for you to run
 - -sd = select directory, the input directory, this will produce a list of all files in that directory to run over<BR>
       you must have your files in a /hdfs directory.
 - -es = apply energy scale, see above
 - --jobName = applys a job name to append to your file names and output directory structure
 - -iswj = isWJets
 - -mt = metType


To get your files from elsewhere to /hdfs do something like this:
```
gsido mkdir /hdfs/store/user/truggles/mySubmitDir
gsido rsync -ahP /nfs_scratch/truggles/httSept04skimMerged/*.root /hdfs/store/user/truggles/httSept04skimMerge/
```

